/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2006 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef SMPC_H
#define SMPC_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REGION_AUTODETECT               0
#define REGION_JAPAN                    1
#define REGION_ASIANTSC                 2
#define REGION_NORTHAMERICA             4
#define REGION_CENTRALSOUTHAMERICANTSC  5
#define REGION_KOREA                    6
#define REGION_ASIAPAL                  10
#define REGION_EUROPE                   12
#define REGION_CENTRALSOUTHAMERICAPAL   13

enum
{
   LANGUAGE_ENGLISH = 0,
   LANGUAGE_GERMAN = 1,
   LANGUAGE_FRENCH = 2,
   LANGUAGE_SPANISH = 3,
   LANGUAGE_ITALIAN = 4,
   LANGUAGE_JAPANESE = 5,
};

typedef struct {
   u8 IREG[7];
   u8 padding[8];
   u8 COMREG;
   u8 OREG[32];
   u8 SR;
   u8 SF;
   u8 padding2[8];
   u8 PDR[2];
   u8 DDR[2];
   u8 IOSEL;
   u8 EXLE;
} Smpc;

extern Smpc * SmpcRegs;

typedef struct 
{
   int offset;
   int size;
   u8 data[256];
} PortData_struct;

typedef struct {
   u8 dotsel; // 0 -> 320 | 1 -> 352
   u8 mshnmi;
   u8 sndres;
   u8 cdres;
   u8 sysres;
   u8 resb;
   u8 ste;
   u8 resd;
   u8 intback;
   u8 intbackIreg0;
   u8 firstPeri;
   u8 regionid;
   u8 regionsetting;
   u8 languageid;
   u8 SMEM[4];
   s32 timing;
   PortData_struct port1;
   PortData_struct port2;
   u8 clocksync;
   u32 basetime;  // Safe until early 2106.  After that you're on your own (:
} SmpcInternal;

extern SmpcInternal * SmpcInternalVars;

int SmpcInit(u8 regionid, int clocksync, u32 basetime, const char *smpcpath, u8 languageid);
void SmpcDeInit(void);
void SmpcRecheckRegion(void);
int SmpcGetLanguage(void);
void SmpcReset(void);
void SmpcResetButton(void);
void SmpcExec(s32 t);
void SmpcINTBACKEnd(void);
void SmpcCKCHG320(void);
void SmpcCKCHG352(void);

u8 FASTCALL	SmpcReadByte(SH2_struct *context, u8*, u32);
u16 FASTCALL	SmpcReadWord(SH2_struct *context, u8*, u32);
u32 FASTCALL	SmpcReadLong(SH2_struct *context, u8*, u32);
void FASTCALL	SmpcWriteByte(SH2_struct *context, u8*, u32, u8);
void FASTCALL	SmpcWriteWord(SH2_struct *context, u8*, u32, u16);
void FASTCALL	SmpcWriteLong(SH2_struct *context, u8*, u32, u32);

int SmpcSaveState(void ** stream);
int SmpcLoadState(const void * stream, int version, int size);

#ifdef __cplusplus
}
#endif

#endif
