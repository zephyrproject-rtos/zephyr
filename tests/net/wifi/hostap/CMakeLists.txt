# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hostap_cmd_wq)

# Build only the offload mechanism under test, plus the test harness. The
# get_cmd_wq() hook (normally in supp_main.c) is stubbed in main.c, so the
# whole wpa_supplicant is not pulled in.
target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/modules/hostap/src/supp_cmd_wq.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/modules/hostap/src
)
