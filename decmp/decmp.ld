/*
 * Copyright (C) 2019 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)

SECTIONS {
	/* sdk functions we call */
	dc_flush_range = 0x80058D00;
	ic_inv_range = 0x80058E10;

	/* std lib functions we call */
	memmove = 0x800B4C1C;
	memcmp = 0x8004FCB0;
	__ashldi3 = 0x80054EDC;

	/* game functions we call */
	bink_mmu_malloc = 0x8003F19C;
	mmu_free = 0x80038954;
	mmu_memset = 0x80038AD4;
	mmu_memcpy = 0x80038AF4;

	module_getmodule = 0x800145E0;

	gc_aram_malloc = 0x8003BCF0;

	/* location of compressed emu */
	emu_cmp = 0x800CB358;

	/* location of decompressed emu */
	emu_entry = 0x800CED60;

	.code 0x800BD220 :
	{
		*(.init)
		*(.text)
		*(.bss)
		*(.sbss)
		*(.data)
		*(.sdata)
		*(.rodata .rodata.*)
		. = 0x2080;
	}
}
