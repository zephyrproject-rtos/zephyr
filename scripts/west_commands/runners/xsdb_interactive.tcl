# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Interactive XSDB bootstrap used by the west xsdb runner (debug/debugserver).
#
# The west runner generates a "key = value" data file in the build directory
# and passes its path as the first argument, e.g.:
#
#     xsdb -interactive xsdb_interactive.tcl <build_dir>/xsdb_debug_params.ini
#
# Recognised keys:
#   board_cfg  - path to the board support/xsdb.cfg
#   elf        - path to the Zephyr ELF
#   hw_server  - optional hw_server URL (read by the board cfg)
#   boot_arg   - repeatable; ordered "key value key value ..." tokens
#                consumed by the board cfg's load_image/parse_args
#
# The parameters are parsed into a local dictionary (no environment variables,
# no sourced globals), so this script can be run standalone for troubleshooting.

if {$argc < 1} {
	error "usage: xsdb -interactive xsdb_interactive.tcl <params.ini>"
}

set params_file [lindex $argv 0]
if {![file exists $params_file]} {
	error "XSDB debug parameter file not found: $params_file"
}

# Parse the "key = value" data file into a local dictionary. No external Tcl
# package is required. "boot_arg" may repeat and builds an ordered list.
set params [dict create boot_args {}]
set fh [open $params_file r]
foreach line [split [read $fh] "\n"] {
	set line [string trim $line]
	if {$line eq "" || [string index $line 0] eq "#"} {
		continue
	}
	set sep [string first "=" $line]
	if {$sep < 0} {
		continue
	}
	set key [string trim [string range $line 0 [expr {$sep - 1}]]]
	set val [string trim [string range $line [expr {$sep + 1}] end]]
	if {$key eq "boot_arg"} {
		dict lappend params boot_args $val
	} else {
		dict set params $key $val
	}
}
close $fh

set cfg_file [dict get $params board_cfg]
# zephyr_boot_args is intentionally global so the interactive reload/restart
# helpers below can reuse it; it is state for the session, not a parameter
# channel.
set zephyr_boot_args [dict get $params boot_args]

# Board cfg scripts define load_image {} taking no proc arguments -- it reads
# its "key value ..." pairs from the global argv via parse_args, the same way
# a plain "xsdb xsdb.cfg key value ..." flash invocation does. Point argv/argc
# at the debug session's boot args before calling it so the same load_image
# works unmodified for both flash and interactive debug.
proc invoke_load_image {} {
	global zephyr_boot_args
	set ::argv $zephyr_boot_args
	set ::argc [llength $zephyr_boot_args]
	load_image
}

# The board cfg reads HW_SERVER_URL from the environment; forward it only when
# the user requested a specific hw_server.
if {[dict exists $params hw_server]} {
	set ::env(HW_SERVER_URL) [dict get $params hw_server]
}

set fh [open $cfg_file r]
set cfg_content [read $fh]
close $fh

# Prevent auto-run so the user can set breakpoints first. Boards with
# multi-stage bring-up (e.g. zynqmp_apu/zynqmp_rpu running FSBL, or PMUFW)
# call "con" one or more times *before* the final application load to let
# those earlier stages run to completion -- only the LAST standalone "con"
# (the one that would free-run the just-loaded application) and the LAST
# standalone "exit" are neutralized; everything else in the board cfg must
# execute unmodified or DDR/clocks never get initialized. The trailing
# "(\s.*)?$" matches an optional whitespace-separated remainder so that a
# line with a trailing comment or arguments (e.g. "con ;# start execution")
# is also matched, while words such as "connect"/"console" are left untouched.
set cfg_lines [split $cfg_content "\n"]
set last_con_idx -1
set last_exit_idx -1
for {set i 0} {$i < [llength $cfg_lines]} {incr i} {
	set line [lindex $cfg_lines $i]
	if {[regexp {^\s*con(\s.*)?$} $line]} {
		set last_con_idx $i
	}
	if {[regexp {^\s*exit(\s.*)?$} $line]} {
		set last_exit_idx $i
	}
}
if {$last_con_idx >= 0} {
	set cfg_lines [lreplace $cfg_lines $last_con_idx $last_con_idx \
		"# con (removed for interactive debugging)"]
}
if {$last_exit_idx >= 0} {
	set cfg_lines [lreplace $cfg_lines $last_exit_idx $last_exit_idx \
		"# exit (removed for interactive debugging)"]
}
set cfg_content [join $cfg_lines "\n"]
# Define load_image only; the west runner calls it explicitly below.
set cfg_content [regsub -all -line {^\s*load_image(\s.*)?$} $cfg_content \
	"# load_image (deferred to interactive bootstrap)"]

# `eval` does not set [info script], but board cfgs source their shared
# helper relative to it (e.g.
# "source [file join [file dirname [info script]] .. .. common
# xsdb_parse_args.tcl]"). Under eval, [info script] would resolve to this
# file's own directory (scripts/west_commands/runners) instead of the
# board's, breaking that lookup. Write the modified content to a sibling
# of the original board cfg and `source` it instead, so [info script] --
# and therefore the board cfg's relative source/file paths -- resolve the
# same way they would for a plain flash invocation.
set cfg_tmp_file [file join [file dirname $cfg_file] ".xsdb_interactive_[pid].tcl"]
set fh [open $cfg_tmp_file w]
puts -nonewline $fh $cfg_content
close $fh
set source_err [catch {source $cfg_tmp_file} source_result]
file delete $cfg_tmp_file
if {$source_err} {
	error $source_result
}

puts "\n========================================"
puts "Initializing platform and loading application..."
puts "========================================"

if {[catch {invoke_load_image} result]} {
	puts "ERROR during initialization: $result"
	puts ""
	puts "The board's initialization failed."
	puts "You can still use XSDB interactively to debug the issue:"
	if {[info exists ::env(HW_SERVER_URL)]} {
		puts "  xsdb% connect -url $::env(HW_SERVER_URL)"
	} else {
		puts "  xsdb% connect"
	}
	puts "  xsdb% targets"
	puts "  xsdb% dow -force [dict get $params elf]"
} else {
	puts "\nPlatform initialized and application loaded successfully!"

	if {[catch {bpadd -addr &main} result]} {
		puts "Note: Could not set breakpoint at main (this is normal if symbols not loaded yet)"
	} else {
		puts "Breakpoint set at main"
	}

	puts ""
	puts "========================================"
	puts "Ready for debugging!"
	puts "========================================"
}

proc reload {} {
	puts "Reloading application..."
	catch {stp}
	puts "Calling load_image to reinitialize..."
	if {[catch {invoke_load_image} result]} {
		puts "ERROR: $result"
	} else {
		puts "Application reloaded successfully!"
	}
}

proc restart {} {
	puts "Restarting application..."
	catch {stp}
	if {[catch {invoke_load_image} result]} {
		puts "ERROR: $result"
	} else {
		con
	}
}

puts {
XSDB interactive session ready.
Custom commands: reload, restart
Type help for the built-in XSDB command reference.}
