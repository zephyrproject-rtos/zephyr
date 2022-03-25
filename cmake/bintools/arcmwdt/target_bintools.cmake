# - memusage: Name of command for memusage
#             In this implementation `sizeac` is used
set_property(TARGET bintools PROPERTY memusage_command "${CMAKE_SIZE}")
set_property(TARGET bintools PROPERTY memusage_flag "-gq")
set_property(TARGET bintools PROPERTY memusage_infile "")


# List of format the tool supports for converting, for example,
# GNU tools uses objectcopy, which supports the following: ihex, srec, binary
set_property(TARGET bintools PROPERTY elfconvert_formats ihex srec binary)

# MWDT toolchain does not support all options in a single command
# Therefore a CMake script is used, so that multiple commands can be executed
# successively.
set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_COMMAND})

set_property(TARGET bintools PROPERTY elfconvert_flag
                                      -DELF2HEX=${CMAKE_ELF2HEX}
                                      -DELF2BIN=${CMAKE_ELF2BIN}
                                      -DSTRIP=${CMAKE_STRIP}
                                      -DOBJCOPY=${CMAKE_OBJCOPY}
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

# - disassembly : Name of command for disassembly of files
#                 In this implementation `elfdumpac` is used
#   disassembly_flag               : -T
#   disassembly_flag_final         : empty
#   disassembly_flag_inline_source : -S
#   disassembly_flag_all           : empty
#   disassembly_flag_infile        : empty
#   disassembly_flag_outfile       : '>'
set_property(TARGET bintools PROPERTY disassembly_command ${CMAKE_OBJDUMP})
set_property(TARGET bintools PROPERTY disassembly_flag -T)
set_property(TARGET bintools PROPERTY disassembly_flag_final "")
set_property(TARGET bintools PROPERTY disassembly_flag_inline_source -S)
set_property(TARGET bintools PROPERTY disassembly_flag_all "") # No support for all ?

set_property(TARGET bintools PROPERTY disassembly_flag_infile "")
set_property(TARGET bintools PROPERTY disassembly_flag_outfile > )

#
# - readelf : Name of command for reading elf files.
#             In this implementation `elfdumpac ` is used
#   readelf_flag          : empty
#   readelf_flag_final    : empty
#   readelf_flag_headers  : -qshp
#   readelf_flag_infile   : empty
#   readelf_flag_outfile  : '>'

# This is using elfdumpac from mwdt.
set_property(TARGET bintools PROPERTY readelf_command ${CMAKE_READELF})

set_property(TARGET bintools PROPERTY readelf_flag "")
set_property(TARGET bintools PROPERTY readelf_flag_final "")
set_property(TARGET bintools PROPERTY readelf_flag_headers -qshp)

set_property(TARGET bintools PROPERTY readelf_flag_infile "")
set_property(TARGET bintools PROPERTY readelf_flag_outfile > )

#
# - strip: Name of command for stripping symbols
#          In this implementation `stripac` is used
#   strip_flag         : empty
#   strip_flag_final   : empty
#   strip_flag_all     : -qls
#   strip_flag_debug   : -ql
#   strip_flag_dwo     : empty
#   strip_flag_infile  : empty
#   strip_flag_outfile : -o

# This is using strip from bintools.
set_property(TARGET bintools PROPERTY strip_command ${CMAKE_STRIP})

# Any flag the strip command requires for processing
set_property(TARGET bintools PROPERTY strip_flag "")
set_property(TARGET bintools PROPERTY strip_flag_final "")

set_property(TARGET bintools PROPERTY strip_flag_all -qls)
set_property(TARGET bintools PROPERTY strip_flag_debug -ql)

set_property(TARGET bintools PROPERTY strip_flag_infile "")
set_property(TARGET bintools PROPERTY strip_flag_outfile -o )
