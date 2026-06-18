# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  fido2_core.c
  fido2_cbor.c
  fido2_crypto.c
)

add_subdirectory(attestation)
add_subdirectory(storage)
add_subdirectory(transport)
add_subdirectory(up)
