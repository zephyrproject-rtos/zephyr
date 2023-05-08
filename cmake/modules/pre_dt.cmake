# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, 2023 Nordic Semiconductor ASA

include_guard(GLOBAL)
include(extensions)

# Finalize the value of DTS_ROOT, so we know where all our
# DTS files, bindings, and vendor prefixes are.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - CMAKE_DTS_PREPROCESSOR: the path to the preprocessor to use
#   for devicetree files
# - DTS_ROOT: a deduplicated list of places where devicetree
#   implementation files (like bindings, vendor prefixes, etc.) are
#   found
# - DTS_ROOT_SYSTEM_INCLUDE_DIRS: set to "PATH1 PATH2 ...",
#   with one path per potential location where C preprocessor #includes
#   may be found for devicetree files
#
# Required variables:
# None.
#
# Optional variables:
# - APPLICATION_SOURCE_DIR: path to app (added to DTS_ROOT)
# - BOARD_DIR: directory containing the board definition (added to DTS_ROOT)
# - DTS_ROOT: initial contents may be populated here
# - ZEPHYR_BASE: path to zephyr repository (added to DTS_ROOT)
# - SHIELD_DIRS: paths to shield definitions (added to DTS_ROOT)

# Using a function avoids polluting the parent scope unnecessarily.
function(pre_dt_module_run)
  # Convert relative paths to absolute paths relative to the application
  # source directory.
  zephyr_file(APPLICATION_ROOT DTS_ROOT)

  # DTS_ROOT always includes the application directory, the board
  # directory, shield directories, and ZEPHYR_BASE.
  list(APPEND
    DTS_ROOT
    ${APPLICATION_SOURCE_DIR}
    ${BOARD_DIR}
    ${SHIELD_DIRS}
    ${ZEPHYR_BASE}
    )

  # Convert the directories in DTS_ROOT to absolute paths without
  # symlinks.
  #
  # DTS directories can come from multiple places. Some places, like a
  # user's CMakeLists.txt can preserve symbolic links. Others, like
  # scripts/zephyr_module.py --settings-out resolve them.
  unset(real_dts_root)
  foreach(dts_dir ${DTS_ROOT})
    file(REAL_PATH ${dts_dir} real_dts_dir)
    list(APPEND real_dts_root ${real_dts_dir})
  endforeach()
  set(DTS_ROOT ${real_dts_root})

  # Finalize DTS_ROOT.
  list(REMOVE_DUPLICATES DTS_ROOT)

  # Finalize DTS_ROOT_SYSTEM_INCLUDE_DIRS.
  set(DTS_ROOT_SYSTEM_INCLUDE_DIRS)
  foreach(dts_root ${DTS_ROOT})
    foreach(dts_root_path
        include
        include/zephyr
        dts/common
        dts/${ARCH}
        dts
        )
      get_filename_component(full_path ${dts_root}/${dts_root_path} REALPATH)
      if(EXISTS ${full_path})
        list(APPEND DTS_ROOT_SYSTEM_INCLUDE_DIRS ${full_path})
      endif()
    endforeach()
  endforeach()

  # Set output variables.
  set(DTS_ROOT ${DTS_ROOT} PARENT_SCOPE)
  set(DTS_ROOT_SYSTEM_INCLUDE_DIRS ${DTS_ROOT_SYSTEM_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

pre_dt_module_run()
