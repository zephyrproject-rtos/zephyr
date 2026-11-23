# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Shared by every board support/xsdb.cfg or support/xsdb_cfg.tcl: source
# this file, then call parse_args with the required/optional parameter
# names your board's load_image accepts, e.g.:
#
#     source [file join [file dirname [info script]] .. .. common xsdb_parse_args.tcl]
#
#     proc load_image {} {
#         global args_arr
#         parse_args {elf pdi} {bl31}
#         ...
#     }

# Populates global array ::args_arr from key/value pairs in argv.
# required : list of keys that MUST be present
# optional : list of keys that MAY be present (can be empty {})
proc parse_args {required optional} {
	global argv args_arr
	if { [llength $argv] % 2 != 0 } {
		error "arguments must be key/value pairs"
	}
	array set args_arr $argv
	set allowed [concat $required $optional]
	foreach key [array names args_arr] {
		if { [lsearch -exact $allowed $key] < 0 } {
			error "unknown parameter '$key' (allowed: [join $allowed {, }])"
		}
	}
	foreach key $required {
		if { ![info exists args_arr($key)] } {
			error "missing required parameter '$key'"
		}
	}
}
