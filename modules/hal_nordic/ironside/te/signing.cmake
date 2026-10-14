# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Signing for an application booted by IronSide TE.
#
# The UROT verifies an MCUboot-format image signed with ECDSA P-384, so the application
# leaves a CONFIG_ROM_START_OFFSET header gap at the base of its slot and is signed by
# ironsdk/scripts/sign.py rather than by imgtool.

function(zephyr_runner_file type path)
  # Makes west flash pick the signed output.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

function(ironside_te_signing_tasks)
  if(CONFIG_IRONSIDE_TE_TEST_KEYSET)
    message(WARNING "
      ----------------------------------------------------------
      --- WARNING: Signing with the IronSide TE test keyset. ---
      --- Its private keys are public, so anyone can sign an ---
      --- image this device will boot. Do not use it in      ---
      --- production: set CONFIG_IRONSIDE_TE_KEYSET.         ---
      ----------------------------------------------------------"
    )
  endif()

  dt_chosen(code_partition PROPERTY "zephyr,code-partition")
  dt_reg_size(slot_size PATH "${code_partition}")

  set(sign_py
    "${PYTHON_EXECUTABLE}"
    "${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/scripts/sign.py"
  )

  # --no-pad-header: the image already starts at the slot base with the
  # CONFIG_ROM_START_OFFSET gap filled in, so sign.py writes the header into
  # that gap rather than prepending one, which would place the image a header
  # below its slot, inside the UROT.
  set(sign_args
    --role app
    --no-pad-header
    --slot-size "${slot_size}"
    --soc "${CONFIG_SOC}"
    --keyset "${CONFIG_IRONSIDE_TE_KEYSET}"
    --version "${CONFIG_IRONSIDE_TE_SIGN_VERSION}"
  )

  if(CONFIG_BUILD_OUTPUT_HEX)
    set(hex_input ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex)
    set(hex_output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.hex)

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${sign_py} "${hex_input}" --out "${hex_output}" ${sign_args}
    )
    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${hex_output})

    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${hex_output}"
      CACHE FILEPATH "Signed kernel hex file" FORCE
    )
    zephyr_runner_file(hex ${hex_output})
  endif()

  if(CONFIG_BUILD_OUTPUT_BIN)
    set(bin_input ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.bin)
    set(bin_output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.bin)

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${sign_py} "${bin_input}" --out "${bin_output}" ${sign_args}
    )
    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${bin_output})

    set(BYPRODUCT_KERNEL_SIGNED_BIN_NAME "${bin_output}"
      CACHE FILEPATH "Signed kernel bin file" FORCE
    )
    zephyr_runner_file(bin ${bin_output})
  endif()
endfunction()

ironside_te_signing_tasks()
