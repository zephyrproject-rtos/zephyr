# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  fido2_core.c
  fido2_cbor.c
)

add_subdirectory(transport)
