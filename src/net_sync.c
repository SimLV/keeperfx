/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file net_sync.c
 *     Network game synchronization for Dungeon Keeper.
 * @par Purpose:
 *     Functions to keep network games synchronized.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     11 Mar 2010 - 30 Aug 2020
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "net_sync.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "bflib_network.h"

#include "config.h"
#include "front_network.h"
#include "player_data.h"
#include "game_merge.h"
#include "net_game.h"
#include "lens_api.h"
#include "game_legacy.h"
#include "keeperfx.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
struct SyncPartCommon
{
    unsigned long play_gameturn;
    unsigned long action_turn_rand_seed;
};

/******************************************************************************/
static char desync_info[(2 * CKS_MAX ) + 1] = ".....................";
static const char desync_letters[CKS_MAX] = {
  'A', // CKS_Action
  'P', // CKS_Players
  '0', // CKS_Creatures_1
  '1', // CKS_Creatures_2
  '2', // CKS_Creatures_3
  '3', // CKS_Creatures_4
  '4', // CKS_Creatures_5
  '5', // CKS_Creatures_6
  'O', // CKS_Things
  'E', // CKS_Effects
  'R', // CKS_Rooms
};

struct ChecksumStorage player_checksum_storage[PLAYERS_EXT_COUNT] = {0};
/******************************************************************************/
long get_resync_sender(void)
{
    for (int i = 0; i < NET_PLAYERS_COUNT; i++)
    {
        struct PlayerInfo* player = get_player(i);
        if (player_exists(player) && ((player->allocflags & PlaF_CompCtrl) == 0))
            return i;
  }
  return -1;
}

// Return true when we have finished sending sync data
static TbBool send_resync_game(TbBool first_resync)
{
    TbBool ret;
    if (first_resync)
    {
        NETLOG("Initiating resync turn: %ld", game.play_gameturn);
        //TODO NET see if it is necessary to dump to file... probably superfluous
        char* fname = prepare_file_path(FGrp_Save, "resync.dat");
        TbFileHandle fh = LbFileOpen(fname, Lb_FILE_MODE_NEW);
        if (fh == -1)
        {
            ERRORLOG("Can't open resync file.");
            return true;
        }

        LbFileWrite(fh, &game, sizeof(game));
        LbFileClose(fh);
    }

    struct SyncPartCommon part1 = {
        .play_gameturn = game.play_gameturn,
        .action_turn_rand_seed = gameadd.action_turn_rand_seed
    };

    struct SyncArrayItem data[] =
    {
        { &part1, sizeof(part1) },
        { &game, sizeof(game) },
        { NULL, 0 },
    };
    ret = LbNetwork_Resync(first_resync, game.play_gameturn, data);
    if (ret)
    {
        NETLOG("Done syncing");
    }
    return ret;
}

static TbBool receive_resync_game(TbBool first_resync)
{
    TbBool ret;
    if (first_resync)
    {
        NETLOG("Initiating resync turn:%ld", game.play_gameturn);
    }
    struct SyncPartCommon part1;
    struct SyncArrayItem data[] =
    {
        { &part1, sizeof(part1) },
        { &game, sizeof(game) },
        { NULL, 0 },
    };
    ret = LbNetwork_Resync(first_resync, game.play_gameturn, data);
    if (ret)
    {
        game.play_gameturn = part1.play_gameturn;
        gameadd.action_turn_rand_seed = part1.action_turn_rand_seed;
        NETLOG("Done syncing");
    }
    return ret;
}

TbBool resync_game(TbBool first_resync)
{
    SYNCDBG(2, "Starting");
    reset_eye_lenses();
    int i = get_resync_sender();
    if (is_my_player_number(i))
    {
        return send_resync_game(first_resync);
    } else
    {
        return receive_resync_game(first_resync);
    }
}

/**
 * Exchanges verification packets between all players, making sure level data is identical.
 * @return Returns true if all players return same checksum.
 */
void perform_checksum_verification(void)
{
    unsigned long checksum_mem = 0;
    for (int i = 1; i < THINGS_COUNT; i++)
    {
        struct Thing* thing = thing_get(i);
        if (thing_exists(thing)) {
            SHIFT_CHECKSUM(checksum_mem);
            checksum_mem ^= (thing->mappos.z.val << 16)
                ^ (thing->mappos.y.val << 8)
                ^ thing->mappos.x.val
                ^ thing->model;
        }
    }
    clear_packets();
    struct Packet* pckt = get_packet(my_player_number);
    set_packet_action(pckt, PckA_LevelExactCheck, 0, 0, 0, 0);
    pckt->chksum = checksum_mem ^ game.action_rand_seed;
    if (LbNetwork_Exchange(pckt))
    {
        ERRORLOG("Network exchange failed on level checksum verification");
    }
    if ( checksums_different() )
    {
        ERRORLOG("Level checksums different for network players");
    }
}

static void update_desync_info(struct PacketEx* v1, struct PacketEx* v2)
{
  for (int i = 0; i < (int)CKS_MAX; i++)
  {
      if (v1->sums[i] != v2->sums[i])
      {
          desync_info[i*2] = desync_letters[i];
      }
  }
}
/**
 * Checks if all active players packets have same checksums.
 * @return Returns false if all checksums are same; true if there's mismatch.
 */
TbBool checksums_different(void)
{
    TbChecksum checksum = 0;
    struct PacketEx* base = NULL;
    for (int i = 0; i < PLAYERS_COUNT; i++)
    {
        struct PlayerInfo* player = get_player(i);
        if (player_exists(player) && ((player->allocflags & PlaF_CompCtrl) == 0))
        {
            struct PacketEx* pckt = get_packet_ex_direct(player->packet_num);
            if (!base)
            {
                checksum = pckt->packet.chksum;
                base = pckt;
            }
            else if (checksum != pckt->packet.chksum)
            {
                update_desync_info(base, pckt);
                return true;
            }
        }
  }
  return false;
}

const char *get_desync_info()
{
    return desync_info;
}

static TbBigChecksum compute_player_checksum(struct PlayerInfo *player)
{
    TbBigChecksum sum = 0;
    if (((player->allocflags & PlaF_CompCtrl) == 0) && (player->acamera != NULL))
    {
        struct Coord3d* mappos = &(player->acamera->mappos);
        sum ^= (TbBigChecksum)player->instance_remain_rurns + (TbBigChecksum)player->instance_num;
        SHIFT_CHECKSUM(sum);
        sum ^= (TbBigChecksum)mappos->x.val + (TbBigChecksum)mappos->z.val + (TbBigChecksum)mappos->y.val;
    }
    return sum;
}

/**
 * Computes checksum of current state of all existing players.
 * @return The checksum value.
 */
TbBigChecksum compute_players_checksum(void)
{
    TbBigChecksum sum = 0;
    for (int i = 0; i < PLAYERS_COUNT; i++)
    {
        struct PlayerInfo* player = get_player(i);
        if (player_exists(player))
        {
            SHIFT_CHECKSUM(sum);
            sum ^= compute_player_checksum(player);
      }
    }
    return sum;
}

TbBigChecksum get_thing_checksum(const struct Thing *thing)
{
    SYNCDBG(18,"Starting");
    if (!thing_exists(thing))
        return 0;
    TbBigChecksum csum = ((ulong)thing->class_id << 24);
    csum ^= (((ulong)thing->mappos.z.val) << 16) ^
            (((ulong)thing->mappos.x.val) << 8) ^
            (ulong)thing->mappos.y.val;
    csum ^= (ulong)thing->health ^
            (((ulong)thing->model) << 4) ^
            (((ulong)thing->owner) << 12);
    csum ^= thing->index;
    if (thing->class_id == TCls_Creature)
    {
        struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
        SHIFT_CHECKSUM(csum);
        csum ^= (ulong)cctrl->inst_turn ^
            (((ulong)cctrl->instance_id) << 4) ^
            (((ulong)thing->field_49) << 12) ^
            (ulong)thing->field_48;

        player_checksum_storage[thing->owner].checksum_creatures[thing->index] = (thing->index << 16) | ((csum & 0xFFFF) ^ (csum >> 16));
    }
    return csum;
}

void resync_reset_storage()
{
    memset(player_checksum_storage, 0, sizeof(player_checksum_storage));
}
/**
 * Adds given value to checksum at current game turn stored in packet file.
 *
 * @param plyr_idx The player whose checksum is computed.
 * @param sum Checksum increase.
 * @param area_name Name of the area from which the checksum increase comes, for logging purposes.
 */
void player_packet_checksum_add(PlayerNumber plyr_idx, TbBigChecksum sum, enum ChecksumKind kind)
{
    struct PacketEx* pckt = get_packet_ex(plyr_idx);
    pckt->packet.chksum ^= sum;
    pckt->sums[(int)kind] ^= sum;
    SYNCDBG(9,"Checksum updated kind:%d amount:%06lX", kind,(unsigned long)sum);
}
/******************************************************************************/
/******************************************************************************/
#ifdef __cplusplus
}
#endif
