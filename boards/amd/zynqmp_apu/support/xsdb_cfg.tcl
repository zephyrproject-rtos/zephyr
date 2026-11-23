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
	parse_args {elf fsbl bl31 pmufw} {bitstream}

	if { [info exists ::env(HW_SERVER_URL)] } {
		connect -url $::env(HW_SERVER_URL)
	} else {
		connect
	}

	# Wait for targets to become available after connect
	for {set i 0} {$i < 20} {incr i} {
		if { [ta] != "" } break
		after 50
	}

	after 2000
	boot_jtag
	after 2000

	# load bitstream while PSU is the active target
	if { [info exists args_arr(bitstream)] } {
		fpga -f $args_arr(bitstream) -no-revision-check
	}

	# Load PMUFW on the PMU MicroBlaze before FSBL so TF-A IPI calls succeed.
	# TF-A communicates with PMUFW via IPI during bl31_main (pm_mmio_read etc.);
	# without PMUFW running the IPI notify loop hangs indefinitely.
	# PMU MicroBlaze is hidden from JTAG by default after reset; writing
	# 0x1C0 to PMU_GLOBAL_GEN_STORAGE4 (0xFFCA0038) makes it visible.
	targets -set -nocase -filter {name =~ "*PSU*"}
	mask_write 0xFFCA0038 0x1C0 0x1C0
	if { [catch {targets -set -nocase -filter {name =~ "MicroBlaze PMU"}} err] == 0 } {
		dow $args_arr(pmufw)
		con
		after 2000
	} else {
		puts "WARNING: PMU target not visible even after enabling JTAG visibility, skipping PMUFW load"
	}

	# Set APU exception vector base and release from reset before FSBL
	targets -set -nocase -filter {name =~ "*APU*"}
	mwr 0xffff0000 0x14000000

	# FSBL must run on APU (A53 #0) to initialize DDR and clocks
	targets -set -nocase -filter {name =~ "*A53*#0"}
	rst -proc

	dow $args_arr(fsbl)
	after 2000
	con
	after 4000
	catch {stop}

	# load Zephyr and BL31 (TF-A) on APU (A53 #0)
	targets -set -nocase -filter {name =~ "*A53*#0"}
	rst -proc
	after 2000
	dow $args_arr(elf)
	dow $args_arr(bl31)
	con
	exit
}

load_image
