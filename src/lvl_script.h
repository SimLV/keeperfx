/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file lvl_script.h
 *     Header file for lvl_script.c.
 * @par Purpose:
 *     Level script commands support.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   KeeperFX Team
 * @date     12 Feb 2009 - 24 Feb 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_LVLSCRIPT_H
#define DK_LVLSCRIPT_H

#include "globals.h"
#include "bflib_basics.h"

#include "config.h"
#include "config_rules.h"
#include "creature_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#define PARTY_TRIGGERS_COUNT     256
#define CREATURE_PARTYS_COUNT    256
#define CONDITIONS_COUNT         512
#define TUNNELLER_TRIGGERS_COUNT 256
#define SCRIPT_VALUES_COUNT      2048
#define WIN_CONDITIONS_COUNT     12
#define DOT_COMMANDS_COUNT       256
#define GROUPS_COUNT             8
#define SLIST_RECORDS_COUNT      256

#define CONDITION_ALWAYS (CONDITIONS_COUNT)

#define SENSIBLE_GOLD 99999999

enum ScriptOperator {
    SOpr_SET = 1,
    SOpr_INCREASE,
    SOpr_DECREASE,
    SOpr_MULTIPLY,
};

enum {
    CurrentPlayer = 15
};

/******************************************************************************/
#pragma pack(1)

typedef int DotCommandIdx;
struct Condition;
struct ScriptLine;
struct ScriptValue;
struct PartyTrigger;

struct ScriptContext
{
    int plr_start;
    int plr_end;
    int player_idx;

    union {
      struct ScriptValue *value;
      struct PartyTrigger *pr_trig;
    };
    SListRecordIdx save_group;
};

struct TunnellerTrigger {
  unsigned char flags;
  unsigned short condit_idx;
  unsigned char plyr_idx;
  unsigned long location;
  unsigned long heading; // originally was 'target'
  long carried_gold;
  unsigned char crtr_level;
  char party_id;
  SListRecordIdx target_group;
};

struct PartyTrigger {
  unsigned char flags;
  unsigned short condit_idx;
  char creatr_id;
  union
  {
      unsigned char plyr_idx;
      char party_id; // for add_to_party
  };
  union
  {
      unsigned long location;
      unsigned long countdown;
  };
  unsigned char crtr_level;
  unsigned short carried_gold;
  union
  {
      unsigned short ncopies;
      unsigned char objectv;
  };
  SListRecordIdx target_group;
};

struct ScriptValue {
  unsigned char flags;
  unsigned short condit_idx;
  unsigned char valtype;
  unsigned char plyr_range;
  union
  {
    struct
    {
      union
      {
          long arg0;
          char* str0;
      };
      union
      {
          long arg1;
          char* str1;
      };
      union
      {
          long arg2;
          char* str2;
      };
    };
    struct
    {
      union
      {
          unsigned long uarg0;
          unsigned char* ustr0;
      };
      union
      {
          unsigned long uarg1;
          unsigned char* ustr1;
      };
      union
      {
          unsigned long uarg2;
          unsigned char* ustr2;
      };
    };
    struct
    {
        char action;
        char param;
        char victims[MAX_SACRIFICE_VICTIMS];
    } sac;
    unsigned char bytes[16];
    char chars[16];
    short shorts[8];
  };
};

struct Condition {
  short condit_idx;
  unsigned char status;
  unsigned char plyr_range;
  unsigned char variabl_type;
  unsigned short variabl_idx;
  unsigned char operation;
  unsigned long rvalue;
  unsigned char plyr_range_right;
  unsigned char variabl_type_right;
  unsigned short variabl_idx_right;
  SListRecordIdx dotlist_to_activate;
  TbBool use_second_variable;
};


struct ScriptFxLine
{
    int used;
    struct Coord3d from;
    struct Coord3d here;
    struct Coord3d to;

    int cx, cy; // midpoint

    int curvature;
    int spatial_step;
    int steps_per_turn;
    int partial_steps;
    int effect;

    int total_steps;
    int step;
};

struct ThingListRecord
{
    SListRecordIdx next_record;
    union
    {
        ThingIndex thing;
        DotCommandIdx dot_command;
    };
};

struct LevelScript {
    struct TunnellerTrigger tunneller_triggers[TUNNELLER_TRIGGERS_COUNT];
    unsigned long tunneller_triggers_num;
    struct PartyTrigger party_triggers[PARTY_TRIGGERS_COUNT];
    unsigned long party_triggers_num;
    struct ScriptValue values[SCRIPT_VALUES_COUNT];
    unsigned long values_num;
    struct Condition conditions[CONDITIONS_COUNT];
    unsigned long conditions_num;
    struct Party creature_partys[CREATURE_PARTYS_COUNT];
    unsigned long creature_partys_num;
    unsigned short win_conditions[WIN_CONDITIONS_COUNT];
    unsigned long win_conditions_num;
    unsigned short lose_conditions[WIN_CONDITIONS_COUNT];
    unsigned long lose_conditions_num;


    // Store strings used at level here
    char strings[2048];
    char *next_string;
    SListRecordIdx groups[GROUPS_COUNT];
    struct ThingListRecord list_records[SLIST_RECORDS_COUNT];
    SListRecordIdx free_list_record_num;
    SListRecordIdx free_list_record_idx;
};

/******************************************************************************/
extern unsigned char next_command_reusable;

#pragma pack()

#define SCRIPT_LIST_ADD(dst) \
    SCRIPT_LIST_ADD__(dst, temp_var ## __LINE__ )

#define SCRIPT_LIST_ADD__(dst, tmp_var) \
    SListRecordIdx tmp_var; \
    (tmp_var) = script_list_add(dst); \
    if ((dst) == 0)  (dst) = tmp_var;       \
    (&gameadd.script.list_records[tmp_var])

extern SListRecordIdx script_list_add(SListRecordIdx idx);
struct ThingListRecord *script_list_pop(SListRecordIdx *idx);
void script_list_free(SListRecordIdx idx);

/******************************************************************************/
#include "lvl_script_lib.h" // For struct ParserContext
/******************************************************************************/

extern const struct NamedCommand player_desc[];
/******************************************************************************/
short clear_script(void);
short load_script(long lvl_num);
TbBool script_scan_line(char *line, struct ParserContext *context);
TbBool preload_script(long lvnum);
/******************************************************************************/

void level_version_check(struct ParserContext *context, const struct ScriptLine* scline);
long get_condition_value(PlayerNumber plyr_idx, unsigned char valtype, unsigned char a3);
void process_level_script(void);

TbBool make_read_group(struct ParserContext *context);
void process_dot_script(SListRecordIdx cmd);
void script_add_creature_to_result(struct ScriptContext *context, struct Thing *thing);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
