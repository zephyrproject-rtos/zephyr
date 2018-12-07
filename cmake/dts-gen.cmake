file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/generated)

# Zephyr code can configure itself based on a KConfig'uration with the
# header file autoconf.h. There exists an analogous file
# generated_dts_board.h that allows configuration based on information
# encoded in DTS.
#
# Here we call on dtc, the gcc preprocessor, and
# scripts/dts/extract_dts_includes.py to generate this header file at
# CMake configure-time.

# Parse boards/shields of each board root to generate the shield list
foreach(board_root ${BOARD_ROOT})
  set(shield_dir ${board_root}/boards/shields)

  # Match the .overlay files in the shield directories to make sure we are
  # finding shields, e.g. x_nucleo_iks01a1/x_nucleo_iks01a1.overlay
  file(GLOB_RECURSE shields_refs_list
    RELATIVE ${shield_dir}
    ${shield_dir}/*/*.overlay
    )

  # The above gives a list like
  # x_nucleo_iks01a1/x_nucleo_iks01a1.overlay;x_nucleo_iks01a2/x_nucleo_iks01a2.overlay
  # we construct a list of shield names by extracting file name and
  # removing the extension.
  foreach(shield_path ${shields_refs_list})
	get_filename_component(shield ${shield_path} NAME_WE)

	# Generate CONFIG flags matching each shield
	string(TOUPPER "CONFIG_SHIELD_${shield}" shield_config)

	if(${shield_config})
      # if shield config flag is on, add shield overlay to the shield overlays
      # list and dts_fixup file to the shield fixup file
      list(APPEND
		dts_files
		${shield_dir}/${shield_path}
		)
      list(APPEND
		dts_fixups
		${shield_dir}/${shield}/dts_fixup.h
		)
	endif()
  endforeach()
endforeach()

if(SUPPORTS_DTS)
  if(DTC_OVERLAY_FILE)
    # Convert from space-separated files into file list
    string(REPLACE " " ";" DTC_OVERLAY_FILE_AS_LIST ${DTC_OVERLAY_FILE})
    list(APPEND
      dts_files
      ${DTC_OVERLAY_FILE_AS_LIST}
      )
  endif()

  set(i 0)
  unset(DTC_INCLUDE_FLAG_FOR_DTS)
  foreach(dts_file ${dts_files})
    list(APPEND DTC_INCLUDE_FLAG_FOR_DTS
         -include ${dts_file})

	if(i EQUAL 0)
	  message(STATUS "Loading ${dts_file} as base")
	else()
	  message(STATUS "Overlaying ${dts_file}")
	endif()

	math(EXPR i "${i}+1")
  endforeach()

  # TODO: Cut down on CMake configuration time by avoiding
  # regeneration of generated_dts_board.h on every configure. How
  # challenging is this? What are the dts dependencies? We run the
  # preprocessor, and it seems to be including all kinds of
  # directories with who-knows how many header files.

  # Error-out when the deprecated naming convention is found (until
  # after 1.14.0 has been released)
  foreach(path
	  ${BOARD_DIR}/dts.fixup
	  ${PROJECT_SOURCE_DIR}/soc/${ARCH}/${SOC_PATH}/dts.fixup
      ${APPLICATION_SOURCE_DIR}/dts.fixup
	  )
	if(EXISTS ${path})
	  message(FATAL_ERROR
		"A deprecated filename has been detected. Porting is required."
		"The file '${path}' exists, but it should be named dts_fixup.h instead."
		"See https://github.com/zephyrproject-rtos/zephyr/pull/10352 for more details"
		)
	endif()
  endforeach()

  # Run extract_dts_includes.py for the header file
  # generated_dts_board.h
  set_ifndef(DTS_BOARD_FIXUP_FILE ${BOARD_DIR}/dts_fixup.h)
  set_ifndef(DTS_SOC_FIXUP_FILE   ${SOC_DIR}/${ARCH}/${SOC_PATH}/dts_fixup.h)

  list(APPEND dts_fixups
    ${DTS_BOARD_FIXUP_FILE}
    ${DTS_SOC_FIXUP_FILE}
    ${APPLICATION_SOURCE_DIR}/dts_fixup.h
    )

  foreach(fixup ${dts_fixups})
    if(EXISTS ${fixup})
      list(APPEND existing_dts_fixups ${fixup})
    endif()
  endforeach()

  if("${existing_dts_fixups}" STREQUAL "")
    unset(DTS_FIXUPS_WITH_FLAG)
  else()
    set(DTS_FIXUPS_WITH_FLAG --fixup ${existing_dts_fixups})
  endif()

  set(CMD_EXTRACT_DTS_INCLUDES ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/dts/extract_dts_includes.py
    --dts ${BOARD}.dts_compiled
    --yaml ${ZEPHYR_BASE}/dts/bindings ${DTS_APP_BINDINGS}
    ${DTS_FIXUPS_WITH_FLAG}
    --include ${GENERATED_DTS_BOARD_H}
    --old-alias-names
    )

  # Run extract_dts_includes.py to create a .conf and a header file that can be
  # included into the CMake namespace
  execute_process(
    COMMAND ${CMD_EXTRACT_DTS_INCLUDES}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  if(NOT "${ret}" STREQUAL "0")
    message(FATAL_ERROR "command failed with return code: ${ret}")
  endif()

else()
  file(WRITE ${GENERATED_DTS_BOARD_H} "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */")
endif(SUPPORTS_DTS)
