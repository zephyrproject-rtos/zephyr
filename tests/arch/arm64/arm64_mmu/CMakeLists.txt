# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm64_mmu)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  ${ZEPHYR_BASE}
  )

# The table accounting suite asserts exact table counts, which only hold
# without memory domains; the domain suite needs them. One per build.
target_sources_ifndef(CONFIG_USERSPACE app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_USERSPACE app PRIVATE src/domain_attrs.c)
