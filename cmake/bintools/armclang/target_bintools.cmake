# List of format the tool supports for converting, for example,
# GNU tools uses objectcopy, which supports the following: ihex, srec, binary
set_property(TARGET bintools PROPERTY elfconvert_formats ihex binary)

# armclang toolchain does not support all options in a single command
# Therefore a CMake script is used, so that multiple commands can be executed
# successively.
set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_COMMAND})

set_property(TARGET bintools PROPERTY elfconvert_flag
                                      -DFROMELF=${CMAKE_FROMELF}
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
#                 In this implementation `fromelf` is used
#   disassembly_flag               : --disassemble
#   disassembly_flag_final         : empty
#   disassembly_flag_inline_source : --interleave=source
#   disassembly_flag_all           : empty, fromelf does not differentiate on this.
#   disassembly_flag_infile        : empty, fromelf doesn't take arguments for filenames
#   disassembly_flag_outfile       : --output

set_property(TARGET bintools PROPERTY disassembly_command ${CMAKE_FROMELF})
set_property(TARGET bintools PROPERTY disassembly_flag --disassemble)
set_property(TARGET bintools PROPERTY disassembly_flag_final "")
set_property(TARGET bintools PROPERTY disassembly_flag_inline_source --interleave=source)
set_property(TARGET bintools PROPERTY disassembly_flag_all "")

set_property(TARGET bintools PROPERTY disassembly_flag_infile "")
set_property(TARGET bintools PROPERTY disassembly_flag_outfile "--output=" )

#
# - readelf : Name of command for reading elf files.
#             In this implementation `fromelf` is used
#   readelf_flag          : empty
#   readelf_flag_final    : empty
#   readelf_flag_headers  : --text
#   readelf_flag_infile   : empty, fromelf doesn't take arguments for filenames
#   readelf_flag_outfile  : --output

# This is using fromelf from arm-ds / Keil.
set_property(TARGET bintools PROPERTY readelf_command ${CMAKE_FROMELF})

set_property(TARGET bintools PROPERTY readelf_flag "")
set_property(TARGET bintools PROPERTY readelf_flag_final "")
set_property(TARGET bintools PROPERTY readelf_flag_headers --text)

set_property(TARGET bintools PROPERTY readelf_flag_infile "")
set_property(TARGET bintools PROPERTY readelf_flag_outfile "--output=")
