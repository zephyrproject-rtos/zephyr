# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  fido2_core.c
  fido2_cbor.c
  fido2_crypto.c
  fido2_clientpin.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS_BUILTIN tfpsacrypto)

add_subdirectory(attestation)
add_subdirectory(storage)
add_subdirectory(transport)
add_subdirectory(up)
