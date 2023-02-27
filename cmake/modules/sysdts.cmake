# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

include(extensions)
include(python)
include(pre_dt)
find_package(HostTools)

# This file is used for processing a system devicetree (.sysdts)
# into a regular devicetree (.dts) for use by the rest of the build
# system.
#
# Outcome:
#
# The pre_dt module will have been included; see its outcome comment
# for additional configuration options.
#
# The following variables will or may be set by this module:
#
# - SYSDTS_FOUND: will be set to TRUE or FALSE depending on
#   a system devicetree was found.
# - DTS_SOURCE: if SYSDTS_FOUND is TRUE, set to the path
#   to the generated .dts file or files to use in the place of
#   BOARD.dts
# - BOARD_DEFCONFIG: may be set to a defconfig file to use
#   for the current execution domain, instead of the usual
#   ${BOARD}_defconfig file (since such files don't make sense
#   semantically in a system DT world where there are notionally
#   multiple "default" configurations per board, at least one per
#   CPU cluster).
#
# Required variables:
# - SYSDT_EXECUTION_DOMAIN: may be provided via any means
#   that zephyr_get(... SYSBUILD LOCAL) can find. The full
#   path to the system DT node that defines the execution domain
#   to use to generate the regular devicetree. The path should
#   look like "/domains/foo".
#
# Optional variables:
#
# - DTS_EXTRA_CPPFLAGS: extra command line options to pass to the
#   C preprocessor when generating the devicetree from DTS_SOURCE
# - SYSDT_OVERLAY_FILE: a whitespace- or semicolon-separated
#   list of system devicetree files (usually with .sysoverlay
#   extensions) to append to the board's system devicetree (.sysdts)

# Any system devicetree overlays are specified here for now.
zephyr_get(SYSDT_OVERLAY_FILE SYSBUILD LOCAL)

# The directory containing devicetree related scripts.
set(DT_SCRIPTS                  ${ZEPHYR_BASE}/scripts/dts)
# This converts system devicetree to a generated "regular" devicetree include file.
set(GEN_DTSI_FROM_SYSDTS_SCRIPT ${DT_SCRIPTS}/gen_dtsi_from_sysdts.py)
# The generated regular devicetree include file which is created from
# a system devicetree.
set(SYSDT_OUTPUT_DTS            ${BINARY_DIR_INCLUDE_GENERATED}/sysdt_output.dts)
# Generated build system internals.
set(SYSDT_POST_CPP              ${PROJECT_BINARY_DIR}/zephyr.sysdts.pre)
set(SYSDT_DEPS                  ${PROJECT_BINARY_DIR}/zephyr.sysdts.d)
# The sysdtlib.SysDT object in pickle format.
set(SYSDT_PICKLE                ${PROJECT_BINARY_DIR}/sysdt.pickle)
# Where we would expect to find a system devicetree.
# TODO: what about board revisions?
set(BOARD_SYSDTS ${BOARD_DIR}/${BOARD}.sysdts)

# See if we have a system devicetree, and bail if we don't.
if (EXISTS ${BOARD_SYSDTS})
  set(SYSDTS_FOUND TRUE)
  message(STATUS "Found BOARD.sysdts: ${BOARD_SYSDTS}")
else()
  set(SYSDTS_FOUND FALSE)
  return()
endif()

#
# Validate requirements.
#

zephyr_get(SYSDT_EXECUTION_DOMAIN SYSBUILD LOCAL)
if(NOT ("${SYSDT_EXECUTION_DOMAIN}" MATCHES "^/domains/"))
  message(FATAL_ERROR "Found system devicetree file ${BOARD_SYSDTS}, but SYSDT_EXECUTION_DOMAIN=\"${SYSDT_EXECUTION_DOMAIN}\" is not set to the path to a node in /domains. Set this variable to the path to the execution domain in your system devicetree that you wish to use.")
else()
  message(STATUS "System DT execution domain: ${SYSDT_EXECUTION_DOMAIN}")
endif()

zephyr_get(DTS_SOURCE SYSBUILD LOCAL)
if(DEFINED DTS_SOURCE)
  message(FATAL_ERROR "do not define DTS_SOURCE; we have a system devicetree")
endif()

#
# Generate SYSDT_OUTPUT_DTS.
#

set(SYSDT_INCLUDE_ARGS -include ${BOARD_SYSDTS}) # Include BOARD.sysdts.
if(SYSDT_OVERLAY_FILE)
  zephyr_dts_normalize_overlay_list(SYSDT_OVERLAY_FILE SYSDT_OVERLAY_FILE_AS_LIST)
  foreach(sysdt_overlay_file ${SYSDT_OVERLAY_FILE_AS_LIST})
    if(NOT sysdt_overlay_file MATCHES ".*[.]sysoverlay$")
      # People are probably going to get confused about file extensions,
      # so try to be helpful by establishing conventions.
      message(WARNING "System devicetree overlay does not have .sysoverlay extension: ${sysdt_overlay_file}")
    endif()
    if(EXISTS "${sysdt_overlay_file}")
      message(STATUS "Found system devicetree overlay: ${sysdt_overlay_file}")
    else()
      message(FATAL_ERROR "System devicetree overlay not found: ${sysdt_overlay_file}")
    endif()
  endforeach()
endif()

if(DEFINED CMAKE_DTS_PREPROCESSOR)
  set(dts_preprocessor ${CMAKE_DTS_PREPROCESSOR})
else()
  set(dts_preprocessor ${CMAKE_C_COMPILER})
endif()
set(source_files ${BOARD_SYSDTS} ${SYSDT_OVERLAY_FILE_AS_LIST})
zephyr_dt_preprocess(
  CPP ${dts_preprocessor}
  SOURCE_FILES ${source_files}
  OUT_FILE ${SYSDT_POST_CPP}
  DEPS_FILE ${SYSDT_DEPS}
  EXTRA_CPPFLAGS ${DTS_EXTRA_CPPFLAGS}
  INCLUDE_DIRECTORIES ${DTS_ROOT_SYSTEM_INCLUDE_DIRS}
  WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
  )

#
# Generate sysdt_output.dts.
#

execute_process(COMMAND ${PYTHON_EXECUTABLE} ${GEN_DTSI_FROM_SYSDTS_SCRIPT}
  --sysdts "${SYSDT_POST_CPP}"
  --domain "${SYSDT_EXECUTION_DOMAIN}"
  --sysdt-pickle-out "${SYSDT_PICKLE}"
  --dts-out "${SYSDT_OUTPUT_DTS}"
  RESULT_VARIABLE ret
  )
if(NOT ret EQUAL 0)
  message(FATAL_ERROR "Failed to generate .dts from system devicetree file(s) (error code ${ret}): ${source_files}")
endif()
message(STATUS "Generated DTS from BOARD.sysdts: ${SYSDT_OUTPUT_DTS}")

#
# Make sure we re-run CMake if anything we know we depend on
# has changed.
#

unset(sysdt_deps)
toolchain_parse_make_rule(${SYSDT_DEPS} sysdt_deps)
set_property(DIRECTORY APPEND PROPERTY
  CMAKE_CONFIGURE_DEPENDS
  ${sysdt_deps}
  ${GEN_DTSI_FROM_SYSDTS_SCRIPT}
  )

# HACK: what's a more scalable way to find a domain defconfig?
set(BOARD_DEFCONFIG ${BOARD_DIR}${SYSDT_EXECUTION_DOMAIN}_defconfig)

# HACK: what's a more scalable way to establish domain overlays?
set(DTS_SOURCE ${SYSDT_OUTPUT_DTS})
set(domain_overlay ${BOARD_DIR}${SYSDT_EXECUTION_DOMAIN}.overlay)
if(EXISTS ${domain_overlay})
  list(APPEND DTS_SOURCE ${domain_overlay})
endif()
