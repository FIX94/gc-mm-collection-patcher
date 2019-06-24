#
# Copyright (C) 2019 FIX94
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the LICENSE file for details.
#
.extern _main
.extern _mm_packfile_transfer_hook
.extern _mm_playandexit

.globl _start
_start:
	# entry after decompression
	b _main

.global _packfile_transfer_prep
_packfile_transfer_prep:
	# restore id from old stack
	lwz 3, -0x1C(1)
	# restore address from old stack
	lwz 4, -0x18(1)
	lwz 4, 0(4)
	b _mm_packfile_transfer_hook

.global _ori_play_endaddr
_ori_play_endaddr:
	.long 0

.global _our_fill_addr
_our_fill_addr:
	.long 0

.global _our_play_prep
_our_play_prep:
	lbz 3, 1(3)
	lis 4, _ori_play_endaddr@ha
	lwz 4, _ori_play_endaddr@l(4)
	mtlr 4
	lis 4, _our_fill_addr@ha
	lwz 4, _our_fill_addr@l(4)
	mtctr 4
	bctr

#practically like _our_play_prep but for only
#a single command and return address for MM6
.global _mm6_cleft_ret_addr
_mm6_cleft_ret_addr:
	.long 0

.global _mm6_cleft_stopsfx
_mm6_cleft_stopsfx:
	li 3, 0xF1 #stop sfx cmd
	lis 4, _mm6_cleft_ret_addr@ha
	lwz 4, _mm6_cleft_ret_addr@l(4)
	mtlr 4
	lis 4, _our_fill_addr@ha
	lwz 4, _our_fill_addr@l(4)
	mtctr 4
	bctr
