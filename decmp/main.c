/*
 * Copyright (C) 2019 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "xz.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

typedef struct _emu_cmp_struct {
	int size;
	uint8_t data[];
} emu_cmp_struct;

static u32 aram_addr = 0;

int module_getmodule();
void mmu_memset(void *buf, int val, int size);

void dc_flush_range(void *buf, int size);
void ic_inv_range(void *buf, int size);

extern emu_cmp_struct emu_cmp;

void emu_entry(int module, u32 aram_addr);
#define EMU_DATA_SPACE 0xA630

int _main()
{
	int module = module_getmodule();
	//if this is MM1 (ID 0) up to MM6 (ID 5)
	if(module >= 0 && module <= 5)
	{
		struct xz_dec *decStr = xz_dec_init(XZ_SINGLE, 0);
		struct xz_buf b;
		b.in = emu_cmp.data;
		b.in_pos = 0;
		b.in_size = emu_cmp.size;
		b.out = (void*)emu_entry;
		b.out_pos = 0;
		b.out_size = EMU_DATA_SPACE;
		xz_dec_run(decStr, &b);
		xz_dec_end(decStr);
		dc_flush_range((void*)emu_entry, EMU_DATA_SPACE);
		ic_inv_range((void*)emu_entry, EMU_DATA_SPACE);
		emu_entry(module, aram_addr);
	}
	else //other module, clear space for videos
	{
		mmu_memset((void*)emu_entry, 0, EMU_DATA_SPACE);
		dc_flush_range((void*)emu_entry, EMU_DATA_SPACE);
	}
	return module;
}

u32 gc_aram_malloc(u32 size);
void _gc_aram_init_hook()
{
	//get our space for the NES driver right on boot
	aram_addr = gc_aram_malloc(0x500);
}
