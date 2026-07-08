# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

# Adds ext-boot metadata header to a PSE84 CM33 secure hex image using imgtool.
# Derives partition addresses and sizes from the devicetree.
#
# When CONFIG_MCUBOOT is set (building MCUBoot itself), uses boot_partition
# for address/size and ROM_START_OFFSET for header size. The header space is
# pre-reserved in the ELF so --pad-header is not used.
#
# Apps chain-loaded by MCUBoot (slot0/slot2) do NOT use this function —
# their signing is handled by cmake/mcuboot.cmake.
#
# Otherwise (standalone secure image or TF-M SPE), uses m33s_xip with a
# fixed header size of 0x400. The header is separate from the code
# partition so --pad-header is used to prepend it.
#
# Usage: pse84_add_extboot_metadata(<input_hex> <output_hex> [PAD_HEADER])
#
# PAD_HEADER — pass --pad-header to imgtool. Use this when the input image
#   does not pre-reserve the header area with zeros (e.g. TF-M SPE binary
#   built by TF-M's own build system). Zephyr images that use
#   ROM_START_OFFSET already have the space reserved and do NOT need this.
function(pse84_add_extboot_metadata INPUT_FILE OUTPUT_FILE)
  cmake_parse_arguments(ARG "PAD_HEADER" "" "" ${ARGN})

  if(CONFIG_MCUBOOT)
    # MCUBoot bootloader image: use boot_partition, header from ROM_START_OFFSET.
    dt_nodelabel(part_node NODELABEL "boot_partition")
    dt_reg_addr(part_addr PATH ${part_node})
    dt_reg_size(slot_size PATH ${part_node})
    set(header_size ${CONFIG_ROM_START_OFFSET})
  else()
    # Standalone secure image or TF-M SPE: use m33s_xip partition.
    #
    # The header size is fixed at 0x400.
    # CONFIG_ROM_START_OFFSET cannot be used here because in NS-side
    # TF-M builds this function runs from the NS application context
    # where ROM_START_OFFSET reflects the NS image, not the SPE.
    dt_nodelabel(part_node NODELABEL "m33s_xip")
    dt_reg_addr(part_addr PATH ${part_node})
    dt_reg_size(slot_size PATH ${part_node})
    set(header_size 0x400)
  endif()

  if(ARG_PAD_HEADER)
    set(pad_header_arg "--pad-header")
  else()
    set(pad_header_arg "")
  endif()

  # With zephyr,mapped-partition, dt_reg_addr already returns the absolute
  # address (translated through the parent's `ranges`), so no flash base
  # needs to be added. Partitions in the CBUS-S address space still need
  # BUILD_OUTPUT_ADJUST_LMA on top to rebase to the SAHB-S programming
  # alias used by the flasher; RRAM partitions do not need this.
  if(CONFIG_BUILD_OUTPUT_ADJUST_LMA)
    math(EXPR hex_addr "${part_addr} + ${CONFIG_BUILD_OUTPUT_ADJUST_LMA}" OUTPUT_FORMAT HEXADECIMAL)
  else()
    set(hex_addr ${part_addr})
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
      --header-size ${header_size} --erased-val 0xff ${pad_header_arg}
      --slot-size ${slot_size} --hex-addr ${hex_addr}
      ${INPUT_FILE} ${OUTPUT_FILE}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${OUTPUT_FILE})
endfunction()

# Signs a PSE84 TF-M SPE image (MCUboot image 0) so the MCUboot bootloader
# validates it with the same key and parameters it uses for the chain-loaded
# Non-Secure image.
#
# cmake/mcuboot.cmake only signs the Zephyr-built artifact, which in a
# Non-Secure TF-M build is the NS app (image 1). The SPE is a TF-M-built
# artifact that cmake/mcuboot.cmake never sees, so it must be signed here to
# receive a valid MCUboot image-0 header. The SPE is signed against the
# Secure slot label (SLOT_LABEL, e.g. m33s_xip) directly, because this runs
# from the Non-Secure build context where slot0_partition is rebound to the
# Non-Secure slots.
#
# Signing parameters mirror cmake/mcuboot.cmake: version, header size
# (ROM_START_OFFSET), slot size, overwrite-only alignment and the optional
# signature key. The TF-M SPE binary does not reserve header space, so
# --pad-header is always passed.
#
# Usage: pse84_sign_tfm(<input_hex> <output_hex> <slot_nodelabel>)
function(pse84_sign_tfm INPUT_FILE OUTPUT_FILE SLOT_LABEL)
  dt_nodelabel(part_node NODELABEL "${SLOT_LABEL}" REQUIRED)
  dt_reg_addr(part_addr PATH ${part_node})
  dt_reg_size(slot_size PATH ${part_node})

  # Mirror cmake/mcuboot.cmake alignment handling. PSE84 uses overwrite-only
  # upgrades, so imgtool takes --overwrite-only --align 1.
  if(CONFIG_MCUBOOT_IMGTOOL_OVERWRITE_ONLY)
    set(align_args --overwrite-only --align 1)
  else()
    dt_chosen(flash_node PROPERTY "zephyr,flash")
    dt_prop(write_block_size PATH "${flash_node}" PROPERTY "write-block-size")
    if(NOT write_block_size)
      set(write_block_size 4)
    endif()
    set(align_args --align ${write_block_size})
  endif()

  # Resolve the MCUboot signature key the same way cmake/mcuboot.cmake does.
  # An unset key or CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE yields a hash-only
  # (unsigned) image, matching the Non-Secure image's signing.
  set(key_args)
  if(NOT CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE)
    set(keyfile "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
    string(CONFIGURE "${keyfile}" keyfile)
    if(NOT "${keyfile}" STREQUAL "")
      if(NOT IS_ABSOLUTE "${keyfile}")
        if(EXISTS "${APPLICATION_CONFIG_DIR}/${keyfile}")
          set(keyfile "${APPLICATION_CONFIG_DIR}/${keyfile}")
        elseif(WEST_TOPDIR)
          set(keyfile "${WEST_TOPDIR}/${keyfile}")
        endif()
      endif()
      set(key_args --key "${keyfile}")
    endif()
  endif()

  # With zephyr,mapped-partition, dt_reg_addr already returns the absolute
  # address. Partitions in the CBUS-S address space still need
  # BUILD_OUTPUT_ADJUST_LMA on top to rebase to the SAHB-S programming alias
  # used by the flasher (see pse84_add_extboot_metadata).
  if(CONFIG_BUILD_OUTPUT_ADJUST_LMA)
    math(EXPR hex_addr "${part_addr} + ${CONFIG_BUILD_OUTPUT_ADJUST_LMA}" OUTPUT_FORMAT HEXADECIMAL)
  else()
    set(hex_addr ${part_addr})
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${IMGTOOL} sign
      --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}
      --header-size ${CONFIG_ROM_START_OFFSET} --pad-header
      --slot-size ${slot_size} ${align_args} ${key_args}
      --hex-addr ${hex_addr}
      ${INPUT_FILE} ${OUTPUT_FILE}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${OUTPUT_FILE})
endfunction()
