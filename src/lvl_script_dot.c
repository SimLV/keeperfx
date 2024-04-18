/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file lvl_script_dot.c
 *     Commands for dot notation
 * @author   KeeperFX Team
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "lvl_script.h"
#include "lvl_script_commands.h"
#include "lvl_script_conditions.h"

#include "game_legacy.h"
#include "game_merge.h"
#include "map_data.h"
#include "player_instances.h"
#include "thing_data.h"
#include "bflib_planar.h"
#include "thing_effects.h"
#include "thing_physics.h"
#include "thing_navigate.h"
#include "post_inc.h"

enum DotCmd
{
    DF_INVALID = 0,
    DF_FIND_CREATURE,
    DF_CREATURE_TO_LOC,
    DF_CREATE_EFFECT,
    DF_READ_GROUP,
    DF_CRE_CAST_POWER,
    DF_CREATE_OBJECT,
};

/// ********

SListRecordIdx script_list_add(SListRecordIdx idx)
{
    SListRecordIdx ret;
    if (gameadd.script.free_list_record_idx != 0) // Take one from free list
    {
        ret = gameadd.script.free_list_record_idx;
        gameadd.script.free_list_record_idx = gameadd.script.list_records[ret].next_record;
    }
    else
    {
        if (gameadd.script.free_list_record_num >= SLIST_RECORDS_COUNT)
        {
            ERRORLOG("Too many slist items!");
            return 0;
        }
        ret = gameadd.script.free_list_record_num++;
    }
    gameadd.script.list_records[ret].next_record = 0;

    if (idx != 0)
    {
        struct ThingListRecord *cmd = &gameadd.script.list_records[idx];
        SListRecordIdx cnt = SLIST_RECORDS_COUNT;
        for (; cmd->next_record; cmd = &gameadd.script.list_records[cmd->next_record])
        {
            if (cnt-- < 0)
            {
                ERRORLOG("Slist damaged!");
                break;
            }
        }
        cmd->next_record = ret;
    }
    return ret;
}

void script_list_free(SListRecordIdx idx)
{
    if (idx != 0)
    {
        struct ThingListRecord *cmd = &gameadd.script.list_records[idx];

        SListRecordIdx prev = 0;
        SListRecordIdx next = cmd->next_record;
        if (next)
        {
            // Reverse the list
            while (next)
            {
                next = gameadd.script.list_records[idx].next_record;
                gameadd.script.list_records[idx].next_record = prev;
                prev = idx;
                idx = next;
            }
            idx = prev;
        }
        while (idx)
        {
            cmd = &gameadd.script.list_records[idx];
            next = cmd->next_record;
            if (idx == gameadd.script.free_list_record_num - 1)
            {
                gameadd.script.free_list_record_num--; //just last item
                cmd->next_record = 0;
            }
            else
            {
                cmd->next_record = gameadd.script.free_list_record_idx;
                gameadd.script.free_list_record_idx = idx;
            }
            idx = next;
        }
    }
}

/*
 * next_record of result should not be used or modified
 */
struct ThingListRecord *script_list_pop(SListRecordIdx *idx)
{
    if (*idx == 0)
    {
        return NULL;
    }
    struct ThingListRecord *cmd = &gameadd.script.list_records[*idx];
    if (cmd->next_record == 0)
    {
        script_list_free(*idx);
        *idx = 0;
        return cmd;
    }
    struct ThingListRecord *nxt = &gameadd.script.list_records[cmd->next_record];
    while (nxt->next_record)
    {
        cmd = nxt;
        nxt = &gameadd.script.list_records[nxt->next_record];
    }
    script_list_free(cmd->next_record);
    cmd->next_record = 0;
    return nxt;
}

/// ********

void script_add_creature_to_result(struct ScriptContext *context, struct Thing *thing)
{
    SCRIPT_LIST_ADD(context->save_group)->thing = thing->index;
}

/// ********
static TbBool whine(struct ParserContext *context)
{
    WARNMSG("Script line has no effect at %d", text_line_number);
    return false;
}

static TbBool cmd_random(struct ParserContext *context, intptr_t option)
{
    return true;
}

/// ********

const struct AdvCommandDesc advanced_subcommands[] = {
        {"RANDOM", 0, "Aaaaaa", cmd_random},
        {NULL,     0, "",       0}
};

/// ********

void process_dot_script(SListRecordIdx cmd_idx)
{
}