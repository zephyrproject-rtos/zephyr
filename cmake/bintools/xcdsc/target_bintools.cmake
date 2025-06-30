# Usage of the objcopy utility (aliased as elfconvert) for converting ELF binaries into other formats,such as hex, binary, etc.
set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_OBJCOPY})
# List of format the tool supports for converting, for example,
# GNU tools uses objectcopy, which supports the following: ihex, srec, binary
set_property(TARGET bintools PROPERTY elfconvert_formats ihex srec binary)
set_property(TARGET bintools PROPERTY elfconvert_flag_strip_all "-S")
set_property(TARGET bintools PROPERTY elfconvert_flag_strip_debug "-g")
set_property(TARGET bintools PROPERTY elfconvert_flag_intarget "--input-target=")
set_property(TARGET bintools PROPERTY elfconvert_flag_outtarget "--output-target=")
set_property(TARGET bintools PROPERTY elfconvert_flag_section_remove "--remove-section=")

# Usage of the objdump utility for producing human-readable disassembly listings from ELF binaries
set_property(TARGET bintools PROPERTY disassembly_command ${CMAKE_OBJDUMP})
set_property(TARGET bintools PROPERTY disassembly_flag -d)

set_property(TARGET bintools PROPERTY disassembly_flag_final "")
set_property(TARGET bintools PROPERTY disassembly_flag_inline_source -S)
# Usage of the strip utility for removing symbols from ELF binaries to ensure that final images include only the necessary code and data, improving load times and reducing footprint.
set_property(TARGET bintools PROPERTY strip_command ${CMAKE_STRIP})
set_property(TARGET bintools PROPERTY strip_flag_all --strip-all)
set_property(TARGET bintools PROPERTY strip_flag_debug --strip-debug)
#Usage of readelf. How to invoke and configure the readelf utility for examining ELF binaries
set_property(TARGET bintools PROPERTY readelf_command ${CMAKE_READELF})
set_property(TARGET bintools PROPERTY readelf_flag "")
set_property(TARGET bintools PROPERTY readelf_flag_final "")
set_property(TARGET bintools PROPERTY readelf_flag_headers -e)
set_property(TARGET bintools PROPERTY readelf_flag_infile "")
set_property(TARGET bintools PROPERTY readelf_flag_outfile ">;" )
