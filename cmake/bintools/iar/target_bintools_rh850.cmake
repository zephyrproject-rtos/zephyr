# List of format the tool supports for converting, for example,
# GNU tools uses objectcopy, which supports the following: ihex, srec, binary
set_property(TARGET bintools PROPERTY elfconvert_formats ihex srec binary)

# armclang toolchain does not support all options in a single command
# Therefore a CMake script is used, so that multiple commands can be executed
# successively.
set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_COMMAND})

set_property(TARGET bintools PROPERTY elfconvert_flag
                                      -DIELFTOOL=${CMAKE_OBJCOPY}
)

set_property(TARGET bintools PROPERTY elfconvert_flag_final
                                      -P ${CMAKE_CURRENT_LIST_DIR}/elfconvert_command.cmake)

set_property(TARGET bintools PROPERTY elfconvert_flag_strip_all "-DSTRIP_ALL=True")
set_property(TARGET bintools PROPERTY elfconvert_flag_strip_debug "-DSTRIP_DEBUG=True")

set_property(TARGET bintools PROPERTY elfconvert_flag_intarget "-DINTARGET=")
set_property(TARGET bintools PROPERTY elfconvert_flag_outtarget "-DOUTTARGET=")

set_property(TARGET bintools PROPERTY elfconvert_flag_section_remove "-DREMOVE_SECTION=")
set_property(TARGET bintools PROPERTY elfconvert_flag_section_only "-DONLY_SECTION=")

# mwdt doesn't handle rename, consider adjusting abstraction.
set_property(TARGET bintools PROPERTY elfconvert_flag_section_rename "-DRENAME_SECTION=")

set_property(TARGET bintools PROPERTY elfconvert_flag_gapfill "-DGAP_FILL=")
set_property(TARGET bintools PROPERTY elfconvert_flag_srec_len "-DSREC_LEN=")

set_property(TARGET bintools PROPERTY elfconvert_flag_infile "-DINFILE=")
set_property(TARGET bintools PROPERTY elfconvert_flag_outfile "-DOUTFILE=")

#
# - disassembly : Name of command for disassembly of files
#                 In this implementation `ielfdumprh850` is used
#   disassembly_flag               : --disasm_data
#   disassembly_flag_final         : empty
#   disassembly_flag_inline_source : empty
#   disassembly_flag_no_aliases    : empty
#   disassembly_flag_all           : empty
#   disassembly_flag_infile        : empty
#   disassembly_flag_outfile       : '>', readelf doesn't take arguments for output
#                                    file, but result is printed to standard out, and
#                                    is redirected.

set_property(TARGET bintools PROPERTY disassembly_command ${CMAKE_OBJDUMP})
set_property(TARGET bintools PROPERTY disassembly_flag --disasm_data)
set_property(TARGET bintools PROPERTY disassembly_flag_final "")
#set_property(TARGET bintools PROPERTY disassembly_flag_inline_source "")
#set_property(TARGET bintools PROPERTY disassembly_flag_no_aliases "")
#set_property(TARGET bintools PROPERTY disassembly_flag_all "")

set_property(TARGET bintools PROPERTY disassembly_flag_infile "")
set_property(TARGET bintools PROPERTY disassembly_flag_outfile ">;" )

#
# - readelf : Name of command for reading elf files.
#             In this implementation `ielfdumprh850` is used
#   readelf_flag          : empty
#   readelf_flag_final    : empty
#   readelf_flag_headers  : empty
#   readelf_flag_infile   : empty, readelf doesn't take arguments for filenames
#   readelf_flag_outfile  : '>', readelf doesn't take arguments for output
#                           file, but result is printed to standard out, and
#                           is redirected.

# This is using readelf from bintools.
set_property(TARGET bintools PROPERTY readelf_command ${CMAKE_READELF})

set_property(TARGET bintools PROPERTY readelf_flag "")
set_property(TARGET bintools PROPERTY readelf_flag_final "")
set_property(TARGET bintools PROPERTY readelf_flag_headers "")

set_property(TARGET bintools PROPERTY readelf_flag_infile "")
set_property(TARGET bintools PROPERTY readelf_flag_outfile ">;" )

# Example on how to support dwarfdump instead of readelf
#set_property(TARGET bintools PROPERTY readelf_command dwarfdump)
#set_property(TARGET bintools PROPERTY readelf_flag "")
#set_property(TARGET bintools PROPERTY readelf_flag_headers -E)
#set_property(TARGET bintools PROPERTY readelf_flag_infile "")
#set_property(TARGET bintools PROPERTY readelf_flag_outfile "-O file=" )
