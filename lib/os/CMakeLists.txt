# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BASE64 base64.c)

zephyr_sources(
  cbprintf_packaged.c
  dec.c
  fdtable.c
  hex.c
  printk.c
  rb.c
  sem.c
  thread_entry.c
  timeutil.c
  heap.c
  heap-validate.c
  bitarray.c
  multi_heap.c
  )

zephyr_sources_ifdef(CONFIG_ONOFF onoff.c)
zephyr_sources_ifdef(CONFIG_NOTIFY notify.c)
zephyr_sources_ifdef(CONFIG_CRC
  crc32c_sw.c
  crc32_sw.c
  crc16_sw.c
  crc8_sw.c
  crc7_sw.c
  )
zephyr_sources_ifdef(CONFIG_CRC_SHELL crc_shell.c)

zephyr_sources_ifdef(CONFIG_CBPRINTF_COMPLETE cbprintf_complete.c)
zephyr_sources_ifdef(CONFIG_CBPRINTF_NANO cbprintf_nano.c)

if(NOT CONFIG_PICOLIBC)
  zephyr_sources(cbprintf.c)
endif()

zephyr_sources_ifdef(CONFIG_JSON_LIBRARY json.c)

zephyr_sources_ifdef(CONFIG_RING_BUFFER ring_buffer.c)

if (CONFIG_ASSERT OR CONFIG_ASSERT_VERBOSE)
zephyr_sources(assert.c)
endif()

zephyr_sources_ifdef(CONFIG_USERSPACE mutex.c user_work.c)

zephyr_sources_ifdef(CONFIG_MPSC_PBUF mpsc_pbuf.c)

zephyr_sources_ifdef(CONFIG_SPSC_PBUF spsc_pbuf.c)

zephyr_sources_ifdef(CONFIG_SCHED_DEADLINE p4wq.c)

zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)

zephyr_sources_ifdef(CONFIG_SHARED_MULTI_HEAP shared_multi_heap.c)

zephyr_sources_ifdef(CONFIG_HEAP_LISTENER heap_listener.c)

zephyr_sources_ifdef(CONFIG_UTF8 utf8.c)

zephyr_sources_ifdef(CONFIG_SYS_MEM_BLOCKS mem_blocks.c)

zephyr_sources_ifdef(CONFIG_WINSTREAM winstream.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)
