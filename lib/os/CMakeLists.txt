# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BASE64 base64.c)

zephyr_sources(
  cbprintf.c
  cbprintf_packaged.c
  crc32c_sw.c
  crc32_sw.c
  crc16_sw.c
  crc8_sw.c
  crc7_sw.c
  dec.c
  fdtable.c
  hex.c
  notify.c
  printk.c
  onoff.c
  rb.c
  sem.c
  thread_entry.c
  timeutil.c
  heap.c
  heap-validate.c
  )

zephyr_sources_ifdef(CONFIG_CBPRINTF_COMPLETE cbprintf_complete.c)
zephyr_sources_ifdef(CONFIG_CBPRINTF_NANO cbprintf_nano.c)

zephyr_sources_ifdef(CONFIG_JSON_LIBRARY json.c)

zephyr_sources_ifdef(CONFIG_RING_BUFFER ring_buffer.c)

zephyr_sources_ifdef(CONFIG_ASSERT assert.c)

zephyr_sources_ifdef(CONFIG_USERSPACE mutex.c user_work.c)

zephyr_sources_ifdef(CONFIG_SCHED_DEADLINE p4wq.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)
