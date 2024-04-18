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

struct ExecContext
{
    SListRecordIdx item;
    enum FunctionType type;
};

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

typedef void (*DotProcessFn)(struct DotCommand *, struct ExecContext *);
/// ********

#define SCRIPT_LIST_ADD(dst) \
    SCRIPT_LIST_ADD__(dst, temp_var ## __LINE__ )

#define SCRIPT_LIST_ADD__(dst, tmp_var) \
    SListRecordIdx tmp_var; \
    (tmp_var) = script_list_add(dst); \
    if ((dst) == 0)  (dst) = tmp_var;       \
    (&gameadd.script.list_records[tmp_var])

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

struct DotCommand *dot_last(DotCommandIdx idx)
{
    static struct DotCommand bad;
    struct DotCommand *cmd = &gameadd.script.dot_commands[idx];
    if ((cmd == NULL) || (idx == 0))
        return &bad;
    for (; cmd->chain_next; cmd = &gameadd.script.dot_commands[cmd->chain_next])
    {
        // empty
    }
    return cmd;
}

#define DOT_ALLOC \
    DotCommandIdx cmd_idx = dot_alloc_(); \
    struct DotCommand *cmd = &gameadd.script.dot_commands[cmd_idx];

static DotCommandIdx dot_alloc_()
{// Allocating
    DotCommandIdx idx = gameadd.script.free_dot_command;
    struct DotCommand *cmd = &gameadd.script.dot_commands[idx];
    if (gameadd.script.free_dot_command == 0)
    {
        if (gameadd.script.dot_commands_num >= DOT_COMMANDS_COUNT)
        {
            SCRPTERRLOG("Too many dot commands!");
            return 0;
        }
        idx = gameadd.script.dot_commands_num;
        gameadd.script.dot_commands_num++;
        cmd = &gameadd.script.dot_commands[idx];
    }
    else
    {
        gameadd.script.free_dot_command = cmd->chain_next;
    }
    cmd->chain_next = 0;
    return idx;
}

void add_to_condition_sublist(struct ParserContext *context, DotCommandIdx cmd_idx)
{
    // add to condition if any
    if (context->prev_command != 0)
    {
        context->prev_command->chain_next = cmd_idx;
        return;
    }
    int condition = get_script_current_condition();
    if (condition == CONDITION_ALWAYS)
    {
        //TODO: what if next_command_reusable != 0
    }
    else
    {
        struct Condition *condt = &gameadd.script.conditions[condition];
        SCRIPT_LIST_ADD(condt->dotlist_to_activate)->dot_command = cmd_idx;
    }
    context->prev_command = &gameadd.script.dot_commands[cmd_idx];
}

void dot_finalize(struct DotCommand *cmd)
{
    int condition = get_script_current_condition();
    if (condition == CONDITION_ALWAYS)
    {
        // TODO: activate & move to free
    }
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

static void invalid_dot_process(struct DotCommand *cmd, struct ExecContext *context)
{
    ERRORLOG("Invalid dot process!");
}

const struct AdvCommandDesc advanced_subcommands[] = {
        {"RANDOM", 0, "Aaaaaa", cmd_random},
        {NULL,     0, "",       0}
};

/// ********

static DotProcessFn dot_process_fns[] = {
        &invalid_dot_process,

};

void process_dot_script(SListRecordIdx cmd_idx)
{
    struct ThingListRecord *cmd_item;
    for (; cmd_idx != 0; cmd_idx = cmd_item->next_record)
    {
        cmd_item = &gameadd.script.list_records[cmd_idx];
        struct DotCommand *cmd = &gameadd.script.dot_commands[cmd_item->dot_command];
        struct ExecContext context = {0}; //One context per chain

        for (;; cmd = &gameadd.script.dot_commands[cmd->chain_next])
        {
            if (cmd->command_index >= STATIC_SIZE(dot_process_fns))
            {
                ERRORLOG("Invalid command index!");
                if (cmd->chain_next == 0)
                    break;
                continue;
            }

            DotProcessFn fn = dot_process_fns[cmd->command_fn];
            fn(cmd, &context);
            if (cmd->chain_next == 0)
                break;
        }
    }
}