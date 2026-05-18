# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

proc boot_jtag { } {
	targets -set -filter {name =~ "PSU*"}
	stop
	mwr 0xffca0010 0x0
	mwr 0xff5e0200 0x0100
	rst -system
}

proc load_image args  {
	set elf_file [lindex $args 0]
	set bitfile [lindex $args 1]
	set fsblelf_file [lindex $args 2]
	set bl31_file [lindex $args 3]
	set pmufw_file [lindex $args 4]

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
	if { [string length $bitfile] != 0 } {
		fpga -f $bitfile -no-revision-check
	}

	# Load PMUFW on the PMU MicroBlaze before FSBL so TF-A IPI calls succeed.
	# TF-A communicates with PMUFW via IPI during bl31_main (pm_mmio_read etc.);
	# without PMUFW running the IPI notify loop hangs indefinitely.
	# PMU MicroBlaze is hidden from JTAG by default after reset; writing
	# 0x1C0 to PMU_GLOBAL_GEN_STORAGE4 (0xFFCA0038) makes it visible.
	if { [string length $pmufw_file] != 0 } {
		targets -set -nocase -filter {name =~ "*PSU*"}
		mask_write 0xFFCA0038 0x1C0 0x1C0
		if { [catch {targets -set -nocase -filter {name =~ "MicroBlaze PMU"}} err] == 0 } {
			dow $pmufw_file
			con
			after 2000
		} else {
			puts "WARNING: PMU target not visible even after enabling JTAG visibility, skipping PMUFW load"
		}
	}

	# Set APU exception vector base and release from reset before FSBL
	targets -set -nocase -filter {name =~ "*APU*"}
	mwr 0xffff0000 0x14000000

	# FSBL must run on APU (A53 #0) to initialize DDR and clocks
	targets -set -nocase -filter {name =~ "*A53*#0"}
	rst -proc

	if { [string length $fsblelf_file] != 0 } {
		dow $fsblelf_file
		after 2000
		con
		after 4000
		catch {stop}
	}

	# load Zephyr and BL31 (TF-A) on APU (A53 #0)
	targets -set -nocase -filter {name =~ "*A53*#0"}
	rst -proc
	after 2000
	dow $elf_file
	if { [string length $bl31_file] != 0 } {
		dow $bl31_file
	}
	con
	exit
}

switch $argc {
	"1" {
		set zephyr_elf [lindex $argv 0]
		set bitstream ""
		set fsbl_elf ""
		set bl31_elf ""
		set pmufw_elf ""
	}
	"2" {
		set par [lindex $argv 1]
		set substring "*.bit"
		if { [string match -nocase $substring $par] == 0 } {
			set bitstream ""
			set fsbl_elf $par
		} else {
			set bitstream $par
			set fsbl_elf ""
		}
		set zephyr_elf [lindex $argv 0]
		set bl31_elf ""
		set pmufw_elf ""
	}
	"3" {
		set zephyr_elf [lindex $argv 0]
		set bitstream [lindex $argv 1]
		set fsbl_elf [lindex $argv 2]
		set bl31_elf ""
		set pmufw_elf ""
	}
	"4" {
		set zephyr_elf [lindex $argv 0]
		set bitstream [lindex $argv 1]
		set fsbl_elf [lindex $argv 2]
		set bl31_elf [lindex $argv 3]
		set pmufw_elf ""
	}
	"5" {
		set zephyr_elf [lindex $argv 0]
		set bitstream [lindex $argv 1]
		set fsbl_elf [lindex $argv 2]
		set bl31_elf [lindex $argv 3]
		set pmufw_elf [lindex $argv 4]
	}
	default {
		puts "Insufficient number of args"
		exit -1
	}
}

load_image "$zephyr_elf" "$bitstream" "$fsbl_elf" "$bl31_elf" "$pmufw_elf"
