# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/sys/mutex.h
)

zephyr_sources(
  cbprintf_packaged.c
  printk.c
  sem.c
  thread_entry.c
  )

zephyr_sources_ifdef(CONFIG_FDTABLE fdtable.c)
zephyr_syscall_header_ifdef(CONFIG_FDTABLE
  ${ZEPHYR_BASE}/include/zephyr/sys/fdtable.h
)

zephyr_sources_ifdef(CONFIG_CBPRINTF_COMPLETE cbprintf_complete.c)
zephyr_sources_ifdef(CONFIG_CBPRINTF_NANO cbprintf_nano.c)

if(NOT CONFIG_PICOLIBC)
  zephyr_sources(cbprintf.c)
endif()

if (CONFIG_ASSERT OR CONFIG_ASSERT_VERBOSE)
zephyr_sources(assert.c)
endif()

zephyr_sources_ifdef(CONFIG_USERSPACE mutex.c user_work.c)

zephyr_sources_ifdef(CONFIG_MPSC_PBUF mpsc_pbuf.c)

zephyr_sources_ifdef(CONFIG_SPSC_PBUF spsc_pbuf.c)

zephyr_sources_ifdef(CONFIG_SCHED_DEADLINE p4wq.c)

zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)

zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

add_subdirectory_ifdef(CONFIG_ZVFS zvfs)
