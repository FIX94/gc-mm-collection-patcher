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
	ar_get_dma_status = 0x80079298;

	ax_acquire_voice = 0x8007BF38;
	ax_set_voice_addr = 0x8007EE34;
	ax_set_voice_src_type = 0x8007E744;
	ax_set_voice_state = 0x8007E808;
	ax_free_voice = 0x8007BEB8;

	mix_init_channel = 0x8007F7EC;

	os_disable_interrupts = 0x8005DD04;
	os_restore_interrupts = 0x8005DD2C;

	os_report = 0x8005A110;
	os_create_thread = 0x80062394;
	os_resume_thread = 0x800629FC;
	os_suspend_thread = 0x80062C84;
	os_is_thread_suspended = 0x80061C54;
	os_join_thread = 0x8006281C;

	dc_flush_range = 0x80058D00;
	ic_inv_range = 0x80058E10;

	/* game functions we call */
	sfx_registercallback = 0x8003C8AC;

	gc_aram_malloc = 0x8003BCF0;
	gc_aram_upload = 0x8003BD88;

	bink_mmu_malloc = 0x8003F19C;
	mmu_free = 0x80038954;
	mmu_memset = 0x80038AD4;

	/* game function exit addresses we patch */
	module_init_exit_addr = 0x8001437C;
	module_uninit_exit_addr = 0x800145DC;

	packfile_transfer_exit_addr = 0x80027C2C;

	/* game vars we use */
	game_ax_voice_array = 0x800DCCD4;

	game_amp_val = 0x80109CC4;

	game_loaded_module = 0x80187AD8;

	game_gc_aram_busy = 0x80187D80;

	.code 0x800CED60 :
	{
		start.o(*)
		*(.text)
		*(.rodata .rodata.*)
		*(.data)
		*(.bss)
		. = 0xA630;
	}
}
