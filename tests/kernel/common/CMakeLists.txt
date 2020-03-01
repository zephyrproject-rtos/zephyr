# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(kernel_common)

if(NOT CONFIG_ARM)
target_sources(app PRIVATE
	src/bitfield.c
  )
endif()
if(CONFIG_PRINTK)
target_sources(app PRIVATE
	src/printk.c
  )
endif()
target_sources(app PRIVATE
	src/atomic.c
	src/byteorder.c
	src/clock.c
	src/main.c
	src/timeout_order.c
	src/multilib.c
  src/errno.c
  src/boot_delay.c
  src/irq_offload.c
  )
