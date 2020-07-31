# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BASE64 base64.c)

zephyr_sources(
  crc32_sw.c
  crc16_sw.c
  crc8_sw.c
  crc7_sw.c
  dec.c
  fdtable.c
  hex.c
  mempool.c
  notify.c
  printk.c
  onoff.c
  rb.c
  sem.c
  thread_entry.c
  timeutil.c
  work_q.c
  heap.c
  heap-validate.c
  )

zephyr_sources_ifdef(CONFIG_JSON_LIBRARY json.c)

zephyr_sources_ifdef(CONFIG_RING_BUFFER ring_buffer.c)

zephyr_sources_ifdef(CONFIG_ASSERT assert.c)

zephyr_sources_ifdef(CONFIG_USERSPACE mutex.c)
