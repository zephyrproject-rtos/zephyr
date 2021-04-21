# This template file can be used as a starting point for implementing support
# for additional tools for reading and/or conversion of elf files.
#
# Although GNU bintools is used as name, then the template can be used to
# support other tools.
#
# Overview of bintools used commands:
# - memusage    : Tool for reporting target memory usage
#                 (if linker support memusage reporting leave this property empty)
# - disassembly : Tool for disassemble the target
# - elfconvert  : Tool for converting from elf into another format.
# - readelf     : Tool for elf file processing
# - strip       : Tool for symnbol stripping
#
# Each tool will have the following minimum properties:
# - <tool>_command : Name of executable to call
# - <tool>_flag    : Flags that must always be used with this command
# - <tool>_flag_infile  : Flag to use when specifying the file to process
# - <tool>_flag_outfile : Flag to use to specify the result of the processing.
#
# each tool might require more flags depending on its use, as example:
# - elfconvert_flag_section_remove : Flag to use when specifying sections to remove
# - readelf_flags_headers : Flag to use to specify headers should be displayed
#
# If a given tool / flag / feature is not supported, then keep the property empty.
# As bintools_template.cmake default has empty flags, then it is sufficient to
# only set those flags that a given set of tools support.
#
# Commands will default echo a message if called, but no command has been defined.
# This is done, so that unexpected calls to non-implemented command can be easily detected.
# To disable the message, simply silence the command with:
#     set_property(TARGET bintools PROPERTY <command>_command ${CMAKE_COMMAND} -E echo "")
#
# The bintools properties are made generic so that implementing support for an
# additional native tool should be as easy as possible.
# However, there might be tools and/or use cases where the current property
# scheme does not cover the exact needs. For those use-cases it is possible
# to implement the call to a native tool inside a CMake script.
# For example, to call a custom script for elfconvert command, one can specify:
#   set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_COMMAND})
#   set_property(TARGET bintools PROPERTY elfconvert_flag "")
#   set_property(TARGET bintools PROPERTY elfconvert_flag_final     -P custom_elfconvert.cmake)
#   set_property(TARGET bintools PROPERTY elfconvert_flag_strip_all "-DSTRIP_ALL=True")
#   set_property(TARGET bintools PROPERTY elfconvert_flag_infile    "-DINFILE=")
#   set_property(TARGET bintools PROPERTY elfconvert_flag_outfile   "-DOUT_FILE=")

#
#
# bintools property overview of all commands:
#
# Command:
# - memusage            : Name of command to execute
#                         Note: For gcc compilers this command is not used,
#                               instead a linker flag is used for this)
#   memusage_flag       : Flags that must always be applied when calling memusage command
#   memusage_flag_final : Flags that must always be applied last at the memusage command
#   memusage_flag_infile: Flag for specifying the input file
#   memusage_byproducts : Byproducts (files) generated when calling memusage
#
#
# - elfconvert  : Name of command for elf file conversion.
#                 For GNU binary utilities this is objcopy
#   elfconvert_formats            : Formats supported by this command.
#   elfconvert_flag               : Flags that must always be applied when calling elfconvert command
#   elfconvert_flag_final         : Flags that must always be applied last at the elfconvert command
#   elfconvert_flag_strip_all     : Flag that is used for stripping all symbols when converting
#   elfconvert_flag_strip_debug   : Flag that is used to strip debug symbols when converting
#   elfconvert_flag_intarget      : Flag for specifying target used for infile
#   elfconvert_flag_outtarget     : Flag for specifying target to use for converted file.
#                                   Target value must be one of those listed described by: elfconvert_formats
#   elfconvert_flag_section_remove: Flag for specifying that following section must be removed
#   elfconvert_flag_section_only  : Flag for specifying that only the following section should be kept
#   elfconvert_flag_section_rename: Flag for specifying that following section must be renamed
#   elfconvert_flag_gapfill       : Flag for specifying the value to fill in gaps between sections
#   elfconvert_flag_srec_len      : Flag for specifying maximum length of Srecord values
#   elfconvert_flag_infile        : Flag for specifying the input file
#   elfconvert_flag_outfile       : Flag for specifying the output file
#                                   For tools that prints to standard out, this should be ">" to indicate redirection
#
#
# - disassembly : Name of command for disassembly of files
#                 For GNU binary utilities this is objdump
#   disassembly_flag               : Flags that must always be applied when calling disassembly command
#   disassembly_flag_final         : Flags that must always be applied last at the disassembly command
#   disassembly_flag_inline_source : Flag to use to display source code mixed with disassembly
#   disassembly_flag_all           : Flag to use for disassemble everything, including zeroes
#   disassembly_flag_infile        : Flag for specifying the input file
#   disassembly_flag_outfile       : Flag for specifying the output file
#                                    For tools that prints to standard out, this should be ">" to indicate redirection
#
#
# - readelf : Name of command for reading elf files.
#             For GNU binary utilities this is readelf
#   readelf_flag          : Flags that must always be applied when calling readelf command
#   readelf_flag_final    : Flags that must always be applied last at the readelf command
#   readelf_flag_headers  : Flag to use for specifying ELF headers should be read
#   readelf_flag_infile   : Flag for specifying the input file
#   readelf_flag_outfile  : Flag for specifying the output file
#                           For tools that prints to standard out, this should be ">" to indicate redirection
#
#
# - strip: Name of command for stripping symbols
#          For GNU binary utilities this is strip
#   strip_flag         : Flags that must always be applied when calling strip command
#   strip_flag_final   : Flags that must always be applied last at the strip command
#   strip_flag_all     : Flag for removing all symbols
#   strip_flag_debug   : Flag for removing debug symbols
#   strip_flag_dwo     : Flag for removing dwarf sections
#   strip_flag_infile  : Flag for specifying the input file
#   strip_flag_outfile : Flag for specifying the output file

set(COMMAND_NOT_SUPPORTED "command not supported on bintools: ")

# If memusage is supported as post-build command, set memusage_type to: command
# and this value to the command to execute in the form: <command> <arguments>
# Note: If memusage is supported during linking, please use:
#       set_property(TARGET linker ... ) found in cmake/linker/linker_flags.cmake instead
set_property(TARGET bintools PROPERTY memusage_command "")
set_property(TARGET bintools PROPERTY memusage_flag "")
set_property(TARGET bintools PROPERTY memusage_flag_final "")
set_property(TARGET bintools PROPERTY memusage_byproducts "")

# disassembly command to use for generation of list file.
set_property(TARGET bintools PROPERTY disassembly_command ${CMAKE_COMMAND} -E echo "disassembly ${COMMAND_NOT_SUPPORTED} ${BINTOOLS}")
set_property(TARGET bintools PROPERTY disassembly_flag "")
set_property(TARGET bintools PROPERTY disassembly_flag_final "")
set_property(TARGET bintools PROPERTY disassembly_flag_inline_source "")
set_property(TARGET bintools PROPERTY disassembly_flag_infile "")
set_property(TARGET bintools PROPERTY disassembly_flag_outfile "")

# elfconvert to use for transforming an elf file into another format, such as intel hex, s-rec, binary, etc.
set_property(TARGET bintools PROPERTY elfconvert_command ${CMAKE_COMMAND} -E echo "elfconvert ${COMMAND_NOT_SUPPORTED} ${BINTOOLS}")
set_property(TARGET bintools PROPERTY elfconvert_formats "")
set_property(TARGET bintools PROPERTY elfconvert_flag "")
set_property(TARGET bintools PROPERTY elfconvert_flag_final "")
set_property(TARGET bintools PROPERTY elfconvert_flag_outtarget "")
set_property(TARGET bintools PROPERTY elfconvert_flag_section_remove "")
set_property(TARGET bintools PROPERTY elfconvert_flag_gapfill "")
set_property(TARGET bintools PROPERTY elfconvert_flag_infile "")
set_property(TARGET bintools PROPERTY elfconvert_flag_outfile "")

# readelf for processing of elf files.
set_property(TARGET bintools PROPERTY readelf_command ${CMAKE_COMMAND} -E echo "readelf ${COMMAND_NOT_SUPPORTED} ${BINTOOLS}")
set_property(TARGET bintools PROPERTY readelf_flag "")
set_property(TARGET bintools PROPERTY readelf_flag_final "")
set_property(TARGET bintools PROPERTY readelf_flag_headers "")
set_property(TARGET bintools PROPERTY readelf_flag_infile "")
set_property(TARGET bintools PROPERTY readelf_flag_outfile "")

# strip command for stripping symbols
set_property(TARGET bintools PROPERTY strip_command ${CMAKE_COMMAND} -E echo "strip ${COMMAND_NOT_SUPPORTED} ${BINTOOLS}")
set_property(TARGET bintools PROPERTY strip_flag "")
set_property(TARGET bintools PROPERTY strip_flag_final "")
set_property(TARGET bintools PROPERTY strip_flag_all "")
set_property(TARGET bintools PROPERTY strip_flag_debug "")
set_property(TARGET bintools PROPERTY strip_flag_dwo "")
set_property(TARGET bintools PROPERTY strip_flag_infile "")
set_property(TARGET bintools PROPERTY strip_flag_outfile "")
