# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BASE64 base64.c)

zephyr_sources(
  dec.c
  hex.c
  rb.c
  timeutil.c
  bitarray.c
  bitmask.c
  )

zephyr_sources_ifdef(CONFIG_ONOFF onoff.c)

zephyr_sources_ifdef(CONFIG_NOTIFY notify.c)

zephyr_sources_ifdef(CONFIG_JSON_LIBRARY json.c)

zephyr_sources_ifdef(CONFIG_RING_BUFFER ring_buffer.c)

zephyr_sources_ifdef(CONFIG_UTF8 utf8.c)

zephyr_sources_ifdef(CONFIG_WINSTREAM winstream.c)

zephyr_sources_ifdef(CONFIG_COBS cobs.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

zephyr_sources_ifdef(CONFIG_LINKER_LAST_SECTION_ID last_section_id.c)
