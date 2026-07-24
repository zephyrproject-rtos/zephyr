# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

# PSE84 image signing and merging.
# The reusable imgtool helpers live in pse84_metadata.cmake
# (pse84_add_extboot_metadata for the boot ROM ext-boot header,
# pse84_sign_tfm for an MCUboot image-0 header); the MCUboot + TF-M
# SIGNING_SCRIPT wrapper lives in pse84_tfm_signing.cmake (it must be a
# separate file because Zephyr invokes it via the SIGNING_SCRIPT property).
# Which of the hexes below `west flash` programs is selected at the end of
# this file (see "Runner hex selection").
#
# Platform invariant
# ------------------
# The boot ROM's extended boot always verifies an MCUboot-format header
# (0x400, CONFIG_ROM_START_OFFSET) on the FIRST image it runs, regardless
# of whether a further bootloader follows. So exactly one image per boot
# chain carries the ext-boot header:
#   - no MCUboot in the chain -> the secure app / TF-M SPE carries it;
#   - MCUboot present         -> the MCUboot bootloader carries it, and the
#                                images it loads get their own MCUboot
#                                headers instead: the NS app from
#                                cmake/mcuboot.cmake and the TF-M SPE from
#                                pse84_sign_tfm.
#
# Build scenarios and the hex that gets flashed
# ---------------------------------------------
#   Secure M33 app, no MCUboot      ext-boot sign vs m33s_xip   .signed.hex
#   MCUboot bootloader (CONFIG_     ext-boot sign vs            .signed.hex
#     MCUBOOT)                        boot_partition
#   App chain-loaded by MCUboot     signed by mcuboot.cmake     .signed.hex
#     (secure/NS/M55)
#   TF-M SPE+NSPE, no MCUboot       ext-boot sign SPE, merge    tfm_merged.hex
#                                     SPE + raw NS
#   TF-M SPE+NSPE + MCUboot         SPE signed as MCUboot       tfm_merged.hex
#                                     image 0; NS signed by
#                                     mcuboot.cmake; merge in
#                                     pse84_tfm_signing.cmake
#
# The NS core always pairs with a TF-M SPE built on the secure core, so a
# Non-Secure build always flashes the merged SPE + NS image.

# --------------------------------------------------------------------------
# TF-M: sign the SPE (as an ext-boot or an MCUboot image, depending on
# whether MCUboot is in the chain) and merge it with the NS image.
# --------------------------------------------------------------------------
if(CONFIG_BUILD_WITH_TFM AND NOT CONFIG_TFM_BL2)
  # PSE84 post-processes the TF-M images itself rather than using the
  # TF-M module's default (unsigned) secure + non-secure merge:
  #  - The Secure image (SPE, image 0) is signed so the first stage of the
  #    boot chain can validate it: the ext-boot header when the SPE runs
  #    directly from the boot ROM, or an MCUboot image-0 header when MCUboot
  #    chain-loads it.
  #  - The image programmed to flash is the merged signed SPE plus the
  #    Non-Secure image.
  # Opt out of the default merge; this platform produces the merged hex.
  set_property(GLOBAL PROPERTY TFM_PLATFORM_HANDLES_MERGE TRUE)

  set(pse84_tfm_s_signed_hex ${CMAKE_BINARY_DIR}/zephyr/tfm_s_signed.hex)
  set(pse84_tfm_merged_hex ${CMAKE_BINARY_DIR}/zephyr/tfm_merged.hex)

  if(CONFIG_TFM_USE_NS_APP)
    set(pse84_ns_hex $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>)
  else()
    set(pse84_ns_hex ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME})
  endif()

  if(CONFIG_BOOTLOADER_MCUBOOT)
    # Chain-loaded by MCUboot: the boot ROM's ext-boot header belongs on the
    # MCUboot bootloader (a separate CONFIG_MCUBOOT build), not on the SPE.
    # Here MCUboot validates both images it loads, so the SPE (image 0) gets
    # a proper MCUboot image-0 header signed with the same key and parameters
    # as the Non-Secure image, which cmake/mcuboot.cmake signs.
    pse84_sign_tfm($<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
                            ${pse84_tfm_s_signed_hex} m33s_xip)

    # The merged hex must contain the *signed* NSPE so MCUboot finds a valid
    # header at its slot. Defer the merge to the signing script, which runs
    # after NS signing has produced zephyr.signed.hex, and point the runner
    # at the merged image.
    set_target_properties(zephyr_property_target PROPERTIES
      SIGNING_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pse84_tfm_signing.cmake)
    set_property(GLOBAL PROPERTY pse84_tfm_s_merge_hex_file
                                 ${pse84_tfm_s_signed_hex})
    set_property(GLOBAL PROPERTY pse84_merged_hex_file
                                 ${pse84_tfm_merged_hex})
  else()
    # No MCUboot: the SPE is the first image the boot ROM runs, so it carries
    # the extended-boot metadata header. The TF-M-built secure binary does
    # not reserve header space, so pad it. Merge the signed SPE with the raw
    # Non-Secure image.
    pse84_add_extboot_metadata($<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
                               ${pse84_tfm_s_signed_hex} PAD_HEADER)

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
        -o ${pse84_tfm_merged_hex}
        ${pse84_tfm_s_signed_hex}
        ${pse84_ns_hex}
    )
    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
      ${pse84_tfm_merged_hex}
    )
  endif()
endif()

# --------------------------------------------------------------------------
# Standalone secure image: prepend the ext-boot header with imgtool.
# --------------------------------------------------------------------------
# The PSE84 boot ROM always validates an ext-boot header on the first image
# it runs, so every secure PSE84 image reaching here needs it. Two cases:
#  - CONFIG_MCUBOOT: the MCUboot bootloader image itself, signed against
#    boot_partition.
#  - !CONFIG_BOOTLOADER_MCUBOOT: a standalone secure image (no MCUboot
#    chain-load), signed against m33s_xip.
# Apps chain-loaded by MCUboot are signed by cmake/mcuboot.cmake and must
# not be double-signed here.
if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE
   AND (CONFIG_MCUBOOT OR NOT CONFIG_BOOTLOADER_MCUBOOT))
  set(unsigned_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex)
  set(signed_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.hex)
  pse84_add_extboot_metadata(${unsigned_hex} ${signed_hex})
endif()

# --------------------------------------------------------------------------
# Runner hex selection: tell `west flash` which of the hexes produced above
# to program. Kept next to the flow that produces the files so the selection
# and the production logic stay in sync.
#   secure M33 or MCUboot-loaded image      -> <kernel>.signed.hex
#   Non-Secure build (always TF-M SPE + NS) -> tfm_merged.hex
# In the TF-M + MCUboot case the stock cmake/mcuboot.cmake later resets
# hex_file to zephyr.signed.hex, so pse84_tfm_signing.cmake re-asserts the
# merged hex after that flow runs.
# --------------------------------------------------------------------------
if((CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
    OR CONFIG_BOOTLOADER_MCUBOOT)
  set_property(TARGET runners_yaml_props_target
    PROPERTY hex_file ${KERNEL_NAME}.signed.hex)
endif()

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_NONSECURE)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()
