# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

include(${ZEPHYR_BASE}/cmake/modules/dts.cmake)

function(variant_dts_configuration_files)
  zephyr_get(DTS_SOURCE SYSBUILD LOCAL)
  set(dts_files ${DTS_SOURCE})

  if(EXTRA_DTC_OVERLAY_FILE)
    zephyr_list(TRANSFORM EXTRA_DTC_OVERLAY_FILE NORMALIZE_PATHS
      OUTPUT_VARIABLE EXTRA_DTC_OVERLAY_FILE_AS_LIST
    )
    build_info(devicetree extra-user-files PATH ${EXTRA_DTC_OVERLAY_FILE_AS_LIST})
    list(APPEND
      dts_files
      ${EXTRA_DTC_OVERLAY_FILE_AS_LIST}
    )
  endif()

  set(i 0)
  foreach(dts_file ${dts_files})
    if(i EQUAL 0)
      message(STATUS "Found BOARD.dts: ${dts_file}")
    else()
      message(STATUS "Found devicetree overlay: ${dts_file}")
    endif()

    math(EXPR i "${i}+1")
  endforeach()

  unset(DTS_ROOT_BINDINGS)
  foreach(dts_root ${DTS_ROOT})
    set(bindings_path ${dts_root}/dts/bindings)
    if(EXISTS ${bindings_path})
      list(APPEND
        DTS_ROOT_BINDINGS
        ${bindings_path}
      )
    endif()

    set(vendor_prefixes ${dts_root}/${VENDOR_PREFIXES})
    if(EXISTS ${vendor_prefixes})
      list(APPEND EXTRA_GEN_EDT_ARGS --vendor-prefixes ${vendor_prefixes})
    endif()

    set(DTS_ROOT_BINDINGS ${DTS_ROOT_BINDINGS} PARENT_SCOPE)
  endforeach()

  # Cache the location of the root bindings so they can be used by
  # scripts which use the build directory.
  set(CACHED_DTS_ROOT_BINDINGS ${DTS_ROOT_BINDINGS} CACHE INTERNAL
    "DT bindings root directories"
  )
  set(dts_files ${dts_files} PARENT_SCOPE)
  set(DTS_SOURCE ${DTS_SOURCE} PARENT_SCOPE)
  set(EXTRA_GEN_EDT_ARGS ${EXTRA_GEN_EDT_ARGS} PARENT_SCOPE)
endfunction()

macro(dts_init)
  variant_dts_configuration_files()
  dts_edt_pickle()
  dts_gen_defines()
  dts_gen_driver_kconfig()
  dts_import()
  dts_dtc()
  dts_build_info_output()
endmacro()
