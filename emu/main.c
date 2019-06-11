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

static void PatchB( u32 dst, u32 src )
{
	u32 newval = (dst - src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000000;
	*(volatile u32*)src = newval;
	dc_flush_range((void*)src, 4);
	ic_inv_range((void*)src, 4);
}
static void PatchBLR( u32 addr )
{
	*(volatile u32*)addr = 0x4E800020;
	dc_flush_range((void*)addr, 4);
	ic_inv_range((void*)addr, 4);
}

static int cur_module = -1;

static u32 used_aram_addr = 0;
static u32 used_aram_addr_prev = 0;
static u32 used_aram_addr_cur = 0;
static u32 used_aram_addr_end = 0;

static void _module_init_hook();
static void _module_uninit_hook();
void _packfile_transfer_prep();

extern u32 module_init_exit_addr;
extern u32 module_uninit_exit_addr;
extern u32 packfile_transfer_exit_addr;
void _main(int module, u32 aram_addr)
{
	//only MM1/MM2 are set up right now
	if(module == 0 || module == 1)
	{
		//copy over current module id
		cur_module = module;
		//copy over main aram address to use
		used_aram_addr = aram_addr;
		used_aram_addr_cur = aram_addr;
		used_aram_addr_prev = aram_addr+0x280;
		used_aram_addr_end = aram_addr+0x4FF;
		// set up module patches
		PatchB((u32)_module_init_hook, (u32)&module_init_exit_addr);
		PatchB((u32)_module_uninit_hook, (u32)&module_uninit_exit_addr);
		PatchB((u32)_packfile_transfer_prep, (u32)&packfile_transfer_exit_addr);
	}
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
extern u32 _our_play_addr;
extern u32 _ori_play_endaddr;
extern void _our_play_prep();
//our hooks
void _mm_playandexit(u32 val);
void _mm2_playandexit(u32 val);
static void _module_init_hook()
{
	if(cur_module == 0)
	{
		//MM1 softpatches
		u8 *relentry1 = (u8*)game_loaded_module[4];
		//swap turbo A and B
		relentry1[0x64AF7] = 0x40;
		relentry1[0x64B13] = 0x40;
		//actually do audio processing internally
		_our_play_addr = (u32)_mm_playandexit;
		PatchB((u32)_our_play_prep, (u32)relentry1+0x63B68);
		_ori_play_endaddr = (u32)relentry1+0x63B68+0x334;
		u8 *relentry2 = (u8*)game_loaded_module[0xD];
		//swap A and B
		relentry2[0xD89E] = 2;
		relentry2[0xD89F] = 1;
	}
	else if(cur_module == 1)
	{
		//MM2 softpatches
		u8 *relentry1 = (u8*)game_loaded_module[4];
		//swap turbo button from A to B
		relentry1[0x10753B] = 0x40;
		relentry1[0x107557] = 0x40;
		//actually do audio processing internally
		_our_play_addr = (u32)_mm2_playandexit;
		PatchB((u32)_our_play_prep, (u32)relentry1+0xF709C);
		_ori_play_endaddr = (u32)relentry1+0xF709C+0x348;
		u8 *relentry2 = (u8*)game_loaded_module[0xD];
		//swap A and B
		relentry2[0xD89E] = 2;
		relentry2[0xD89F] = 1;
		//allow A in weapon menu
		relentry2[0x36E0D] = 0xD;
	}
	//grab voice to use
	if(!used_nes_voice)
		used_nes_voice = ax_acquire_voice(31,(void*)0,0);
	if(!used_nes_voice)
		return;
	//clear old aram
	u8 *tmpbuf = apuGetBuf();
	u32 tmpsize = apuGetBufSize();
	if(tmpbuf && tmpsize == 0x280)
	{
		mmu_memset(tmpbuf, 0, tmpsize);
		dc_flush_range(tmpbuf, tmpsize);
		wait_audio_data();
		gc_aram_upload(tmpbuf, used_aram_addr, tmpsize, 0);
		wait_audio_data();
		gc_aram_upload(tmpbuf, used_aram_addr+0x280, tmpsize, 0);
		wait_audio_data();
	}
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

static u8 *thread_stack = (u8*)0;
static u8 *game_driver_addr_p1 = (u8*)0; //bank C
static u8 *game_driver_addr_p2 = (u8*)0; //bank F

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
	//revert module hooks
	PatchBLR((u32)&module_init_exit_addr); //revert module init
	PatchBLR((u32)&module_uninit_exit_addr); //revert module uninit
	PatchBLR((u32)&packfile_transfer_exit_addr); //revert pack transfer
	//reset current module id
	cur_module = -1;
}
static uint8_t bin1Get(uint16_t addr)
{
	return game_driver_addr_p1[addr&0x3FFF];
}
static uint8_t bin2Get(uint16_t addr)
{
	return game_driver_addr_p2[addr&0x3FFF];
}
extern uint16_t game_amp_val;
uint8_t curAmpVal = 0;
uint16_t game_driver_start = 0, game_driver_end = 0;
void _mm_packfile_transfer_hook(int id, u8 *address)
{
	bool got_nes_rom = false;
	//MM1 pack loaded that contains NES rom
	if(cur_module == 0 && id == 0x11)
	{
		got_nes_rom = true;
		//bank 4 10024
		game_driver_addr_p1 = address+0x10024;
		//bank 7 1C024
		game_driver_addr_p2 = address+0x1C024;
		//entry at bank 7+0x1551 (D551)
		game_driver_start = 0xD551;
		//force end at bank 7+0x156E (D56E)
		//and add one instruction below BLE to not false end early
		game_driver_end = 0xD570;
	} //MM2 pack loaded that contains NES rom
	else if(cur_module == 1 && id == 0x13)
	{
		got_nes_rom = true;
		//30044 bank C
		game_driver_addr_p1 = address+0x30044;
		//remove door open/close loop in sound driver,
		//because the game does not properly stop it
		game_driver_addr_p1[0x3DB2] = 6;
		//3C044 bank F
		game_driver_addr_p2 = address+0x3C044;
		//entry at bank F+0x10A7 (D0A7)
		game_driver_start = 0xD0A7;
		//force end at bank F+0x10BE (D0BE)
		//and add one instruction below BLE to not false end early
		game_driver_end = 0xD0C0;
	}
	if(got_nes_rom)
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
		for(addr = 0x8000; addr < 0xC000; addr++)
			memInitMapperGetPointer(addr, bin1Get);
		for(addr = 0xC000; addr < 0x10000; addr++)
			memInitMapperGetPointer(addr, bin2Get);
		//create emu thread
		our_thread = bink_mmu_malloc(0x400);
		thread_stack = bink_mmu_malloc(0x800);
		os_create_thread(our_thread, doFrameUpdate, 0, thread_stack + 0x800, 0x800, 0, 0);
		running = 1;
	}
}
int os_disable_interrupts();
int os_restore_interrupts(int t);
//code is like internal MM1 play routine
void _mm_playandexit(u32 val)
{
	int t = os_disable_interrupts();
	u8 used = memGet8(0x45);
	if(used < 0x10)
	{
		//out of all things to change, its the hurt sound,
		//now being an ID thats not in the original NES driver,
		//so I guess as dumb as it is, we have to redirect it...
		if(val == 0x33) val = 0x16;
		memSet8(0x580+used,val);
		memSet8(0x45,used+1);
	}
	os_restore_interrupts(t);
}
//code is like internal MM2 play routine
void _mm2_playandexit(u32 val)
{
	int t = os_disable_interrupts();
	u8 used = memGet8(0x66);
	if(used < 0x10)
	{
		//the main theme part of the ending got its own ID in game,
		//but in the original NES driver that ID does not exist,
		//so redirect it to the original main theme ID to fix this
		if(val == 0x18) val = 0xD;
		memSet8(0x580+used,val);
		memSet8(0x66,used+1);
	}
	os_restore_interrupts(t);
}
