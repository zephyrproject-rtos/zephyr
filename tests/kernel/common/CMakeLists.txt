cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

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
	src/dlist.c
	src/intmath.c
	src/main.c
	src/slist.c
	src/timeout_order.c
	src/multilib.c
  )
