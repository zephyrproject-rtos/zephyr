include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

if(NOT CONFIG_ARM)
target_sources(app PRIVATE
	src/bitfield.c
  )
endif()
target_sources(app PRIVATE
	src/atomic.c
	src/byteorder.c
	src/clock.c
	src/dlist.c
	src/intmath.c
	src/main.c
	src/printk.c
	src/rand32.c
	src/slist.c
	src/timeout_order.c
  )
