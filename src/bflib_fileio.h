/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file bflib_fileio.h
 *     Header file for bflib_fileio.c.
 * @par Purpose:
 *     File handling routines wrapper.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     10 Feb 2008 - 30 Dec 2008
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef BFLIB_FILEIO_H
#define BFLIB_FILEIO_H

#include "bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

#pragma pack(1)

enum TbFileMode {
        Lb_FILE_MODE_NEW       = 0,
        Lb_FILE_MODE_OLD       = 1,
        Lb_FILE_MODE_READ_ONLY = 2,
};

enum TbFileSeekMode {
        Lb_FILE_SEEK_BEGINNING = 0,
        Lb_FILE_SEEK_CURRENT   = 1,
        Lb_FILE_SEEK_END       = 2,
};

struct TbDriveInfo {
        unsigned long TotalClusters;
        unsigned long FreeClusters;
        unsigned long SectorsPerCluster;
        unsigned long BytesPerSector;
};

#pragma pack()

/******************************************************************************/

int __fastcall LbDriveCurrent(unsigned int *drive);
int __fastcall LbDriveChange(const unsigned int drive);
int __fastcall LbDriveExists(const unsigned int drive);
int __fastcall LbDirectoryChange(const char *path);
int __fastcall LbDriveFreeSpace(const unsigned int drive, struct TbDriveInfo *drvinfo);
bool __fastcall LbFileExists(const char *fname);
int __fastcall LbFilePosition(TbFileHandle handle);
TbFileHandle __fastcall LbFileOpen(const char *fname, unsigned char accmode);
int __fastcall LbFileClose(TbFileHandle handle);
int __fastcall LbFileSeek(TbFileHandle handle, unsigned long offset, unsigned char origin);
int __fastcall LbFileRead(TbFileHandle handle, void *buffer, unsigned long len);
long __fastcall LbFileWrite(TbFileHandle handle, const void *buffer, const unsigned long len);
long __fastcall LbFileLength(const char *fname);
long __fastcall LbFileLengthHandle(TbFileHandle handle);
int __fastcall LbFileFindFirst(const char *filespec, struct TbFileFind *ffind,unsigned int attributes);
int __fastcall LbFileFindNext(struct TbFileFind *ffind);
int __fastcall LbFileFindEnd(struct TbFileFind *ffind);
int __fastcall LbFileRename(const char *fname_old, const char *fname_new);
int __fastcall LbFileDelete(const char *filename);
short __fastcall LbFileFlush(TbFileHandle handle);
char *__fastcall LbGetCurrWorkDir(char *dest, const unsigned long maxlen);
int LbDirectoryCurrent(char *buf, unsigned long buflen);
int __fastcall LbFileMakeFullPath(const bool append_cur_dir,
  const char *directory, const char *filename, char *buf, const unsigned long len);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
