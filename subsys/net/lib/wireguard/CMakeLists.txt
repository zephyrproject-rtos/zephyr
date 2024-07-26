# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/wireguard/)

zephyr_library_sources(wg.c)

zephyr_library_sources(crypto/crypto.c)
zephyr_library_sources(crypto/refc/chacha20.c)
zephyr_library_sources(crypto/refc/poly1305-donna.c)
zephyr_library_sources(crypto/refc/blake2s.c)
zephyr_library_sources(crypto/refc/chacha20poly1305.c)
zephyr_library_sources(crypto/refc/x25519.c)
