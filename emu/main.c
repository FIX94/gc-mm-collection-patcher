/*
 * Copyright (C) 2019 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include "common.h"
#include "mem.h"
#include "apu.h"
#include "cpu.h"
#include "ppu.h"
#include "input.h"

static void PatchB( void *dst, void *src )
{
	u32 newval = ((u32)dst) - ((u32)src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000000;
	*(volatile u32*)src = newval;
	dc_flush_range(src, 4);
	ic_inv_range(src, 4);
}
static void PatchBL( void *dst, void *src )
{
	u32 newval = ((u32)dst) - ((u32)src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000001;
	*(volatile u32*)src = newval;
	dc_flush_range(src, 4);
	ic_inv_range(src, 4);
}
static void PatchBLR( void *addr )
{
	*(volatile u32*)addr = 0x4E800020;
	dc_flush_range(addr, 4);
	ic_inv_range(addr, 4);
}
static void PatchNOP( void *addr )
{
	*(volatile u32*)addr = 0x60000000;
	dc_flush_range(addr, 4);
	ic_inv_range(addr, 4);
}
static void PatchByte( void *addr, u8 val )
{
	*(volatile u8*)addr = val;
	dc_flush_range(addr, 1);
	ic_inv_range(addr, 1);
}

static int cur_module = -1;

static u32 used_aram_addr = 0;
static u32 used_aram_addr_prev = 0;
static u32 used_aram_addr_cur = 0;
static u32 used_aram_addr_end = 0;

static void _module_init_hook(void *ptr1, void *ptr2);
static void _module_uninit_hook();
void _packfile_transfer_prep();

extern u32 module_init_oslink_addr;
extern u32 module_uninit_exit_addr;
extern u32 packfile_transfer_exit_addr;

void _main(int module, u32 aram_addr)
{
	//copy over current module id
	cur_module = module;
	//copy over main aram address to use
	used_aram_addr = aram_addr;
	used_aram_addr_cur = aram_addr;
	used_aram_addr_prev = aram_addr+0x280;
	used_aram_addr_end = aram_addr+0x4FF;
	// set up module patches
	PatchBL(_module_init_hook, &module_init_oslink_addr);
	PatchB(_module_uninit_hook, &module_uninit_exit_addr);
	PatchB(_packfile_transfer_prep, &packfile_transfer_exit_addr);
}


static void *our_thread = (void*)0;
static volatile int running = 0;

static u8 *used_nes_voice = (void*)0;

static void _my_sfx_callback(u32 callid)
{
	if(callid == 0xDEADBEEF && used_nes_voice && our_thread && os_is_thread_suspended(our_thread))
	{
		u32 caddr = (*(u16*)(used_nes_voice+0x1B2))<<17;
		caddr |= (*(u16*)(used_nes_voice+0x1B4))<<1;
		if (caddr < used_aram_addr_prev)
		{
			used_aram_addr_cur = used_aram_addr+0x280;
			used_aram_addr_prev = caddr;
			os_resume_thread(our_thread);
        }
        else if((caddr >= used_aram_addr+0x280) &&
            (used_aram_addr_prev < used_aram_addr+0x280))
        {
			used_aram_addr_cur = used_aram_addr;
			used_aram_addr_prev = caddr;
			os_resume_thread(our_thread);
		}
	}
}

//use functions similar to how the game does it
static void update_audio_data(u8 *data, u32 len)
{
	dc_flush_range(data, len);
	gc_aram_upload(data, used_aram_addr_cur, len, 0);
}

extern u8 game_gc_aram_busy;
static void wait_audio_data()
{
	while(game_gc_aram_busy)
	{
		if(ar_get_dma_status() == 0)
			game_gc_aram_busy = 0;
	}
}

static int doFrameUpdate()
{
	while(running)
	{
		while(!apuReady())
		{
			//main CPU clock
			cpuCycle();
			//run graphics
			ppuCycle();
			//run audio
			apuCycle();
			//mapper related irqs
			//mapperCycle();
			if(ppuDrawDone())
			{
				//play hack
				cpuActivate();
			}
		}
		update_audio_data(apuGetBuf(), apuGetCurPosBytes());
		//just resets cur pos
		apuUpdate();
		os_suspend_thread(our_thread);
	}
	return 0;
}

typedef struct _ax_voice_array {
	void *addr;
	u32 val1;
	u32 val2;
	u32 val3;
} ax_voice_array;

//from game
extern u32 *game_loaded_module;
extern ax_voice_array game_ax_voice_array[];
//from start.S
extern void *_our_fill_addr;
extern void *_ori_play_endaddr;
extern void _our_play_prep();
//our hooks
static bool _got_game_hooks = false;
static void *_our_empty_addr = (void*)0;
static void *_ori_play_patchaddr = (void*)0;

static void _mm1_mm2_fillqueue(u8 val);
static void _mm3_mm4_mm5_fillqueue(u8 val);

static void _mm1_emptyqueue(void);
static void _mm2_emptyqueue(void);
static void _mm3_mm4_mm5_emptyqueue(void);

static void _module_init_hook(void *ptr1, void *ptr2)
{
	//original instruction
	os_link(ptr1, ptr2);

	//relentry1 start 80572CCC
	u8 *relentry1 = (u8*)game_loaded_module[4];

	//swap turbo A and B
	//seemingly used in both MM1 and MM3
	PatchByte(relentry1+0x64AF7, 0x40); PatchByte(relentry1+0x64B13, 0x40);
	//seemingly used in MM2
	PatchByte(relentry1+0x10753B, 0x40); PatchByte(relentry1+0x107557, 0x40);
	//unused? I assume originally for MM3
	PatchByte(relentry1+0x1F4C1B, 0x40); PatchByte(relentry1+0x1F4C37, 0x40);
	//seemingly used in MM4
	PatchByte(relentry1+0x2D60B3, 0x40); PatchByte(relentry1+0x2D60DB, 0x40);
	//seemingly used in MM5
	PatchByte(relentry1+0x39802F, 0x40); PatchByte(relentry1+0x398057, 0x40);
	//seemingly used in MM6
	PatchByte(relentry1+0x47F507, 0x40); PatchByte(relentry1+0x47F52F, 0x40);

	//swap slide A and B
	//unused? I assume originally for MM3
	PatchByte(relentry1+0x1F4C5F, 0x80);
	//seemingly used in MM4
	PatchByte(relentry1+0x2D60FF, 0x80);
	//seemingly used in MM5
	//requires 2 patches cause of gravity mans stage
	PatchByte(relentry1+0x3980B7, 0x80); PatchByte(relentry1+0x3980CB, 0x80);
	//seemingly used in MM6
	PatchByte(relentry1+0x47F543, 0x80);

	//relentry2 start 809F5FAC
	u8 *relentry2 = (u8*)game_loaded_module[0xD];

	//MM1-MM6 swap A and B
	PatchByte(relentry2+0xD89E, 2); PatchByte(relentry2+0xD89F, 1);
	//extra patch required for MM2 to allow A
	//in weapon menu after swapping A and B
	PatchByte(relentry2+0x36E0D, 0xD);

	//actually do audio processing internally
	if(cur_module == 0) //MM1
	{
		_got_game_hooks = true;
		_our_fill_addr = _mm1_mm2_fillqueue;
		_our_empty_addr = _mm1_emptyqueue;
		_ori_play_patchaddr = relentry1+0x63B68;
		_ori_play_endaddr = _ori_play_patchaddr+0x334;
		//MM1 has hurt sound set to 0x33 for some reason, even though in
		//the original driver it should be 0x16 instead, so change it back
		PatchByte(relentry1+0x79DEF, 0x16);
	}
	else if(cur_module == 1) //MM2
	{
		_got_game_hooks = true;
		_our_fill_addr = _mm1_mm2_fillqueue;
		_our_empty_addr = _mm2_emptyqueue;
		_ori_play_patchaddr = relentry1+0xF709C;
		_ori_play_endaddr = _ori_play_patchaddr+0x348;
		//in MM2 the main theme part of the ending got a new ID of 0x18,
		//but in the original NES driver that ID is 0xD, so change it back
		PatchByte(relentry1+0x98B77, 0x0D);
	}
	else if(cur_module == 2) //MM3
	{
		_got_game_hooks = true;
		_our_fill_addr = _mm3_mm4_mm5_fillqueue;
		_our_empty_addr = _mm3_mm4_mm5_emptyqueue;
		_ori_play_patchaddr = relentry1+0x1F21C4;
		_ori_play_endaddr = _ori_play_patchaddr+0x354;
	}
	else if(cur_module == 3) //MM4
	{
		_got_game_hooks = true;
		_our_fill_addr = _mm3_mm4_mm5_fillqueue;
		_our_empty_addr = _mm3_mm4_mm5_emptyqueue;
		_ori_play_patchaddr = relentry1+0x2D2B60;
		_ori_play_endaddr = _ori_play_patchaddr+0x458;
	}
	else if(cur_module == 4) //MM5
	{
		_got_game_hooks = true;
		_our_fill_addr = _mm3_mm4_mm5_fillqueue;
		_our_empty_addr = _mm3_mm4_mm5_emptyqueue;
		_ori_play_patchaddr = relentry1+0x395420;
		_ori_play_endaddr = _ori_play_patchaddr+0x448;
		//the audio driver for MM5 is strangely messed up in different ways,
		//it likes to send more F1 commands (stop sfx) than it should,
		//these patches remove those calls when dying, because otherwise it
		//mutes the death sound before its ever even played
		PatchNOP(relentry1+0x2EC454); //taking lethal damage
		PatchNOP(relentry1+0x358EB4); //falling into pit
		//this just has the potential to mute other sounds when shooting
		PatchNOP(relentry1+0x36AECC); //shooting charge shot
		//it also sends out more F2 commands (stop music) than it should
		PatchNOP(relentry1+0x3165BC); //elevator in gyro man stage
	}
	/* todo
	else if(cur_module == 5)
	{
	} */
}

static u8 *thread_stack = (u8*)0;
static u8 *game_driver_addr_p1 = (u8*)0; //16/8k bank 0
static u8 *game_driver_addr_p2 = (u8*)0; //16/8k bank 1
static u8 *game_driver_addr_p3 = (u8*)0; //8k bank 2
static u8 *game_driver_addr_p4 = (u8*)0; //8k bank 3
//used in cpu.c
void(*doEmptyQueue)(void) = (void*)0;

static void _module_uninit_hook()
{
	if(used_nes_voice)
	{
		//clear callback and unregister voice
		sfx_registercallback((void*)0);
		u32 idx = *(u32*)(used_nes_voice+0x18);
		game_ax_voice_array[idx].addr = (void*)0;
		ax_set_voice_state(used_nes_voice, 0);
		ax_free_voice(used_nes_voice);
		used_nes_voice = (void*)0;
	}
	if(our_thread)
	{
		running = 0;
		//if it still is doing one sound block
		while(!os_is_thread_suspended(our_thread)) ;
		//done with that, resume it with running set to 0 to quit out
		os_resume_thread(our_thread);
		//join it up now finally
		os_join_thread(our_thread, (void*)0);
		mmu_free(our_thread);
		our_thread = (void*)0;
	}
	if(thread_stack)
	{
		mmu_free(thread_stack);
		thread_stack = (void*)0;
	}
	apuDeinitBufs();
	memDeinitBufs();
	game_driver_addr_p1 = (void*)0;
	game_driver_addr_p2 = (void*)0;
	game_driver_addr_p3 = (void*)0;
	game_driver_addr_p4 = (void*)0;
	//reset nes driver hook
	_got_game_hooks = false;
	_our_fill_addr = (void*)0;
	_our_empty_addr = (void*)0;
	_ori_play_patchaddr = (void*)0;
	_ori_play_endaddr = (void*)0;
	doEmptyQueue = (void*)0;
	//reset current module id
	cur_module = -1;
	//revert module hooks
	PatchBL(os_link, &module_init_oslink_addr); //revert module init
	PatchBLR(&module_uninit_exit_addr); //revert module uninit
	PatchBLR(&packfile_transfer_exit_addr); //revert pack transfer
}

static uint8_t bin1Get_16k(uint16_t addr)
{
	return game_driver_addr_p1[addr&0x3FFF];
}
static uint8_t bin2Get_16k(uint16_t addr)
{
	return game_driver_addr_p2[addr&0x3FFF];
}

static uint8_t bin1Get_8k(uint16_t addr)
{
	return game_driver_addr_p1[addr&0x1FFF];
}
static uint8_t bin2Get_8k(uint16_t addr)
{
	return game_driver_addr_p2[addr&0x1FFF];
}
static uint8_t bin3Get_8k(uint16_t addr)
{
	return game_driver_addr_p3[addr&0x1FFF];
}
static uint8_t bin4Get_8k(uint16_t addr)
{
	return game_driver_addr_p4[addr&0x1FFF];
}

extern uint16_t game_amp_val;
//used in apu.c and cpu.c
uint8_t curAmpVal = 0;
uint16_t game_driver_start = 0, game_driver_end = 0;
//used to buffer emu and game thread
static int _intqueue = 0;
static u8 _intarray[0x10];

void _mm_packfile_transfer_hook(int id, u8 *address)
{
	bool got_nes_rom = false;
	int nes_hdr_len = 0;
	//MM1 pack loaded that contains NES rom
	if(cur_module == 0 && id == 0x11)
	{
		got_nes_rom = true;
		nes_hdr_len = 0x24;
		//music bank 4
		game_driver_addr_p1 = address+(4<<14)+nes_hdr_len;
		//static bank 7
		game_driver_addr_p2 = address+(7<<14)+nes_hdr_len;
		//entry at bank 7+0x1551 (D551)
		game_driver_start = 0xD551;
		//force end at bank 7+0x156E (D56E)
		game_driver_end = 0xD56E;
	} //MM2 pack loaded that contains NES rom
	else if(cur_module == 1 && id == 0x13)
	{
		got_nes_rom = true;
		nes_hdr_len = 0x44;
		//music bank C
		game_driver_addr_p1 = address+(0xC<<14)+nes_hdr_len;
		//remove door open/close loop in sound driver,
		//because the game does not properly stop it
		PatchByte(game_driver_addr_p1+0x3DB2, 6);
		//static bank F
		game_driver_addr_p2 = address+(0xF<<14)+nes_hdr_len;
		//entry at bank F+0x10A7 (D0A7)
		game_driver_start = 0xD0A7;
		//force end at bank F+0x10BE (D0BE)
		game_driver_end = 0xD0BE;
	} //MM3 pack loaded that contains NES rom
	else if(cur_module == 2 && id == 0x13)
	{
		got_nes_rom = true;
		nes_hdr_len = 0x84;
		//music banks 16, 17 and 18
		game_driver_addr_p1 = address+(0x16<<13)+nes_hdr_len;
		game_driver_addr_p2 = address+(0x17<<13)+nes_hdr_len;
		game_driver_addr_p3 = address+(0x18<<13)+nes_hdr_len;
		//patch out bank switching code, we have that
		//bank hardcoded into the right spot already
		PatchByte(game_driver_addr_p1+0x3E, 0xEA); PatchByte(game_driver_addr_p1+0x3F, 0xEA);
		PatchByte(game_driver_addr_p1+0x40, 0xEA); PatchByte(game_driver_addr_p1+0x41, 0xEA);
		//static bank 1F
		game_driver_addr_p4 = address+(0x1F<<13)+nes_hdr_len;
		//entry at bank 1F+0x1FA8 (FFA8)
		game_driver_start = 0xFFA8;
		//force end at bank 1F+0x1FC5 (FFC5)
		game_driver_end = 0xFFC5;
	} //MM4 pack loaded that contains NES rom
	else if(cur_module == 3 && id == 0x16)
	{
		got_nes_rom = true;
		nes_hdr_len = 0x104;
		//music banks 1E, 1F and 1D (not sequential for some reason)
		game_driver_addr_p1 = address+(0x1E<<13)+nes_hdr_len;
		game_driver_addr_p2 = address+(0x1F<<13)+nes_hdr_len;
		game_driver_addr_p3 = address+(0x1D<<13)+nes_hdr_len;
		//patch out bank switching code, we have that
		//bank hardcoded into the right spot already
		PatchByte(game_driver_addr_p1+0x3E, 0xEA); PatchByte(game_driver_addr_p1+0x3F, 0xEA);
		PatchByte(game_driver_addr_p1+0x40, 0xEA); PatchByte(game_driver_addr_p1+0x41, 0xEA);
		//static bank 3F
		game_driver_addr_p4 = address+(0x3F<<13)+nes_hdr_len;
		//entry at bank 3F+0x1F78 (FF78)
		game_driver_start = 0xFF78;
		//force end at bank 3F+0x1F9F (FF9F)
		game_driver_end = 0xFF9F;
	} //MM5 pack loaded that contains NES rom
	else if(cur_module == 4 && id == 0x16)
	{
		got_nes_rom = true;
		nes_hdr_len = 0x84;
		//music banks 18, 19 and 1A
		game_driver_addr_p1 = address+(0x18<<13)+nes_hdr_len;
		game_driver_addr_p2 = address+(0x19<<13)+nes_hdr_len;
		game_driver_addr_p3 = address+(0x1A<<13)+nes_hdr_len;
		//patch out bank switching code, we have that
		//bank hardcoded into the right spot already
		PatchByte(game_driver_addr_p1+0x3E, 0xEA); PatchByte(game_driver_addr_p1+0x3F, 0xEA);
		PatchByte(game_driver_addr_p1+0x40, 0xEA); PatchByte(game_driver_addr_p1+0x41, 0xEA);
		//static bank 1F
		game_driver_addr_p4 = address+(0x1F<<13)+nes_hdr_len;
		//entry at bank 1F+0x1F88 (FF88)
		game_driver_start = 0xFF88;
		//force end at bank 1F+0x1FA9 (FFA9)
		game_driver_end = 0xFFA9;
	}
	if(got_nes_rom && _got_game_hooks)
	{
		//use music volume for driver general volume
		uint16_t tmpval = game_amp_val>>1;
		curAmpVal = tmpval > 0x7F ? 0x7F : tmpval;
		//init emu
		apuInitBufs();
		cpuInit();
		ppuInit();
		memInit();
		apuInit();
		inputInit();
		memInitGetSetPointers(); //pre-set get8/set8
		uint32_t addr;
		if(cur_module == 0 || cur_module == 1) //MM1/MM2
		{
			for(addr = 0x8000; addr < 0xC000; addr++)
				memInitMapperGetPointer(addr, bin1Get_16k);
			for(addr = 0xC000; addr < 0x10000; addr++)
				memInitMapperGetPointer(addr, bin2Get_16k);
		}
		else //MM3/MM4/MM5
		{
			for(addr = 0x8000; addr < 0xA000; addr++)
				memInitMapperGetPointer(addr, bin1Get_8k);
			for(addr = 0xA000; addr < 0xC000; addr++)
				memInitMapperGetPointer(addr, bin2Get_8k);
			for(addr = 0xC000; addr < 0xE000; addr++)
				memInitMapperGetPointer(addr, bin3Get_8k);
			for(addr = 0xE000; addr < 0x10000; addr++)
				memInitMapperGetPointer(addr, bin4Get_8k);
			//make sure to mute driver before game starts
			for(addr = 0xDC; addr < 0xE4; addr++)
				memSet8(addr, 0x88);
		}
		//got ROM so we can now patch the music call
		PatchB(_our_play_prep, _ori_play_patchaddr);
		doEmptyQueue = _our_empty_addr;
		_intqueue = 0;
		//create emu thread
		our_thread = bink_mmu_malloc(0x400);
		thread_stack = bink_mmu_malloc(0x800);
		os_create_thread(our_thread, doFrameUpdate, 0, thread_stack + 0x800, 0x800, 0, 0);
		running = 1;
		//grab voice to use
		used_nes_voice = ax_acquire_voice(31,(void*)0,0);
		//clear old aram
		u8 *tmpbuf = apuGetBuf();
		u32 tmpsize = apuGetBufSize();
		mmu_memset(tmpbuf, 0, tmpsize);
		dc_flush_range(tmpbuf, tmpsize);
		wait_audio_data();
		gc_aram_upload(tmpbuf, used_aram_addr, tmpsize, 0);
		wait_audio_data();
		gc_aram_upload(tmpbuf, used_aram_addr+0x280, tmpsize, 0);
		wait_audio_data();
		//reset aram addresses
		used_aram_addr_cur = used_aram_addr;
		used_aram_addr_prev = used_aram_addr+0x280;
		//setup voice
		mix_init_channel(used_nes_voice, 0, 0, -960, -960, 64, 127, 0);
		u16 ax_addr_arr[8];
		ax_addr_arr[0] = 1; //looping
		ax_addr_arr[1] = 0xA; //pcm16
		u32 shifted_addr = used_aram_addr>>1;
		ax_addr_arr[2] = shifted_addr>>16;
		ax_addr_arr[3] = shifted_addr&0xFFFF;
		u32 shifted_addr_end = used_aram_addr_end>>1;
		ax_addr_arr[4] = shifted_addr_end>>16;
		ax_addr_arr[5] = shifted_addr_end&0xFFFF;
		ax_addr_arr[6] = shifted_addr>>16;
		ax_addr_arr[7] = shifted_addr&0xFFFF;
		ax_set_voice_addr(used_nes_voice, ax_addr_arr);
		ax_set_voice_src_type(used_nes_voice, 0);
		//register voice into game and set callback
		u32 idx = *(u32*)(used_nes_voice+0x18);
		game_ax_voice_array[idx].addr = used_nes_voice;
		game_ax_voice_array[idx].val1 = 4;
		game_ax_voice_array[idx].val2 = 0xDEADBEEF+0x14;
		game_ax_voice_array[idx].val3 = 0xFFFFFFFF;
		sfx_registercallback(_my_sfx_callback);
		//and go
		ax_set_voice_state(used_nes_voice, 1);
	}
}

//store request into separate queue for thread separation
static void _mm1_mm2_fillqueue(u8 val)
{
	int t = os_disable_interrupts();
	if(_intqueue < 0x10) //up to 16 elements
		_intarray[_intqueue++] = val;
	os_restore_interrupts(t);
}
static void _mm3_mm4_mm5_fillqueue(u8 val)
{
	int t = os_disable_interrupts();
	if(_intqueue < 8) //up to 8 elements
		_intarray[_intqueue++] = val;
	os_restore_interrupts(t);
}

//empty queue into emu mem for internal MM1 play routine
static void _mm1_emptyqueue()
{
	int t = os_disable_interrupts();
	int copypos;
	for(copypos = 0; copypos < _intqueue; copypos++)
		memSet8(0x580+copypos, _intarray[copypos]);
	memSet8(0x45, _intqueue);
	_intqueue = 0;
	os_restore_interrupts(t);
}
//empty queue into emu mem for internal MM2 play routine
static void _mm2_emptyqueue()
{
	int t = os_disable_interrupts();
	int copypos;
	for(copypos = 0; copypos < _intqueue; copypos++)
		memSet8(0x580+copypos, _intarray[copypos]);
	memSet8(0x66, _intqueue);
	_intqueue = 0;
	os_restore_interrupts(t);
}
//empty queue into emu mem for internal MM3/MM4/MM5 play routine
static void _mm3_mm4_mm5_emptyqueue()
{
	int t = os_disable_interrupts();
	u8 curslot = memGet8(0xDA);
	int copypos;
	for(copypos = 0; copypos < _intqueue; copypos++)
		memSet8(0xDC+((curslot+copypos)&7), _intarray[copypos]);
	memSet8(0xDA, (curslot+_intqueue)&7);
	_intqueue = 0;
	os_restore_interrupts(t);
}
