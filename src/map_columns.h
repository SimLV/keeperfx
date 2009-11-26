/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file map_columns.h
 *     Header file for map_columns.c.
 * @par Purpose:
 *     Column and columns array data management functions.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     27 Oct 2009 - 09 Nov 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_MAPCOLUMN_H
#define DK_MAPCOLUMN_H

#include "globals.h"
#include "bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
#define COLUMNS_COUNT        2048
#define COLUMN_STACK_HEIGHT     8
/******************************************************************************/
#pragma pack(1)

struct Column { // sizeof=0x18
    short use;
    unsigned char bitfileds;
    unsigned short solidmask;
    unsigned short baseblock;
    unsigned char orient;
    unsigned short cubes[COLUMN_STACK_HEIGHT];
};

#pragma pack()
/******************************************************************************/
/******************************************************************************/
struct Column *get_column_at(long slb_x, long slb_y);
struct Column *get_map_column(struct Map *map);

long get_top_cube_at_pos(long mpos);
long get_top_cube_at(long slb_x, long slb_y);
void make_solidmask(struct Column *col);
void clear_columns(void);
void init_columns(void);
long find_column(struct Column *col);
long create_column(struct Column *col);
unsigned short find_column_height(struct Column *col);
void init_whole_blocks(void);
void init_top_texture_to_cube_table(void);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
