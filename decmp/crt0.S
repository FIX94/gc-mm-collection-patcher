#
# Copyright (C) 2019 FIX94
#
# This software may be modified and distributed under the terms
# of the MIT license.  See the LICENSE file for details.
#
.extern _gc_aram_init_hook
.text
.section .init

	#early module init hook
	b _main
	#gc aram init hook right after
	b _gc_aram_init_hook
