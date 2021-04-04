/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file engine_render_data.cpp
 *     Colour arrays for drawing stripey lines.
 * @par Purpose:
 *     Provides a set of colours to use when drawing a stripey line, e.g. for a bounding box. Used by draw_stripey_line() in engine_render.c.
 * @par Comment:
 *     None.
 * @author   Ed Kearney
 * @date     07 Sep 2020 - 11 Sep 2020
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "engine_render.h"

#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

// uses a color of the given ID from the palette: MAIN.PAL
// See https://github.com/dkfans/keeperfx/pull/811#issuecomment-688918505 for more instructions on how to add colours
struct stripey_line      basic_stripey_line = { { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00 },              0 }; // example
struct stripey_line     basic_stripey_line2 = { { 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x0f, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x08 },              0 }; // example
struct stripey_line        red_stripey_line = { { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x47, 0x47, 0x46, 0x45, 0x44, 0x43, 0x42, 0x41, 0x40, 0x40 },        SLC_RED };
struct stripey_line      green_stripey_line = { { 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa7, 0xa7, 0xa6, 0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0, 0xa0 },      SLC_GREEN };
struct stripey_line     yellow_stripey_line = { { 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb7, 0xb7, 0xb6, 0xb5, 0xb4, 0xb3, 0xb2, 0xb1, 0xb0, 0xb0 },     SLC_YELLOW };
struct stripey_line      brown_stripey_line = { { 0x04, 0x09, 0x0b, 0x0c, 0x0d, 0x0d, 0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00 },      SLC_BROWN };
struct stripey_line       grey_stripey_line = { { 0x09, 0x0b, 0x0c, 0x0d, 0x0e, 0x7f, 0x7f, 0x7f, 0x0e, 0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x03, 0x03 },       SLC_GREY };
struct stripey_line     purple_stripey_line = { { 0x69, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x6f, 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x69, 0x67, 0x62, 0x62 },     SLC_PURPLE };
struct stripey_line       blue_stripey_line = { { 0x58, 0x59, 0x5b, 0x5c, 0x5d, 0x5e, 0x5e, 0x5e, 0x5d, 0x5c, 0x5b, 0x59, 0x58, 0x53, 0x51, 0x51 },       SLC_BLUE };
struct stripey_line     orange_stripey_line = { { 0x26, 0x28, 0x2a, 0x86, 0x87, 0x88, 0x88, 0x88, 0x87, 0x86, 0x2a, 0x28, 0x26, 0x23, 0x21, 0x21 },     SLC_ORANGE };
struct stripey_line      white_stripey_line = { { 0x13, 0x15, 0x19, 0x1a, 0x1b, 0x1c, 0x1c, 0x1c, 0x1b, 0x1a, 0x19, 0x15, 0x13, 0x12, 0x08, 0x08 },      SLC_WHITE };


struct stripey_line colored_stripey_lines[STRIPEY_LINE_COLOR_COUNT] = { 
    red_stripey_line,
    green_stripey_line,
    yellow_stripey_line,
    brown_stripey_line,
    grey_stripey_line,
    purple_stripey_line,
    blue_stripey_line,
    orange_stripey_line,
    white_stripey_line,
};
/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
