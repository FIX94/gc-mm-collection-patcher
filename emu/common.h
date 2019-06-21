#ifndef _common_h_
#define _common_h_

//emu defines
#if DO_INLINE_ATTRIBS
#ifdef _MSC_VER
#define FIXNES_NOINLINE __declspec(noinline)
#define FIXNES_ALWAYSINLINE __forceinline
#else
#define FIXNES_NOINLINE __attribute__((noinline))
#define FIXNES_ALWAYSINLINE __attribute__((always_inline))
#endif
#else
#define FIXNES_NOINLINE
#define FIXNES_ALWAYSINLINE
#endif

//just various typedef things
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef char bool;
#define true 1
#define false 0

//functions imported from game
int ar_get_dma_status();

u8 *ax_acquire_voice(int prio, void *cb, int sth);
void ax_set_voice_state(void *voice, int state);
void ax_set_voice_addr(void *voice, void *addr_arr);
void ax_set_voice_src_type(void *voice, u32 type);
void ax_free_voice(u8 *voice);

void mix_init_channel(void *voice, int v1, int v2, int v3, int v4, int v5, int v6, int v7);

void os_report(const char *in, ...);

int os_disable_interrupts();
int os_restore_interrupts(int t);

void os_link(void *ptr1, void *ptr2);

void os_create_thread(void *thread, void *func, u32 funcvar, void *stack_start, u32 stack_size, u32 prio, u32 params);
void os_resume_thread(void *thread);
void os_suspend_thread(void *thread);
void os_join_thread(void *thread, void *ret);
u32 os_is_thread_suspended(void *thread);

void dc_flush_range(void *buf, int size);
void ic_inv_range(void *buf, int size);

void gc_aram_upload(void *src, u32 dst, u32 len, u32 type);
u32 gc_aram_malloc(u32 size);

void *bink_mmu_malloc(u32 size);
void mmu_free(void *ptr);
void mmu_memset(void *dat, int val, uint32_t size);

void sfx_registercallback(void *cb);

#endif
