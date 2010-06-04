/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file player_computer.c
 *     Computer player definitions and activities.
 * @par Purpose:
 *     Defines a computer player control variables and events/checks/processes
 *      functions.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     10 Mar 2009 - 20 Mar 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "player_computer.h"

#include <limits.h>
#include "globals.h"
#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "bflib_dernc.h"
#include "bflib_memory.h"

#include "config.h"
#include "config_creature.h"
#include "keeperfx.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
struct TrapDoorSelling {
    long category;
    long model;
};

struct MoveToRoom {
    char kind;
    long field_1;
};

/******************************************************************************/
long task_dig_room_passage(struct Computer2 *comp, struct ComputerTask *ctask);
long task_dig_room(struct Computer2 *comp, struct ComputerTask *ctask);
long task_check_room_dug(struct Computer2 *comp, struct ComputerTask *ctask);
long task_place_room(struct Computer2 *comp, struct ComputerTask *ctask);
long task_dig_to_entrance(struct Computer2 *comp, struct ComputerTask *ctask);
long task_dig_to_gold(struct Computer2 *comp, struct ComputerTask *ctask);
long task_dig_to_attack(struct Computer2 *comp, struct ComputerTask *ctask);
long task_magic_call_to_arms(struct Computer2 *comp, struct ComputerTask *ctask);
long task_pickup_for_attack(struct Computer2 *comp, struct ComputerTask *ctask);
long task_move_creature_to_room(struct Computer2 *comp, struct ComputerTask *ctask);
long task_move_creature_to_pos(struct Computer2 *comp, struct ComputerTask *ctask);
long task_move_creatures_to_defend(struct Computer2 *comp, struct ComputerTask *ctask);
long task_slap_imps(struct Computer2 *comp, struct ComputerTask *ctask);
long task_dig_to_neutral(struct Computer2 *comp, struct ComputerTask *ctask);
long task_magic_speed_up(struct Computer2 *comp, struct ComputerTask *ctask);
long task_wait_for_bridge(struct Computer2 *comp, struct ComputerTask *ctask);
long task_attack_magic(struct Computer2 *comp, struct ComputerTask *ctask);
long task_sell_traps_and_doors(struct Computer2 *comp, struct ComputerTask *ctask);
/******************************************************************************/
const struct TaskFunctions task_function[] = {
    {NULL, NULL},
    {"COMPUTER_DIG_ROOM_PASSAGE", task_dig_room_passage},
    {"COMPUTER_DIG_ROOM",         task_dig_room},
    {"COMPUTER_CHECK_ROOM_DUG",   task_check_room_dug},
    {"COMPUTER_PLACE_ROOM",       task_place_room},
    {"COMPUTER_DIG_TO_ENTRANCE",  task_dig_to_entrance},
    {"COMPUTER_DIG_TO_GOLD",      task_dig_to_gold},
    {"COMPUTER_DIG_TO_ATTACK",    task_dig_to_attack},
    {"COMPUTER_MAGIC_CALL_TO_ARMS", task_magic_call_to_arms},
    {"COMPUTER_PICKUP_FOR_ATTACK", task_pickup_for_attack},
    {"COMPUTER_MOVE_CREATURE_TO_ROOM", task_move_creature_to_room},
    {"COMPUTER_MOVE_CREATURE_TO_POS", task_move_creature_to_pos},
    {"COMPUTER_MOVE_CREATURES_TO_DEFEND", task_move_creatures_to_defend},
    {"COMPUTER_SLAP_IMPS",        task_slap_imps},
    {"COMPUTER_DIG_TO_NEUTRAL",   task_dig_to_neutral},
    {"COMPUTER_MAGIC_SPEED_UP",   task_magic_speed_up},
    {"COMPUTER_WAIT_FOR_BRIDGE",  task_wait_for_bridge},
    {"COMPUTER_ATTACK_MAGIC",     task_attack_magic},
    {"COMPUTER_SELL_TRAPS_AND_DOORS", task_sell_traps_and_doors},
};

const struct TrapDoorSelling trapdoor_sell[] = {
    {TDSC_Door, 4},
    {TDSC_Trap, 1},
    {TDSC_Trap, 6},
    {TDSC_Door, 3},
    {TDSC_Trap, 5},
    {TDSC_Trap, 4},
    {TDSC_Door, 2},
    {TDSC_Trap, 3},
    {TDSC_Door, 1},
    {TDSC_Trap, 2},
    {TDSC_EndList, 0},
};

const struct MoveToRoom move_to_room[] = {
    {RoK_TRAINING,  40},
    {RoK_LIBRARY,   35},
    {RoK_WORKSHOP,  32},
    {RoK_SCAVENGER, 20},
    {RoK_NONE,       0},
};
/******************************************************************************/
DLLIMPORT long _DK_task_dig_room_passage(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_dig_room(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_check_room_dug(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_place_room(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_dig_to_entrance(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_dig_to_gold(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_dig_to_attack(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_magic_call_to_arms(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_pickup_for_attack(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_move_creature_to_room(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_move_creature_to_pos(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_move_creatures_to_defend(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_slap_imps(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_dig_to_neutral(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_magic_speed_up(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_wait_for_bridge(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_attack_magic(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT long _DK_task_sell_traps_and_doors(struct Computer2 *comp, struct ComputerTask *ctask);
DLLIMPORT struct ComputerTask *_DK_get_task_in_progress(struct Computer2 *comp, long a2);
DLLIMPORT struct ComputerTask *_DK_get_free_task(struct Computer2 *comp, long a2);
DLLIMPORT short _DK_fake_dump_held_creatures_on_map(struct Computer2 *comp, struct Thing *thing, struct Coord3d *pos);
DLLIMPORT long _DK_fake_place_thing_in_power_hand(struct Computer2 *comp, struct Thing *thing, struct Coord3d *pos);
DLLIMPORT struct Thing *_DK_find_creature_to_be_placed_in_room(struct Computer2 *comp, struct Room **roomp);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
struct ComputerTask *get_computer_task(long idx)
{
    if ((idx < 1) || (idx >= COMPUTER_TASKS_COUNT))
    {
        return &game.computer_task[0];
    } else
    {
        return &game.computer_task[idx];
    }
}

TbBool computer_task_invalid(struct ComputerTask *ctask)
{
    if (ctask <= &game.computer_task[0])
        return true;
    return false;
}

TbBool remove_task(struct Computer2 *comp, struct ComputerTask *ctask)
{
  struct ComputerTask *nxctask;
  long i;
  i = comp->field_14C6;
  if (&game.computer_task[i] == ctask)
  {
    comp->field_14C6 = ctask->next_task;
    ctask->next_task = 0;
    set_flag_byte(&ctask->field_0, 0x01, false);
    return false;
  }
  nxctask = &game.computer_task[i];
  while (!computer_task_invalid(nxctask))
  {
      i = nxctask->next_task;
      if (&game.computer_task[i] == ctask)
      {
        nxctask->next_task = ctask->next_task;
        ctask->next_task = 0;
        set_flag_byte(&ctask->field_0, 0x01, false);
        return true;
      }
      nxctask = &game.computer_task[i];
  }
  return false;
}

struct ComputerTask *get_task_in_progress(struct Computer2 *comp, long a2)
{
    return _DK_get_task_in_progress(comp, a2);
}

struct ComputerTask *get_free_task(struct Computer2 *comp, long a2)
{
    return _DK_get_free_task(comp, a2);
}

short fake_dump_held_creatures_on_map(struct Computer2 *comp, struct Thing *thing, struct Coord3d *pos)
{
    return _DK_fake_dump_held_creatures_on_map(comp, thing, pos);
}

long fake_place_thing_in_power_hand(struct Computer2 *comp, struct Thing *thing, struct Coord3d *pos)
{
    SYNCDBG(9,"Starting");
    return _DK_fake_place_thing_in_power_hand(comp, thing, pos);
}

TbBool worker_needed_in_dungeons_room_kind(const struct Dungeon *dungeon, long rkind)
{
    long i;
    switch (rkind)
    {
    case RoK_LIBRARY:
        if (dungeon->field_F78 < 0)
            return false;
        return true;
    case RoK_TRAINING:
        if (2 * dungeon->field_14B8 >= dungeon->field_AF9)
            return false;
        return true;
    case RoK_WORKSHOP:
        for (i = 1; i < TRAP_TYPES_COUNT; i++)
        {
            if ((dungeon->trap_buildable[i]) && (dungeon->trap_amount[i] == 0))
            {
              break;
            }
        }
        if (i == TRAP_TYPES_COUNT)
            return false;
        return true;
    default:
        return true;
    }
}

long get_job_for_room(long rkind)
{
    switch (rkind)
    {
    case RoK_LIBRARY:
        return Job_RESEARCH;
    case RoK_TRAINING:
        return Job_TRAIN;
    case RoK_WORKSHOP:
        return Job_MANUFACTURE;
    case RoK_SCAVENGER:
        return Job_SCAVENGE;
    case RoK_TEMPLE:
        return Job_TEMPLE;
    case RoK_GUARDPOST:
        return Job_GUARD;
//    case RoK_TORTURE: -- no 'bad jobs' should be listed here
//        return Job_KINKY_TORTURE;
    default:
        return Job_NULL;
    }
}

TbBool person_will_do_job_for_room(const struct Thing *thing, const struct Room *room)
{
    struct CreatureStats *crstat;
    crstat = creature_stats_get_from_thing(thing);
    return (get_job_for_room(room->kind) & crstat->jobs_not_do) == 0;
}

TbBool person_will_do_job_for_room_kind(const struct Thing *thing, long rkind)
{
    struct CreatureStats *crstat;
    crstat = creature_stats_get_from_thing(thing);
    return (get_job_for_room(rkind) & crstat->jobs_not_do) == 0;
}

struct Room *get_room_to_place_creature(const struct Computer2 *comp, const struct Thing *thing)
{
  const struct Dungeon *dungeon;
  long chosen_priority;
  struct Room *chosen_room;
  struct Room *room;
  long total_spare_cap;
  long i,k,rkind;

    dungeon = comp->dungeon;

    chosen_room = NULL;
    chosen_priority = LONG_MIN;
    for (k=0; move_to_room[k].kind != RoK_NONE; k++)
    {
        rkind = move_to_room[k].kind;
        if (person_will_do_job_for_room_kind(thing,rkind))
        {
            if (!worker_needed_in_dungeons_room_kind(dungeon,rkind))
                continue;
        }
        // Find specific room which meets capacity demands
        i = dungeon->room_kind[rkind];
        room = find_room_with_most_spare_capacity_starting_with(i,&total_spare_cap);
        if (room_is_invalid(room))
            continue;
        if (chosen_priority < total_spare_cap * move_to_room[k].field_1)
        {
            chosen_priority = total_spare_cap * move_to_room[k].field_1;
            chosen_room = room;
        }
    }
  return chosen_room;
}

struct Thing *find_creature_to_be_placed_in_room(struct Computer2 *comp, struct Room **roomp)
{
    Thing_Maximizer_Filter filter;
    struct CompoundFilterParam param;
    struct Dungeon *dungeon;
    struct Thing *thing;
    struct Room *room;
    SYNCDBG(9,"Starting");
    dungeon = comp->dungeon;
    if (dungeon_invalid(dungeon))
    {
        ERRORLOG("Invalid dungeon in computer player.");
        return INVALID_THING;
    }
    //return _DK_find_creature_to_be_placed_in_room(comp, roomp);
    param.ptr1 = (void *)comp;
    filter = player_list_creature_filter_needs_to_be_placed_in_room;
    thing = get_player_list_creature_with_filter(dungeon->creatr_list_start, filter, &param);
    if (thing_is_invalid(thing))
        return INVALID_THING;
    room = get_room_of_given_kind_for_thing(thing,dungeon,param.num2);
    if (room_is_invalid(room))
        return INVALID_THING;
    *roomp = room;
    return thing;
}

long task_dig_room_passage(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_dig_room_passage(comp,ctask);
}

long task_dig_room(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_dig_room(comp,ctask);
}

long task_check_room_dug(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_check_room_dug(comp,ctask);
}

long task_place_room(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_place_room(comp,ctask);
}

long task_dig_to_entrance(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_dig_to_entrance(comp,ctask);
}

long task_dig_to_gold(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_dig_to_gold(comp,ctask);
}

long task_dig_to_attack(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_dig_to_attack(comp,ctask);
}

long task_magic_call_to_arms(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_magic_call_to_arms(comp,ctask);
}

long task_pickup_for_attack(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_pickup_for_attack(comp,ctask);
}

long task_move_creature_to_room(struct Computer2 *comp, struct ComputerTask *ctask)
{
    struct Thing *thing;
    struct Room *room;
    struct Coord3d pos;
    long i;
    SYNCDBG(9,"Starting");
    //return _DK_task_move_creature_to_room(comp,ctask);
    thing = thing_get(comp->field_14C8);
    if (!thing_is_invalid(thing))
    {
      room = room_get(ctask->word_80);
      pos.x.val = room->stl_x << 8;
      pos.y.val = room->stl_y << 8;
      pos.z.val = 256;
      if (fake_dump_held_creatures_on_map(comp, thing, &pos) > 0)
        return 2;
      remove_task(comp, ctask);
      return 0;
    }
    i = ctask->field_7C;
    ctask->field_7C--;
    if (i <= 0)
    {
      remove_task(comp, ctask);
      return 1;
    }
    thing = find_creature_to_be_placed_in_room(comp, &room);
    if (!thing_is_invalid(thing))
    {
        //TODO CREATURE_AI try to make sure the creature will do proper activity in the room
        //     ie. select a room tile which is far from CTA and enemies
        //TODO CREATURE_AI don't place creatures at center of a temple
        //TODO CREATURE_AI make sure to place creatures at "active" portal tile if we want them to leave
        ctask->word_80 = room->index;
        pos.x.val = room->stl_x << 8;
        pos.y.val = room->stl_y << 8;
        pos.z.val = 256;
        if ( fake_place_thing_in_power_hand(comp, thing, &pos) )
          return 2;
        remove_task(comp, ctask);
        return 0;
      }
      remove_task(comp, ctask);
      return 0;
}

long task_move_creature_to_pos(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_move_creature_to_pos(comp,ctask);
}

long task_move_creatures_to_defend(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_move_creatures_to_defend(comp,ctask);
}

long task_slap_imps(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_slap_imps(comp,ctask);
}

long task_dig_to_neutral(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_dig_to_neutral(comp,ctask);
}

long task_magic_speed_up(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_magic_speed_up(comp,ctask);
}

long task_wait_for_bridge(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_wait_for_bridge(comp,ctask);
}

long task_attack_magic(struct Computer2 *comp, struct ComputerTask *ctask)
{
    SYNCDBG(9,"Starting");
    return _DK_task_attack_magic(comp,ctask);
}

long task_sell_traps_and_doors(struct Computer2 *comp, struct ComputerTask *ctask)
{
    struct Dungeon *dungeon;
    const struct TrapDoorSelling *tdsell;
    TbBool item_sold;
    long value,model;
    long i;
    SYNCDBG(19,"Starting");
    //return _DK_task_sell_traps_and_doors(comp,ctask);
    dungeon = comp->dungeon;
    if (dungeon_invalid(dungeon))
    {
        ERRORLOG("Invalid dungeon in computer player.");
        return 0;
    }
    if ((ctask->field_7C >= ctask->long_76) && (ctask->field_80 >= dungeon->field_AF9))
    {
        i = 0;
        value = 0;
        item_sold = false;
        for (i=0; i < sizeof(trapdoor_sell)/sizeof(trapdoor_sell[0]); i++)
        {
            tdsell = &trapdoor_sell[ctask->long_86];
            switch (tdsell->category)
            {
            case TDSC_Door:
                model = tdsell->model;
                if ((model < 0) || (model >= DOOR_TYPES_COUNT))
                {
                    ERRORLOG("Internal error - invalid model %ld in slot %ld",model,i);
                    break;
                }
                if (dungeon->door_amount[model] > 0)
                {
                  item_sold = true;
                  value = game.doors_config[model].selling_value;
                  if (remove_workshop_item(dungeon->field_E9F, 9, model))
                  {
                    remove_workshop_object_from_player(dungeon->field_E9F, door_to_object[model]);
                  }
                  SYNCDBG(9,"Door model %ld sold for %ld gold",model,value);
                }
                break;
            case TDSC_Trap:
                model = tdsell->model;
                if ((model < 0) || (model >= TRAP_TYPES_COUNT))
                {
                    ERRORLOG("Internal error - invalid model %ld in slot %ld",model,i);
                    break;
                }
                if (dungeon->trap_amount[model] > 0)
                {
                  item_sold = true;
                  value = game.traps_config[model].selling_value;
                  if (remove_workshop_item(dungeon->field_E9F, 8, model))
                  {
                    remove_workshop_object_from_player(dungeon->field_E9F, trap_to_object[model]);
                  }
                  SYNCDBG(9,"Trap model %ld sold for %ld gold",model,value);
                }
                break;
            default:
                ERRORLOG("Unknown SELL_ITEM type");
                break;
            }
            ctask->long_86++;
            if (trapdoor_sell[ctask->long_86].category == TDSC_EndList)
                ctask->long_86 = 0;
            if (item_sold)
            {
                ctask->field_70--;
                if (ctask->field_70 > 0)
                {
                  ctask->long_76 += value;
                  dungeon->field_AFD += value;
                  dungeon->field_AF9 += value;
                  return 1;
                }
                remove_task(comp, ctask);
                return 1;
            }
        }
    }
    SYNCDBG(9,"Couldn't sell anything, aborting.");
    remove_task(comp, ctask);
    return 0;
}

TbBool create_task_move_creatures_to_defend(struct Computer2 *comp, struct Coord3d *pos, long creatrs_num, unsigned long evflags)
{
    struct ComputerTask *ctask;
    SYNCDBG(7,"Starting");
    ctask = get_free_task(comp, 1);
    if (ctask == NULL)
        return false;
    ctask->ttype = CTT_MoveCreaturesToDefend;
    ctask->pos_76.x.val = pos->x.val;
    ctask->pos_76.y.val = pos->y.val;
    ctask->pos_76.z.val = pos->z.val;
    ctask->field_7C = creatrs_num;
    ctask->field_70 = evflags;
    ctask->field_A = game.play_gameturn;
    ctask->field_5C = game.play_gameturn;
    ctask->field_60 = comp->field_34;
    return true;
}

TbBool create_task_magic_call_to_arms(struct Computer2 *comp, struct Coord3d *pos, long creatrs_num)
{
    struct ComputerTask *ctask;
    SYNCDBG(7,"Starting");
    ctask = get_free_task(comp, 1);
    if (ctask == NULL)
        return false;
    ctask->ttype = CTT_MagicCallToArms;
    ctask->field_1 = 0;
    ctask->pos_76.x.val = pos->x.val;
    ctask->pos_76.y.val = pos->y.val;
    ctask->pos_76.z.val = pos->z.val;
    ctask->field_7C = creatrs_num;
    ctask->field_A = game.play_gameturn;
    ctask->field_60 = 25;
    ctask->field_5C = game.play_gameturn - 25;
    ctask->field_8E = 2500;
    return true;
}

TbBool create_task_sell_traps_and_doors(struct Computer2 *comp, long value)
{
    struct ComputerTask *ctask;
    SYNCDBG(7,"Starting");
    ctask = get_free_task(comp, 1);
    if (ctask == NULL)
        return false;
    ctask->ttype = CTT_SellTrapsAndDoors;
    ctask->field_70 = 0;
    ctask->field_A = game.play_gameturn;
    ctask->field_5C = game.play_gameturn;
    ctask->field_60 = 1;
    ctask->field_70 = 5;
    ctask->long_76 = 0;
    ctask->field_7C = value;
    ctask->field_80 = value;
    ctask->long_86 = 0;
    return true;
}

TbBool create_task_move_creature_to_pos(struct Computer2 *comp, struct Thing *thing, long a2, long a3)
{
    struct ComputerTask *ctask;
    SYNCDBG(7,"Starting");
    ctask = get_free_task(comp, 0);
    if (ctask == NULL)
        return false;
    ctask->ttype = CTT_MoveCreatureToPos;
    ctask->word_86 = a2 << 8;
    ctask->word_88 = a3 << 8;
    ctask->word_76 = thing->index;
    ctask->word_80 = 0;
    ctask->field_A = game.play_gameturn;
    return true;
}

long process_tasks(struct Computer2 *comp)
{
    struct ComputerTask *ctask;
    long ndone;
    long i,n;
    unsigned long k;
    //return _DK_process_tasks(comp);
    ndone = 0;
    k = 0;
    i = comp->field_14C6;
    while (i != 0)
    {
        if ((i < 0) || (i >= COMPUTER_TASKS_COUNT))
        {
          ERRORLOG("Jump to invalid computer task %ld detected",i);
          break;
        }
        if (comp->field_10 <= 0)
            break;
        ctask = &game.computer_task[i];
        i = ctask->next_task;
        if ((ctask->field_0 & 0x01) != 0)
        {
            n = ctask->ttype;
            if ((n > 0) && (n < sizeof(task_function)/sizeof(task_function[0])))
            {
                SYNCDBG(12,"Computer Task Type %ld",n);
                task_function[n].func(comp, ctask);
                ndone++;
            } else
            {
                ERRORLOG("Bad Computer Task Type %ld",n);
            }
        }
        k++;
        if (k > COMPUTER_TASKS_COUNT)
        {
          ERRORLOG("Infinite loop detected when sweeping computer tasks");
          break;
        }
    }
    return ndone;
}
/******************************************************************************/
