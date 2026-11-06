# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tls_credentials_keygen)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_TLS_CREDENTIALS_SHELL_KEYGEN app PRIVATE src/shell.c)
target_sources_ifdef(CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_CUSTOM app PRIVATE src/test_store.c)
