# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

# PSE84 TF-M + MCUboot signing script.
#
# On PSE84 the Non-Secure image lives in a slot whose size differs from
# the Secure slot. The board devicetree already rebinds slot0_partition /
# slot1_partition to the Non-Secure slots for the Non-Secure build (see
# kit_pse84_eval_m33_ns.dts), so the stock cmake/mcuboot.cmake flow signs
# the NSPE against the correct slot size on its own. This wrapper only
# adds the PSE84-specific post-signing steps on top of that flow:
#
#   1. Run the stock MCUboot signing (produces zephyr.signed.hex/.bin and
#      points the runner at zephyr.signed.hex).
#   2. Merge the signed SPE image with the freshly signed NSPE image into
#      tfm_merged.hex. The SPE hex and the merged-output path are handed
#      over from the SoC CMakeLists via global properties.
#   3. Override the runner hex_file to tfm_merged.hex so `west flash`
#      programs the combined signed SPE + signed NSPE image.

# Step 1: stock MCUboot signing (slot geometry comes from slot0/slot1
# partitions, which the Non-Secure devicetree points at the NS slots).
include(${ZEPHYR_BASE}/cmake/mcuboot.cmake)

# Step 2: merge the signed SPE with the signed NSPE. The SoC CMakeLists
# deferred this merge to here so that zephyr.signed.hex (produced above)
# is used instead of the raw NS image. The SPE was already signed as
# MCUboot image 0 by pse84_sign_tfm; this lets MCUboot validate
# image 1 (NSPE) at slot2 in addition to image 0 (SPE) at slot0.
get_property(pse84_tfm_s_merge_hex_file GLOBAL
             PROPERTY pse84_tfm_s_merge_hex_file)
get_property(pse84_merged_hex_file GLOBAL
             PROPERTY pse84_merged_hex_file)

if(pse84_merged_hex_file)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${pse84_merged_hex_file}
      ${pse84_tfm_s_merge_hex_file}
      ${CMAKE_BINARY_DIR}/zephyr/zephyr.signed.hex
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
    ${pse84_merged_hex_file}
  )

  # Step 3: flash the merged image instead of the bare signed NSPE.
  set_target_properties(runners_yaml_props_target PROPERTIES
    hex_file ${pse84_merged_hex_file})
endif()
