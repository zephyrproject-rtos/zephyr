# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

source [file join [file dirname [info script]] .. .. common xsdb_parse_args.tcl]

proc boot_jtag { } {
	targets -set -filter {name =~ "PSU*"}
	stop
	mwr 0xffca0010 0x0
	mwr 0xff5e0200 0x0100
	rst -system
}

proc load_image {} {
	global args_arr
	parse_args {elf} {bitstream fsbl}

	if { [info exists ::env(HW_SERVER_URL)] } {
		connect -url $::env(HW_SERVER_URL)
	} else {
		connect
	}

	after 2000
	boot_jtag
	after 2000

	# load bitstream while PSU is the active target
	if { [info exists args_arr(bitstream)] } {
		fpga -f $args_arr(bitstream) -no-revision-check
	}

	# FSBL must run on APU (A53 #0) to initialize DDR and clocks
	if { [info exists args_arr(fsbl)] } {
		targets -set -nocase -filter {name =~ "*A53*#0"}
		rst -proc
		dow $args_arr(fsbl)
		after 1000
		con
		after 3000
		stop
	}

	# load Zephyr on RPU (R5 #0)
	targets -set -nocase -filter {name =~ "*R5*#0"}
	rst -proc
	after 100

	# Initialize TCM vector region (first 256 bytes) for JTAG boot.
	# This prevents prefetch abort from uninitialized exception vectors
	# at 0x0 when booting via JTAG/XSDB without PLM initialization.
	mwr -size w 0x0 0x0 64
	after 100

	dow $args_arr(elf)
	con
	exit
}

load_image
