# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_UOSCORE OR CONFIG_UEDHOC)

  set(UOSCORE_UEDHOC_SRC_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/src)
  set(UOSCORE_UEDHOC_INCLUDE_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/inc)

  zephyr_include_directories(${UOSCORE_UEDHOC_INCLUDE_DIR})

# Common part used by both libraries

  zephyr_library_named(uoscore_uedhoc_common)

  zephyr_library_compile_definitions(MBEDTLS)

  if (CONFIG_UOSCORE_DEBUG OR CONFIG_UEDHOC_DEBUG)
    zephyr_library_compile_definitions(DEBUG_PRINT)
  endif()

  zephyr_library_sources(
    ${UOSCORE_UEDHOC_SRC_DIR}/common/byte_array.c
    ${UOSCORE_UEDHOC_SRC_DIR}/common/crypto_wrapper.c
    ${UOSCORE_UEDHOC_SRC_DIR}/common/memcpy_s.c
    ${UOSCORE_UEDHOC_SRC_DIR}/common/print_util.c
  )

  zephyr_library_link_libraries(mbedTLS)

# UOSCORE

  if (CONFIG_UOSCORE)

    zephyr_library_named(uoscore)

    zephyr_library_compile_definitions(MBEDTLS)

    if (CONFIG_UOSCORE_DEBUG)
      zephyr_library_compile_definitions(DEBUG_PRINT)
    endif()

    zephyr_library_sources(
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/aad.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/coap2oscore.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/nonce.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/option.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/oscore2coap.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/oscore_coap.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/oscore_cose.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/oscore_hkdf_info.c
      ${UOSCORE_UEDHOC_SRC_DIR}/oscore/security_context.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/oscore_aad_array.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/oscore_enc_structure.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/oscore_info.c
    )

    zephyr_library_link_libraries(mbedTLS)

  endif() # CONFIG_UOSCORE

# UEDHOC

  if (CONFIG_UEDHOC)

    zephyr_library_named(uedhoc)

    zephyr_library_compile_definitions(MBEDTLS)

    if (CONFIG_UEDHOC_DEBUG)
      zephyr_library_compile_definitions(DEBUG_PRINT)
    endif()

    zephyr_library_sources(
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/associated_data_encode.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/cert.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/ciphertext.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/c_x.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/edhoc_cose.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/edhoc_exporter.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/edhoc_method_type.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/hkdf_info.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/initiator.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/okm.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/plaintext_decode.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/plaintext_encode.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/prk.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/responder.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/retrieve_cred.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/runtime_context.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/signature_or_mac_msg.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/suites.c
      ${UOSCORE_UEDHOC_SRC_DIR}/edhoc/th.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_bstr_type.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_cert.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_id_cred_x.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_message_1.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_message_2.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_message_3.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_decode_plaintext.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_bstr_type.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_data_2.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_enc_structure.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_id_cred_x.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_info.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_int_type.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_message_1.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_message_2.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_message_3.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_message_error.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_sig_structure.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_th2.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_th3.c
      ${UOSCORE_UEDHOC_SRC_DIR}/cbor/edhoc_encode_th4.c
    )

    zephyr_library_link_libraries(mbedTLS)

  endif() # CONFIG_UEDHOC

endif() # CONFIG_UOSCORE OR CONFIG_UEDHOC
