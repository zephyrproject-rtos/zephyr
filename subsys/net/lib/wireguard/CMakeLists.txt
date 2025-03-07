# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(wireguard)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(../../../../include)

zephyr_library_sources(wg.c)
zephyr_library_sources(wg_psa.c)

zephyr_library_sources(
  crypto/crypto.c
)

# Reference implementations needed when not using PSA or for algorithms
# not available in PSA (Blake and XChaCha20-Poly1305)
zephyr_library_sources(
  crypto/refc/hchacha20.c
  crypto/refc/blake2s.c
)

zephyr_library_sources_ifdef(CONFIG_BOARD_NATIVE_SIM ns_rtc.c)
