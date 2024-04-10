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

struct ExecContext {
    SListRecordIdx item;
    enum FunctionType type;
};

enum DotCmd {
    DF_INVALID = 0,
    DF_FIND_CREATURE,
    DF_CREATURE_TO_LOC,
    DF_CREATE_EFFECT,
    DF_READ_GROUP,
    DF_CRE_CAST_POWER,
};

typedef void (*DotProcessFn)(struct DotCommand *, struct ExecContext *);
/// ********

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
    for (;cmd->chain_next;cmd = &gameadd.script.dot_commands[cmd->chain_next])
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
        struct Condition* condt = &gameadd.script.conditions[condition];
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

static TbBool cmd_set_player(struct ParserContext *context, intptr_t option)
{
    context->active_player = option;
    context->player_is_set = true;
    context->fn_type = CtPlayer;
    context->dot_commands = player_dot_commands;
    context->construct_fn = &whine;
    return true;
}

/// ********

/// find_creatures
static long add_creatures_diagonal(const struct Thing *thing, MaxTngFilterParam param, long maximizer)
{
    struct ActionPoint* apt = param->ptr1;
    if (thing->class_id == TCls_Creature)
    {
        if ((param->plyr_idx == ALL_PLAYERS) || (thing->owner == param->plyr_idx))
        {
            if (creature_matches_model(thing, param->model_id))
            {
                if (!thing_is_picked_up(thing))
                {
                    MapCoordDelta dist = get_distance_xy(thing->mappos.x.val, thing->mappos.y.val, apt->mappos.x.val, apt->mappos.y.val);
                    if (dist > apt->range) // Too far away
                        return -1;
                    // Just add to a list and wait for another creature
                    gameadd.script.list_records[param->num2].thing = thing->index;
                    param->num2 = script_list_add(param->num2); // Adding empty element ahead
                    return -1;
                }
            }
        }
    }
    // If conditions are not met, return -1 to be sure thing will not be returned.
    return -1;
}

static void find_creatures_process(struct DotCommand *cmd, struct ExecContext *context)
{
    SListRecordIdx lst = script_list_add(0);
    gameadd.script.list_records[lst].thing = 0;

    int loc = filter_criteria_loc(cmd->location);
    struct ActionPoint* apt = action_point_get(loc);
    if (!action_point_exists(apt))
    {
        WARNLOG("Action point is invalid:%d", apt->num);
        return;
    }
    if (apt->range == 0)
    {
        WARNLOG("Action point with zero range:%d", apt->num);
        return;
    }

    int dist = 2 * coord_subtile(apt->range + COORD_PER_STL - 1) + 1;
    dist = dist * dist;

    Thing_Maximizer_Filter filter = add_creatures_diagonal;
    struct CompoundTngFilterParam param;
    param.model_id = cmd->model;
    param.plyr_idx = (unsigned char)cmd->active_player;
    param.ptr1 = apt;
    param.num2 = lst;
    get_thing_spiral_near_map_block_with_filter(apt->mappos.x.val, apt->mappos.y.val,
                                                       dist,
                                                       filter, &param);
    if (gameadd.script.list_records[lst].thing == 0)
    {
        script_list_free(lst);
        lst = 0;
    }
    else
    {
        script_list_pop(&lst); // last item should be empty one
    }

    if (cmd->assign_to)
    {
        script_list_free(gameadd.script.groups[cmd->assign_to]);
        gameadd.script.groups[cmd->assign_to] = lst;
    }
}

static TbBool make_find_creatures(struct ParserContext *context)
{
    DOT_ALLOC
    add_to_condition_sublist(context, cmd_idx);
    cmd->active_player = context->active_player;
    cmd->location = context->location;
    cmd->model = context->creature;
    cmd->command_fn = DF_FIND_CREATURE;
    if (context->is_assign)
    {
        cmd->assign_to = context->active_group->id;
    }
    return true;
}

static TbBool pl_find_creatures(struct ParserContext *context, intptr_t option)
{
    struct ScriptLine scline = {0};

    if (!parse_args(context, &scline))
        return false;

    if ((context->fn_type != CtPlayer) && (context->fn_type != CtCreature))
    {
        // TODO: construct next item
    }
    if (scline.np[1] == 0)
        scline.np[1] = CREATURE_ANY;

    context->location_is_set = true;
    context->location = scline.np[0];
    if (context->location == -1)
    {
        SCRPTERRLOG("Invalid ap number: %d", scline.np[0]);
        return false;
    }
    context->creature = scline.np[1];
    context->fn_type = CtCreature;
    // This is used because I expect filters after this one
    context->construct_fn = &make_find_creatures;
    // If chain is stopped here we should construct creature group
    return true;
}

/// ********

static void creature_to_loc_process(struct DotCommand *cmd, struct ExecContext *context)
{
    // I don't store locations for now
}

static TbBool cre_cmd_location(struct ParserContext *context, intptr_t option)
{
    if (context->fn_type != CtCreature)
    {
        SCRPTERRLOG("Unexpected!");
    }
    context->dot_commands = location_dot_commands;
    context->fn_type = CtLocation;

    //DOT_ALLOC
    //add_to_condition_sublist(cmd_idx);
    //cmd->command_fn = DF_CREATURE_TO_LOC;
    
    // If chain is stopped here we should construct location into var (but we will whine instead)
    context->construct_fn = &whine;
    return true;
}

/// ********

static void cre_cast_power_process(struct DotCommand *cmd, struct ExecContext *context)
{
    SListRecordIdx idx = context->item;
    struct ThingListRecord *item;
    for (; idx != 0; idx = item->next_record)
    {
        item = &gameadd.script.list_records[idx];
        struct Thing *thing = thing_get(item->thing);
        if (thing->alloc_flags & TAlF_IsInLimbo)
            continue;
        script_use_power_on_creature(thing, (short)cmd->arg1, (short)cmd->arg2, cmd->active_player, true);
    }
}

static TbBool cre_cast_power(struct ParserContext *context, intptr_t option)
{
    struct ScriptLine scline = {0};

    if (!parse_args(context, &scline))
        return false;
    long power = get_id(power_desc, scline.tp[0]);
    if (power == -1)
    {
        SCRPTERRLOG("Unknown magic, '%s'", scline.tp[0]);
        return false;
    }
    DOT_ALLOC
    add_to_condition_sublist(context, cmd_idx);
    cmd->active_player = context->active_player;
    cmd->model = power;
    cmd->arg2 = scline.np[1];
    cmd->command_fn = DF_CRE_CAST_POWER;

    context->construct_fn = NULL;
    // If chain is stopped here we should keep location
    return true;
}

/// ********

static void create_effect_process(struct DotCommand *cmd, struct ExecContext *context)
{
    SListRecordIdx idx = context->item;
    struct ThingListRecord *item;
    for (; idx != 0; idx = item->next_record)
    {
        item = &gameadd.script.list_records[idx];
        struct Thing *thing = thing_get(item->thing);
        struct Coord3d pos;
        
        set_coords_to_subtile_center(&pos, thing->mappos.x.stl.num,  thing->mappos.y.stl.num, 0);
        pos.z.val += get_floor_height(pos.x.stl.num, pos.y.stl.num);
        TbBool Price = (cmd->model == -(TngEffElm_Price));
        if (Price)
        {
            pos.z.val += 128;
        }
        struct Thing* efftng = create_used_effect_or_element(&pos, cmd->model, game.neutral_player_num);
        if (!thing_is_invalid(efftng))
        {
            if (thing_in_wall_at(efftng, &efftng->mappos))
            {
                move_creature_to_nearest_valid_position(efftng);
            }
            if (Price)
            {
                efftng->price_effect.number = cmd->arg2;
            }
        }
    }
}

static TbBool loc_cmd_create_effect(struct ParserContext *context, intptr_t option)
{
    struct ScriptLine scline = {0};

    if (!parse_args(context, &scline))
        return false;

    DOT_ALLOC
    add_to_condition_sublist(context, cmd_idx);
    cmd->active_player = context->active_player;
    cmd->model = scline.np[0];
    cmd->arg2 = scline.np[1];
    cmd->command_fn = DF_CREATE_EFFECT;
    
    context->construct_fn = NULL;
    // If chain is stopped here we should keep location
    return true;
}

/// ********

static void read_group_process(struct DotCommand *cmd, struct ExecContext *context)
{
    if (cmd->arg1 >= GROUPS_COUNT)
    {
        ERRORLOG("Invalid group:%d !", cmd->arg1);
        return;
    }
    context->item = gameadd.script.groups[cmd->arg1];
}

TbBool make_read_group(struct ParserContext *context)
{
    DOT_ALLOC
    add_to_condition_sublist(context, cmd_idx);

    cmd->command_fn = DF_READ_GROUP;
    cmd->arg1 = context->active_group->id;
    context->construct_fn = &whine;

    // Only creatures are supported for now
    context->fn_type = CtCreature;
    context->dot_commands = creature_list_dot_commands;

    return true;
}

/// ********

static void invalid_dot_process(struct DotCommand *cmd, struct ExecContext *context)
{
    ERRORLOG("Invalid dot process!");
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
        {"FIND_CREATURES", 0, "Lc", pl_find_creatures},
        {NULL,             0, "",  0}
};

const struct DotCommandDesc creature_list_dot_commands[] = {
        {"LOCATION", 0, "", cre_cmd_location},
        {"CAST_POWER", 0, "An", cre_cast_power},
        {NULL,       0, "", 0}
};

const struct DotCommandDesc location_dot_commands[] = {
        {"CREATE_EFFECT", 0, "Na", loc_cmd_create_effect},
        {NULL,            0, "",  0}
};

/// ********

static DotProcessFn dot_process_fns[] = {
        &invalid_dot_process,
        &find_creatures_process,
        &creature_to_loc_process,
        &create_effect_process,
        &read_group_process,
        &cre_cast_power_process,
};

void process_dot_script(SListRecordIdx cmd_idx)
{
    struct ThingListRecord *cmd_item;
    for (;cmd_idx != 0;cmd_idx = cmd_item->next_record)
    {
        cmd_item = &gameadd.script.list_records[cmd_idx];
        struct DotCommand *cmd = &gameadd.script.dot_commands[cmd_item->dot_command];
        struct ExecContext context = {0}; //One context per chain

        for (;;cmd = &gameadd.script.dot_commands[cmd->chain_next])
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