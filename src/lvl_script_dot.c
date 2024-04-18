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

#include "game_merge.h"
#include "player_instances.h"
#include "post_inc.h"

static TbBool parse_args(struct ParserContext *context, struct ScriptLine *scline)
{
    struct CommandDesc cmd_desc;
    struct CommandToken token;
    memcpy(&cmd_desc.args, context->current_command->args, sizeof(cmd_desc.args));

    context->line = get_next_token(context->line, &token);

    if (token.type != TkOpen)
    {
        SCRPTERRLOG("Invalid token at %s", context->line);
        return false;
    }

    int para_level = 0;
    int args_count = script_recognize_params(&context->line, &cmd_desc, scline, &para_level, 0, context->file_version);
    if (args_count < 0)
    {
        return false;
    }
    if (args_count < COMMANDDESC_ARGS_COUNT)
    {
        int required = count_required_parameters(cmd_desc.args);
        if (args_count < required) // Required arguments have upper-case type letters
        {
            SCRPTERRLOG("Not enough parameters for \"%s\", got only %d", context->current_command->text,
                        args_count);
            return false;
        }
    }
    return true;
}

/// ********
struct DotCommand *dot_last(DotCommandIdx idx)
{
    static struct DotCommand bad;
    struct DotCommand *cmd = &gameadd.script.dot_commands[idx];
    if ((cmd == NULL) || (idx == 0))
        return &bad;
    for (;cmd->chain_next;cmd = &gameadd.script.dot_commands[cmd->chain_next])
    {
        // empty
    }
    return cmd;
}

static struct DotCommand *dot_alloc()
{// Allocating
    DotCommandIdx idx = gameadd.script.free_dot_command;
    struct DotCommand *cmd = &gameadd.script.dot_commands[idx];
    if (gameadd.script.free_dot_command == 0)
    {
        idx = gameadd.script.dot_commands_num;
        gameadd.script.dot_commands_num++;
        cmd = &gameadd.script.dot_commands[idx];
    }
    else
    {
        gameadd.script.free_dot_command = cmd->chain_next;
    }
    cmd->chain_next = 0;
    // add to condition if any
    int condition = get_script_current_condition();
    if (condition == CONDITION_ALWAYS)
    {
        //TODO: what if next_command_reusable != 0
    }
    else
    {
        struct Condition* condt = &gameadd.script.conditions[condition];
        if (condt->dot_to_activate)
        {
            dot_last(condt->dot_to_activate)->chain_next = idx;
        }

    }
    return cmd;
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

static TbBool cmd_set_player(struct ParserContext *context, intptr_t option)
{
    context->active_player = option;
    context->player_is_set = true;
    context->dot_commands = player_dot_commands;
    context->construct_fn = &whine;
    return true;
}

static TbBool make_find_creatures(struct ParserContext *context)
{
    struct DotCommand *cmd = dot_alloc();
    cmd->active_player = context->active_player;
    return true;
}

static TbBool pl_find_creatures(struct ParserContext *context, intptr_t option)
{
    struct ScriptLine scline = {0};

    if (!parse_args(context, &scline))
        return false;

    if ((context->fn_type != CtUnused) && (context->fn_type != CtCreature))
    {
        // TODO: construct next item
    }

    context->location_is_set = true;
    context->location = scline.np[0];
    context->fn_type = CtCreature;
    context->construct_fn = &make_find_creatures;
    // If chain is stopped here we should construct creature group
    return true;
}

static TbBool cre_cmd_location(struct ParserContext *context, intptr_t option)
{
    if ((context->fn_type != CtUnused) && (context->fn_type != CtLocation))
    {
        // TODO: construct next item

    }
    context->dot_commands = location_dot_commands;
    // If chain is stopped here we should construct location
    return true;
}

static TbBool loc_cmd_create_effect(struct ParserContext *context, intptr_t option)
{
    struct ScriptLine scline = {0};

    if (!parse_args(context, &scline))
        return false;

    // If chain is stopped here we should keep location
    return true;
}

#define SET_PLAYER_CMD(P) \
    {"" #P ,          P, "", cmd_set_player}

const struct DotCommandDesc main_dot_commands[] = {
        SET_PLAYER_CMD (PLAYER0),
        SET_PLAYER_CMD (PLAYER1),
        SET_PLAYER_CMD (PLAYER2),
        SET_PLAYER_CMD (PLAYER3),
        SET_PLAYER_CMD (PLAYER_GOOD),
        SET_PLAYER_CMD (PLAYER_NEUTRAL),
        SET_PLAYER_CMD (PLAYER4),
        SET_PLAYER_CMD (PLAYER5),
        SET_PLAYER_CMD (PLAYER6),
        {NULL, 0, "", 0}
};

const struct DotCommandDesc player_dot_commands[] = {
        {"FIND_CREATURES", 0, "L", pl_find_creatures},
        {NULL,             0, "",  0}
};

const struct DotCommandDesc creature_list_dot_commands[] = {
        {"LOCATION", 0, "", cre_cmd_location},
        {NULL,       0, "", 0}
};

const struct DotCommandDesc location_dot_commands[] = {
        {"CREATE_EFFECT", 0, "A", loc_cmd_create_effect},
        {NULL,            0, "",  0}
};