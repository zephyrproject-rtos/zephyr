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

	if { [info exists ::env(HW_SERVER_URL)] } {
		connect -url $::env(HW_SERVER_URL)
	} else {
		connect
	}

	after 2000
	boot_jtag
	after 2000

	# load bitstream while PSU is the active target
	if { [string length $bitfile] != 0 } {
		fpga -f $bitfile -no-revision-check
	}

	# FSBL must run on APU (A53 #0) to initialize DDR and clocks
	if { [string length $fsblelf_file] != 0 } {
		targets -set -nocase -filter {name =~ "*A53*#0"}
		rst -proc
		dow $fsblelf_file
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

	dow $elf_file
	con
	exit
}

switch $argc {
	"1" {
		set zephyr_elf [lindex $argv 0]
		set bitstream ""
		set fsbl_elf ""
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
	}
	"3" {
		set zephyr_elf [lindex $argv 0]
		set bitstream [lindex $argv 1]
		set fsbl_elf [lindex $argv 2]
	}
	default {
		puts "Insufficient number of args"
		exit -1
	}
}

load_image "$zephyr_elf" "$bitstream" "$fsbl_elf"
