# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Configure SoC settings based on Kconfig settings.
#
# This CMake module will set the following variables in the build system based
# on Kconfig settings for the selected SoC.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - SOC_NAME:   Name of the SoC in use, identical to CONFIG_SOC
# - SOC_SERIES: Name of the SoC series in use, identical to CONFIG_SOC_SERIES
# - SOC_FAMILY: Name of the SoC family, identical to CONFIG_SOC_FAMILY
#
# Variables set by this module and not mentioned above are considered internal
# use only and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

include(kconfig)

if(HWMv2)
  # 'SOC_ROOT' is a prioritized list of directories where socs may be
  # found. It always includes ${ZEPHYR_BASE}/soc at the lowest priority.
  list(APPEND SOC_ROOT ${ZEPHYR_BASE})

  # Use SOC to search for a 'CMakeLists.txt' file.
  # e.g. zephyr/soc/xtensa/intel_adsp/CMakeLists.txt.
  foreach(root ${SOC_ROOT})
    # Check that the root looks reasonable.
    if(NOT IS_DIRECTORY "${root}/soc")
      message(WARNING "\nSOC_ROOT element(s) without a 'soc' subdirectory:
  ${root}
  Hints:
    - if your SoC family directory is '/foo/bar/soc/vendor/my_soc_family', then add '/foo/bar' to SOC_ROOT, not the entire SoC family path
    - if in doubt, use absolute paths\n")
    endif()

    if(EXISTS ${root}/soc)
      set(SOC_DIR ${root}/soc)
      break()
    endif()
  endforeach()

  if(NOT SOC_DIR)
    message(FATAL_ERROR "Could not find SOC=${SOC_NAME} for BOARD=${BOARD},\n"
            "please check your installation.\n"
            "SOC roots searched:\n"
            "${SOC_ROOT}\n"
    )
  endif()

  # Split SOC_FAMILY to obtain the path for specific soc
  # Example: soc_family = nxp_imx or nxp-imx => PREFIX_DIR=/nxp/imx
  string(REGEX REPLACE "^([a-zA-Z0-9]+)[_\-]([a-zA-Z0-9]+)$" "\\1/\\2" PREFIX_DIR ${CONFIG_SOC_FAMILY})

  set(SOC_NAME   ${CONFIG_SOC})
  set(SOC_SERIES ${CONFIG_SOC_SERIES})
  set(SOC_TOOLCHAIN_NAME ${CONFIG_SOC_TOOLCHAIN_NAME})
  set(SOC_FAMILY ${CONFIG_SOC_FAMILY})
  set(SOC_V2_DIR ${SOC_${SOC_NAME}_DIR})
  set(SOC_FULL_DIR ${SOC_DIR}/${PREFIX_DIR}/${CONFIG_SOC_SERIES})

  if(NOT IS_DIRECTORY ${SOC_FULL_DIR})
    message(WARNING "Could not find a proper SOC_FAMILY=${CONFIG_SOC_FAMILY} for BOARD=${BOARD},\n"
          "please check your SOC_FAMILY.\n"
          "Is expected to be like vendor[_\-]soc_family.\n"
  )
  endif()

endif()
