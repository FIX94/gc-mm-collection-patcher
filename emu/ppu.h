/*
 * Copyright (C) 2017 - 2018 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef _ppu_h_
#define _ppu_h_

#include "common.h"

void ppuInit();
void ppuAddCycles();
FIXNES_ALWAYSINLINE void ppuCycle();
FIXNES_ALWAYSINLINE bool ppuDrawDone();
uint8_t ppuGet8(uint16_t addr);
void ppuSet8(uint16_t addr, uint8_t val);
uint8_t ppuNMI();
void ppuDumpMem();
uint16_t ppuGetCurVramAddr();
void ppuSoftReset();
void ppuReset();

void ppuSetNameTblSingleLower();
void ppuSetNameTblSingleUpper();
void ppuSetNameTblVertical();
void ppuSetNameTblHorizontal();
void ppuSetNameTbl4Screen();
void ppuSetNameTblCustom(uint16_t addrA, uint16_t addrB, uint16_t addrC, uint16_t addrD);

void ppuDrawNSFTrackNum(uint8_t cTrack, uint8_t trackTotal);

uint8_t ppuGetVRAM0(uint16_t addr);
uint8_t ppuGetVRAM1(uint16_t addr);
uint8_t ppuGetVRAM2(uint16_t addr);
uint8_t ppuGetVRAM3(uint16_t addr);
void ppuSetVRAM0(uint16_t addr, uint8_t val);
void ppuSetVRAM1(uint16_t addr, uint8_t val);
void ppuSetVRAM2(uint16_t addr, uint8_t val);
void ppuSetVRAM3(uint16_t addr, uint8_t val);

uint8_t ppuGetPALRAM(uint16_t addr);
void ppuSetPALRAM(uint16_t addr, uint8_t val);
void ppuSetPALRAMMirror(uint16_t addr, uint8_t val);

extern bool ppu4Screen;

//for initial NT set in main.c
enum {
	NT_UNKNOWN = 0,
	NT_HORIZONTAL,
	NT_VERTICAL,
	NT_4SCREEN,
};

#endif
