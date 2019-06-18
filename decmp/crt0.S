#
# Copyright (C) 2019 FIX94
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the LICENSE file for details.
#
.extern _main
.extern _gc_aram_init_hook

.extern game_controller_update
.extern gc_entry_playlogos
.extern frontend_init_play_fmv_addr
.extern frontend_init_skip_fmv_addr
.extern input_start_held

.text
.section .init

	#early module init hook
	b _main
	#gc aram init hook right after
	b _gc_aram_init_hook
	#frontend init hook
	b _check_fmv_skip

_check_fmv_skip:
	# update controller
	bl game_controller_update
	#see if start is pressed
	lis 3, input_start_held@ha
	lbz 3, input_start_held@l(3)
	cmpwi 3, 0
	beq _return_to_fmv_playmedia
	#jump to fmv_playmedia end
	lis 3, frontend_init_skip_fmv_addr@h
	ori 3, 3, frontend_init_skip_fmv_addr@l
	mtlr 3
	#skips right over logos and movies
	blr
_return_to_fmv_playmedia:
	lis 3, frontend_init_play_fmv_addr@h
	ori 3, 3, frontend_init_play_fmv_addr@l
	mtlr 3
	#plays logos and intro movies
	b gc_entry_playlogos
