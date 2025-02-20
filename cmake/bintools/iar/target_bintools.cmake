# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0
#
# - elfconvert  : Name of command for elf file conversion.
#                 In this implementation `objcopy` is used
#   elfconvert_formats            : Formats supported: ihex, srec, binary
#   elfconvert_flag               : empty
#   elfconvert_flag_final         : empty
#   elfconvert_flag_strip_all     : -S
#   elfconvert_flag_strip_debug   : -g
#   elfconvert_flag_intarget      : --input-target=
#   elfconvert_flag_outtarget     : --output-target=
#   elfconvert_flag_section_remove: --remove-section=
#   elfconvert_flag_section_only  : --only-section=
#   elfconvert_flag_section_rename: --rename-section;
#   elfconvert_flag_gapfill       : --gap-fill;
#                                   Note: The ';' will be transformed into an
#                                   empty space when executed
#   elfconvert_flag_srec_len      : --srec-len=
#   elfconvert_flag_infile        : empty, objcopy doesn't take arguments for filenames
#   elfconvert_flag_outfile       : empty, objcopy doesn't take arguments for filenames
#

# elfconvert to use for transforming an elf file into another format,
# such as intel hex, s-rec, binary, etc.
set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_OBJCOPY})

# List of format the tool supports for converting, for example,
# GNU tools uses objectcopy, which supports the following: ihex, srec, binary
set_property(TARGET bintools PROPERTY elfconvert_formats ihex srec binary)

set_property(TARGET bintools PROPERTY elfconvert_flag "")
set_property(TARGET bintools PROPERTY elfconvert_flag_final "")

set_property(TARGET bintools PROPERTY elfconvert_flag_strip_all "-S")
set_property(TARGET bintools PROPERTY elfconvert_flag_strip_debug "-g")

set_property(TARGET bintools PROPERTY elfconvert_flag_intarget "--input-target=")
set_property(TARGET bintools PROPERTY elfconvert_flag_outtarget "--output-target=")

set_property(TARGET bintools PROPERTY elfconvert_flag_section_remove "--remove-section=")
set_property(TARGET bintools PROPERTY elfconvert_flag_section_only "--only-section=")
set_property(TARGET bintools PROPERTY elfconvert_flag_section_rename "--rename-section;")

set_property(TARGET bintools PROPERTY elfconvert_flag_lma_adjust "--change-section-lma;")

# Note, placing a ';' at the end results in the following param  to be a list,
# and hence space separated.
# Thus the command line argument becomes:
# `--gap-file <value>` instead of `--gap-fill<value>` (The latter would result in an error)
set_property(TARGET bintools PROPERTY elfconvert_flag_gapfill "--gap-fill;")
set_property(TARGET bintools PROPERTY elfconvert_flag_srec_len "--srec-len=")

set_property(TARGET bintools PROPERTY elfconvert_flag_infile "")
set_property(TARGET bintools PROPERTY elfconvert_flag_outfile "")

#
# - disassembly : Name of command for disassembly of files
#                 In this implementation `objdump` is used
#   disassembly_flag               : -d
#   disassembly_flag_final         : empty
#   disassembly_flag_inline_source : -S
#   disassembly_flag_all           : -SDz
#   disassembly_flag_infile        : empty, objdump doesn't take arguments for filenames
#   disassembly_flag_outfile       : '>', objdump doesn't take arguments for output file, but result is printed to standard out, and is redirected.

set_property(TARGET bintools PROPERTY disassembly_command ${CMAKE_OBJDUMP})
set_property(TARGET bintools PROPERTY disassembly_flag -d)
set_property(TARGET bintools PROPERTY disassembly_flag_final "")
set_property(TARGET bintools PROPERTY disassembly_flag_inline_source -S)
set_property(TARGET bintools PROPERTY disassembly_flag_all -SDz)

set_property(TARGET bintools PROPERTY disassembly_flag_infile "")
set_property(TARGET bintools PROPERTY disassembly_flag_outfile ">;" )

#
# - strip: Name of command for stripping symbols
#          In this implementation `strip` is used
#   strip_flag         : empty
#   strip_flag_final   : empty
#   strip_flag_all     : --strip-all
#   strip_flag_debug   : --strip-debug
#   strip_flag_dwo     : --strip-dwo
#   strip_flag_infile  : empty, strip doesn't take arguments for input file
#   strip_flag_outfile : -o

# This is using strip from bintools.
set_property(TARGET bintools PROPERTY strip_command ${CMAKE_STRIP})

# Any flag the strip command requires for processing
set_property(TARGET bintools PROPERTY strip_flag "")
set_property(TARGET bintools PROPERTY strip_flag_final "")

set_property(TARGET bintools PROPERTY strip_flag_all --strip-all)
set_property(TARGET bintools PROPERTY strip_flag_debug --strip-debug)
set_property(TARGET bintools PROPERTY strip_flag_dwo --strip-dwo)
set_property(TARGET bintools PROPERTY strip_flag_remove_section -R )

set_property(TARGET bintools PROPERTY strip_flag_infile "")
set_property(TARGET bintools PROPERTY strip_flag_outfile -o )

#
# - readelf : Name of command for reading elf files.
#             In this implementation `readelf` is used
#   readelf_flag          : empty
#   readelf_flag_final    : empty
#   readelf_flag_headers  : -e
#   readelf_flag_infile   : empty, readelf doesn't take arguments for filenames
#   readelf_flag_outfile  : '>', readelf doesn't take arguments for output
#                           file, but result is printed to standard out, and
#                           is redirected.

# This is using readelf from bintools.
set_property(TARGET bintools PROPERTY readelf_command ${CMAKE_READELF})

set_property(TARGET bintools PROPERTY readelf_flag "")
set_property(TARGET bintools PROPERTY readelf_flag_final "")
set_property(TARGET bintools PROPERTY readelf_flag_headers -e)

set_property(TARGET bintools PROPERTY readelf_flag_infile "")
set_property(TARGET bintools PROPERTY readelf_flag_outfile ">;" )

# Example on how to support dwarfdump instead of readelf
#set_property(TARGET bintools PROPERTY readelf_command dwarfdump)
#set_property(TARGET bintools PROPERTY readelf_flag "")
#set_property(TARGET bintools PROPERTY readelf_flag_headers -E)
#set_property(TARGET bintools PROPERTY readelf_flag_infile "")
#set_property(TARGET bintools PROPERTY readelf_flag_outfile "-O file=" )

set_property(TARGET bintools PROPERTY symbols_command ${CMAKE_NM})
set_property(TARGET bintools PROPERTY symbols_flag "")
set_property(TARGET bintools PROPERTY symbols_final "")
set_property(TARGET bintools PROPERTY symbols_infile "")
set_property(TARGET bintools PROPERTY symbols_outfile ">;" )
