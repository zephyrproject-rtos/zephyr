# Copyright (c) 2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

########################################################
# Linkable loadable extensions (llext)
########################################################
#
# These functions simplify the creation and management of linkable
# loadable extensions (llexts).
#

# Configuration functions
#
# The following functions simplify access to the compilation/link stage
# properties of an llext using the same API of the target_* functions.
#

function(llext_compile_definitions target_name)
  target_compile_definitions(${target_name}_llext_lib PRIVATE ${ARGN})
endfunction()

function(llext_compile_features target_name)
  target_compile_features(${target_name}_llext_lib PRIVATE ${ARGN})
endfunction()

function(llext_compile_options target_name)
  target_compile_options(${target_name}_llext_lib PRIVATE ${ARGN})
endfunction()

function(llext_include_directories target_name)
  target_include_directories(${target_name}_llext_lib PRIVATE ${ARGN})
endfunction()

function(llext_link_options target_name)
  target_link_options(${target_name}_llext_lib PRIVATE ${ARGN})
endfunction()

# Build control functions
#
# The following functions add targets and subcommands to the build system
# to compile and link an llext.
#

# Usage:
#   add_llext_target(<target_name>
#                    OUTPUT  <output_file>
#                    SOURCES <source_files>
#   )
#
# Add a custom target that compiles a set of source files to a .llext file.
#
# Output and source files must be specified using the OUTPUT and SOURCES
# arguments. Only one source file is supported when LLEXT_TYPE_ELF_OBJECT is
# selected, since there is no linking step in that case.
#
# The llext code will be compiled with mostly the same C compiler flags used
# in the Zephyr build, but with some important modifications. The list of
# flags to remove and flags to append is controlled respectively by the
# LLEXT_REMOVE_FLAGS and LLEXT_APPEND_FLAGS global variables.
#
# The following custom properties of <target_name> are defined and can be
# retrieved using the get_target_property() function:
#
# - lib_target  Target name for the source compilation and/or link step.
# - lib_output  The binary file resulting from compilation and/or
#               linking steps.
# - pkg_input   The file to be used as input for the packaging step.
# - pkg_output  The final .llext file.
#
# Example usage:
#   add_llext_target(hello_world
#     OUTPUT  ${PROJECT_BINARY_DIR}/hello_world.llext
#     SOURCES ${PROJECT_SOURCE_DIR}/src/llext/hello_world.c
#   )
# will compile the source file src/llext/hello_world.c to a file
# named "${PROJECT_BINARY_DIR}/hello_world.llext".
#
function(add_llext_target target_name)
  set(single_args OUTPUT)
  set(multi_args SOURCES)
  cmake_parse_arguments(PARSE_ARGV 1 LLEXT "${options}" "${single_args}" "${multi_args}")

  # Check that the llext subsystem is enabled for this build
  if (NOT CONFIG_LLEXT)
    message(FATAL_ERROR "add_llext_target: CONFIG_LLEXT must be enabled")
  endif()

  # Source and output files must be provided
  zephyr_check_arguments_required_all("add_llext_target" LLEXT OUTPUT SOURCES)

  list(LENGTH LLEXT_SOURCES source_count)
  if(CONFIG_LLEXT_TYPE_ELF_OBJECT AND NOT (source_count EQUAL 1))
    message(FATAL_ERROR "add_llext_target: only one source file is supported "
                        "for ELF object file builds")
  endif()

  set(llext_pkg_output ${LLEXT_OUTPUT})
  set(source_files ${LLEXT_SOURCES})

  set(zephyr_flags
      "$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>"
  )
  llext_filter_zephyr_flags(LLEXT_REMOVE_FLAGS ${zephyr_flags}
      zephyr_filtered_flags)

  # Compile the source file using current Zephyr settings but a different
  # set of flags to obtain the desired llext object type.
  set(llext_lib_target ${target_name}_llext_lib)
  if(CONFIG_LLEXT_TYPE_ELF_OBJECT)

    # Create an object library to compile the source file
    add_library(${llext_lib_target} OBJECT ${source_files})
    set(llext_lib_output $<TARGET_OBJECTS:${llext_lib_target}>)

  elseif(CONFIG_LLEXT_TYPE_ELF_RELOCATABLE)

    # CMake does not directly support a "RELOCATABLE" library target.
    # The "SHARED" target would be similar, but that unavoidably adds
    # a "-shared" flag to the linker command line which does firmly
    # conflict with "-r".
    # A workaround is to use an executable target and make the linker
    # output a relocatable file. The output file suffix is changed so
    # the result looks like the object file it actually is.
    add_executable(${llext_lib_target} EXCLUDE_FROM_ALL ${source_files})
    target_link_options(${llext_lib_target} PRIVATE
      $<TARGET_PROPERTY:linker,partial_linking>)
    set_target_properties(${llext_lib_target} PROPERTIES
      SUFFIX ${CMAKE_C_OUTPUT_EXTENSION})
    set(llext_lib_output $<TARGET_FILE:${llext_lib_target}>)

    # Add the llext flags to the linking step as well
    target_link_options(${llext_lib_target} PRIVATE
      ${LLEXT_APPEND_FLAGS}
    )

  elseif(CONFIG_LLEXT_TYPE_ELF_SHAREDLIB)

    # Create a shared library
    add_library(${llext_lib_target} SHARED ${source_files})
    set(llext_lib_output $<TARGET_FILE:${llext_lib_target}>)

    # Add the llext flags to the linking step as well
    target_link_options(${llext_lib_target} PRIVATE
      ${LLEXT_APPEND_FLAGS}
    )

  endif()

  target_compile_definitions(${llext_lib_target} PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
    LL_EXTENSION_BUILD
  )
  target_compile_options(${llext_lib_target} PRIVATE
    ${zephyr_filtered_flags}
    ${LLEXT_APPEND_FLAGS}
  )
  target_include_directories(${llext_lib_target} PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
  )
  target_include_directories(${llext_lib_target} SYSTEM PUBLIC
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
  )
  add_dependencies(${llext_lib_target}
    zephyr_interface
    zephyr_generated_headers
  )

  # Set up an intermediate processing step between compilation and packaging
  # to be used to support POST_BUILD commands on targets that do not use a
  # dynamic library.
  set(llext_proc_target ${target_name}_llext_proc)
  set(llext_pkg_input ${PROJECT_BINARY_DIR}/${target_name}.llext.pkg_input)
  add_custom_target(${llext_proc_target} DEPENDS ${llext_pkg_input})
  set_property(TARGET ${llext_proc_target} PROPERTY has_post_build_cmds 0)

  # By default this target must copy the `lib_output` binary file to the
  # expected `pkg_input` location. If actual POST_BUILD commands are defined,
  # they will take care of this and the default copy is replaced by a no-op.
  set(has_post_build_cmds "$<TARGET_PROPERTY:${llext_proc_target},has_post_build_cmds>")
  set(noop_cmd ${CMAKE_COMMAND} -E true)
  set(copy_cmd ${CMAKE_COMMAND} -E copy ${llext_lib_output} ${llext_pkg_input})
  add_custom_command(
    OUTPUT ${llext_pkg_input}
    COMMAND "$<IF:${has_post_build_cmds},${noop_cmd},${copy_cmd}>"
    DEPENDS ${llext_lib_target} ${llext_lib_output}
    COMMAND_EXPAND_LISTS
  )

  # LLEXT ELF processing for importing via SLID
  #
  # This command must be executed as last step of the packaging process,
  # to ensure that the ELF processed for binary generation contains SLIDs.
  # If executed too early, it is possible that some tools executed to modify
  # the ELF file (e.g., strip) undo the work performed here.
  if (CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID)
    set(slid_inject_cmd
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/llext_inject_slids.py
      --elf-file ${llext_pkg_output}
    )
  else()
    set(slid_inject_cmd ${CMAKE_COMMAND} -E true)
  endif()

  # Type-specific packaging of the built binary file into an .llext file
  if(CONFIG_LLEXT_TYPE_ELF_OBJECT)

    # No packaging required, simply copy the object file
    add_custom_command(
      OUTPUT ${llext_pkg_output}
      COMMAND ${CMAKE_COMMAND} -E copy ${llext_pkg_input} ${llext_pkg_output}
      COMMAND ${slid_inject_cmd}
      DEPENDS ${llext_proc_target} ${llext_pkg_input}
    )

  elseif(CONFIG_LLEXT_TYPE_ELF_RELOCATABLE)

    # Need to remove just some sections from the relocatable object
    # (using strip in this case would remove _all_ symbols)
    add_custom_command(
      OUTPUT ${llext_pkg_output}
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.xt.*
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${llext_pkg_input}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${llext_pkg_output}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      COMMAND ${slid_inject_cmd}
      DEPENDS ${llext_proc_target} ${llext_pkg_input}
    )

  elseif(CONFIG_LLEXT_TYPE_ELF_SHAREDLIB)

    # Need to strip the shared library of some sections
    add_custom_command(
      OUTPUT ${llext_pkg_output}
      COMMAND $<TARGET_PROPERTY:bintools,strip_command>
              $<TARGET_PROPERTY:bintools,strip_flag>
              $<TARGET_PROPERTY:bintools,strip_flag_remove_section>.xt.*
              $<TARGET_PROPERTY:bintools,strip_flag_infile>${llext_pkg_input}
              $<TARGET_PROPERTY:bintools,strip_flag_outfile>${llext_pkg_output}
              $<TARGET_PROPERTY:bintools,strip_flag_final>
      COMMAND ${slid_inject_cmd}
      DEPENDS ${llext_proc_target} ${llext_pkg_input}
    )

  endif()

  # Add user-visible target and dependency, and fill in properties
  get_filename_component(output_name ${llext_pkg_output} NAME)
  add_custom_target(${target_name}
    COMMENT "Generating ${output_name}"
    DEPENDS ${llext_pkg_output}
  )
  set_target_properties(${target_name} PROPERTIES
    lib_target ${llext_lib_target}
    lib_output ${llext_lib_output}
    pkg_input  ${llext_pkg_input}
    pkg_output ${llext_pkg_output}
  )
endfunction()

# Usage:
#   add_llext_command(
#     TARGET <target_name>
#     PRE_BUILD | POST_BUILD | POST_PKG
#     COMMAND <command> [...]
#   )
#
# Add a custom command to an llext target that will be executed during
# the build. The command will be executed at the specified build step and
# can refer to <target>'s properties for build-specific details.
#
# The different build steps are:
# - PRE_BUILD:  Before the llext code is linked, if the architecture uses
#               dynamic libraries. This step can access `lib_target` and
#               its own properties.
# - POST_BUILD: After the llext code is built, but before packaging
#               it in an .llext file. This step is expected to create a
#               `pkg_input` file by reading the contents of `lib_output`.
# - POST_PKG:   After the .llext file has been created. This can operate on
#               the final llext file `pkg_output`.
#
# Anything else after COMMAND will be passed to add_custom_command() as-is
# (including multiple commands and other options).
function(add_llext_command)
  set(options     PRE_BUILD POST_BUILD POST_PKG)
  set(single_args TARGET)
  # COMMAND and other options are passed to add_custom_command() as-is

  cmake_parse_arguments(PARSE_ARGV 0 LLEXT "${options}" "${single_args}" "${multi_args}")
  zephyr_check_arguments_required_all("add_llext_command" LLEXT TARGET)

  # Check the target exists and refers to an llext target
  set(target_name ${LLEXT_TARGET})
  set(llext_lib_target  ${target_name}_llext_lib)
  set(llext_proc_target ${target_name}_llext_proc)
  if(NOT TARGET ${llext_lib_target})
    message(FATAL_ERROR "add_llext_command: not an llext target: ${target_name}")
  endif()

  # ARM uses an object file representation so there is no link step.
  if(CONFIG_ARM AND LLEXT_PRE_BUILD)
    message(FATAL_ERROR
	    "add_llext_command: PRE_BUILD not supported on this arch")
  endif()

  # Determine the build step and the target to attach the command to
  # based on the provided options
  if(LLEXT_PRE_BUILD)
    # > before the object files are linked:
    #   - execute user command(s) before the lib target's link step.
    set(cmd_target ${llext_lib_target})
    set(build_step PRE_LINK)
  elseif(LLEXT_POST_BUILD)
    # > after linking, but before llext packaging:
    #   - stop default file copy to prevent user files from being clobbered;
    #   - execute user command(s) after the (now empty) `llext_proc_target`.
    set_property(TARGET ${llext_proc_target} PROPERTY has_post_build_cmds 1)
    set(cmd_target ${llext_proc_target})
    set(build_step POST_BUILD)
  elseif(LLEXT_POST_PKG)
    # > after the final llext binary is ready:
    #   - execute user command(s) after the main target is done.
    set(cmd_target ${target_name})
    set(build_step POST_BUILD)
  else()
    message(FATAL_ERROR "add_llext_command: build step must be provided")
  endif()

  # Check that the first unparsed argument is the word COMMAND
  list(GET LLEXT_UNPARSED_ARGUMENTS 0 command_str)
  if(NOT command_str STREQUAL "COMMAND")
    message(FATAL_ERROR "add_llext_command: COMMAND argument must be provided")
  endif()

  # Add the actual command(s) to the target
  add_custom_command(
    TARGET ${cmd_target} ${build_step}
    ${LLEXT_UNPARSED_ARGUMENTS}
    COMMAND_EXPAND_LISTS
  )
endfunction()

# llext helper functions

# Usage:
#   llext_filter_zephyr_flags(<filter> <flags> <outvar>)
#
# Filter out flags from a list of flags. The filter is a list of regular
# expressions that will be used to exclude flags from the input list.
#
# The resulting generator expression will be stored in the variable <outvar>.
#
# Example:
#   llext_filter_zephyr_flags(LLEXT_REMOVE_FLAGS zephyr_flags zephyr_filtered_flags)
#
function(llext_filter_zephyr_flags filter flags outvar)
  list(TRANSFORM ${filter}
       REPLACE "(.+)" "^\\1$"
       OUTPUT_VARIABLE llext_remove_flags_regexp
  )
  list(JOIN llext_remove_flags_regexp "|" llext_remove_flags_regexp)
  if ("${llext_remove_flags_regexp}" STREQUAL "")
    # an empty regexp would match anything, we actually need the opposite
    # so set it to match empty strings
    set(llext_remove_flags_regexp "^$")
  endif()

  set(zephyr_filtered_flags
      "$<FILTER:${flags},EXCLUDE,${llext_remove_flags_regexp}>"
  )

  set(${outvar} ${zephyr_filtered_flags} PARENT_SCOPE)
endfunction()
