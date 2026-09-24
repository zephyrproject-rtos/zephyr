# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

# Build-time patching of the SMIF external-flash CMSIS flash-loader (.FLM).
#
# With -DPATCH_FLM=ON the base flash-loader, fetched as a hal_infineon blob,
# is rewritten during the build with the external-flash geometry taken from
# the board's `infineon,smif-nor` devicetree node (command set, sector map,
# hybrid regions and timings). OpenOCD is then redirected to the patched
# loader through its script search path, so 'west flash' programs the external
# exactly as the device on the board requires.

include_guard(GLOBAL)

set(PATCH_FLM OFF CACHE BOOL
    "Patch the SMIF flash-loader from devicetree before flashing")

# Optional -D overrides so a custom device can be targeted without editing a
# board file. When empty, the values from the board's call are used.
set(PATCH_FLM_DT_NODE "" CACHE STRING
    "Override the infineon,smif-nor devicetree node label used by PATCH_FLM")
set(PATCH_FLM_SLOT "" CACHE STRING
    "Override the SMIF slave slot / chip-select index used by PATCH_FLM")
set(PATCH_FLM_MODE "" CACHE STRING
    "Override the PATCH_FLM mode: sfdp (default) or static")

# infineon_patch_flm(<dt-node> <slot> [mode])
#
#   dt-node  Label of the `infineon,smif-nor` node to describe, e.g. ext_flash.
#   slot     SMIF slave slot in the flash-loader to populate; this is the
#            device's chip-select index.
#   mode     Optional: "sfdp" (default) patches the slot for SFDP runtime
#            discovery (chip-select/base/size + DETECT_SFDP), leaving the
#            command set to the device; "static" writes the full command set
#            from devicetree (only valid for configurator-generated FLMs whose
#            deviceCfg pointers are already wired).
#
# Each argument is a board default; override at configure time with
# -DPATCH_FLM_DT_NODE=, -DPATCH_FLM_SLOT= or -DPATCH_FLM_MODE=.
function(infineon_patch_flm dt_node slot)
  if(NOT PATCH_FLM)
    return()
  endif()

  # Resolve node / slot / mode: a cache-var override wins over the board default.
  set(node "${dt_node}")
  if(NOT PATCH_FLM_DT_NODE STREQUAL "")
    set(node "${PATCH_FLM_DT_NODE}")
  endif()

  set(sel "${slot}")
  if(NOT PATCH_FLM_SLOT STREQUAL "")
    set(sel "${PATCH_FLM_SLOT}")
  endif()

  set(mode "sfdp")
  if(${ARGC} GREATER 2)
    set(mode "${ARGV2}")
  endif()
  if(NOT PATCH_FLM_MODE STREQUAL "")
    set(mode "${PATCH_FLM_MODE}")
  endif()

  set(mode_arg "")
  if(mode STREQUAL "sfdp")
    set(mode_arg --sfdp)
  endif()

  set(flm_script "${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr/scripts/flm_smif.py")
  set(flm_base
      "${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr/blobs/flashloader/PSE84/PSE84_SMIF.FLM")
  set(flm_dts "${BOARD_DIR}/${BOARD}_memory_map.dtsi")
  set(flm_out "${CMAKE_BINARY_DIR}/PSE84_SMIF.FLM")

  if(NOT EXISTS "${flm_base}")
    message(WARNING
      "PATCH_FLM is set but the base flash-loader is missing:\n"
      "  ${flm_base}\n"
      "Run 'west blobs fetch hal_infineon' to download it. Falling back to the "
      "default OpenOCD flash-loader.")
    return()
  endif()

  add_custom_command(
    OUTPUT "${flm_out}"
    COMMAND "${PYTHON_EXECUTABLE}" "${flm_script}" patch "${flm_base}"
            --cfg-dt "${flm_dts}" --dt-node "${node}" --slot "${sel}"
            ${mode_arg} -o "${flm_out}"
    DEPENDS "${flm_base}" "${flm_script}" "${flm_dts}"
    COMMENT "Patching SMIF flash-loader (${mode}) for '${node}' (slot ${sel}) -> ${flm_out}"
    VERBATIM)

  add_custom_target(patch_flm ALL DEPENDS "${flm_out}")

  # Point OpenOCD at the patched loader. The board's SMIF flash bank reads its
  # ELF path from the $QSPI_FLASHLOADER Tcl variable (with a guarded default);
  # this override sets it to the patched loader and is sourced before the board
  # config, so it takes precedence.
  set(flm_override "${CMAKE_BINARY_DIR}/flm_override.cfg")
  file(WRITE "${flm_override}" "set QSPI_FLASHLOADER \"${flm_out}\"\n")
  board_runner_args(openocd "--config=${flm_override}")
  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")
endfunction()
