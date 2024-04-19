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

enum PrecommandIdx
{
    PCI_INVALID = 0,
    PCI_LOCATION,
    PCI_RANDOM,
};

typedef void (*PreCommandFn)(struct ScriptContext *context, struct PreCommand *cmd, struct TriggerCommonData *dst);
static void call_chain(struct ScriptContext *context);
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
/*
 * Saves things to context so it will be optionally saved to a group
 * */
void script_add_thing_to_result(struct ScriptContext *context, struct Thing *thing)
{
    SCRIPT_LIST_ADD(context->save_group)->thing = thing->index;
}

/// ********

static void check_random(struct ParserContext *context, const struct ScriptLine *scline)
{
    SCRPTERRLOG("Not implemented");
}

/// ********

static void cmd_location(struct ScriptContext *context, struct PreCommand *cmd, struct TriggerCommonData *dst)
{
    SListRecordIdx cmd_idx = gameadd.script.groups[cmd->np[0]];
    SListRecordIdx next_precommand = context->precommand;

    struct ThingListRecord *cmd_item;
    for (; cmd_idx != 0; cmd_idx = cmd_item->next_record)
    {
        cmd_item = &gameadd.script.list_records[cmd_idx];

        context->active_location = thing_get(cmd_item->thing)->mappos;
        context->precommand = next_precommand; // restore so each precommand and a command will run again each turn of a for
        call_chain(context);
    }
}

static void check_location(struct ParserContext *context, const struct ScriptLine *scline)
{
    int idx = gameadd.script.free_precommand++;
    struct PreCommand *cmd = &gameadd.script.precommands[idx];
    cmd->precommand_idx = PCI_LOCATION;
    cmd->np[0] = scline->np[0];
    strcpy(context->outer_scline->tp[context->dst_idx], "_PRECOMMAND_");

    SCRIPT_LIST_ADD(context->precommands)->precommand = idx;
}

/// ********

static void whine(struct ScriptContext *context, struct PreCommand *cmd, struct TriggerCommonData *dst)
{
    WARNMSG("Invalid precommand");
}

/// ********

const struct CommandDesc advanced_subcommands[] = {
        {"RANDOM",   "Aaaaaa", 0, &check_random, NULL},
        {"LOCATION", "G", 0, &check_location, NULL},
        {NULL,       "",       0, NULL, NULL}
};

const PreCommandFn pre_commands[] = {
        whine,
        cmd_location
};
/// ********

TbBool process_advanced_command(struct ParserContext *context, char **line, const struct CommandDesc *command)
{
    if (command == NULL)
    {
        return false;
    }
    int required = count_required_parameters(command->args);

    struct ScriptLine *outer_scline = context->outer_scline;
    struct ScriptLine new_line = { 0 };
    context->outer_scline = context->scline;
    context->scline = &new_line;
    int args_count = script_recognize_params(context, line, command, 0, 0);
    if (args_count < 0)
    {
        context->scline = context->outer_scline;
        context->outer_scline = outer_scline;
        return false;
    }
    if (args_count < COMMANDDESC_ARGS_COUNT)
    {
        if (args_count < required)
        {
            SCRPTERRLOG("Not enough parameters for \"%s\", got only %d", command->textptr, args_count);
            context->scline = context->outer_scline;
            context->outer_scline = outer_scline;
            return false;
        }
    }
    command->check_fn(context, &new_line);
    context->scline = context->outer_scline;
    context->outer_scline = outer_scline;
    return true;
}
/// ********

/*
 * Call next precommands and finally a command
 */
static void call_chain(struct ScriptContext *context)
{
    if (context->precommand == 0)
    {
        context->command_fn(context);
        return;
    }
    struct ThingListRecord *cmd_item = &gameadd.script.list_records[context->precommand];
    struct PreCommand *cmd = &gameadd.script.precommands[cmd_item->precommand];

    if (cmd->precommand_idx >= STATIC_SIZE(pre_commands))
    {
        ERRORLOG("Invalid command index!");
        return;
    }

    context->precommand = cmd_item->next_record;
    PreCommandFn fn = pre_commands[cmd->precommand_idx];
    fn(context, cmd, context->common);
}

void process_precommands(struct ScriptContext *context, struct TriggerCommonData *trigger, CommandFn command_fn)
{
    context->command_fn = command_fn;
    context->precommand = trigger->precommands;
    call_chain(context);
}
