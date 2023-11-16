# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
  tls_credentials.c
  tls_credentials_digest_raw.c
)
zephyr_library_sources_ifdef(CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE
  tls_credentials_trusted.c
  tls_credentials_digest_raw.c
)
zephyr_library_sources_ifdef(CONFIG_TLS_CREDENTIALS_SHELL
  tls_credentials_shell.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
