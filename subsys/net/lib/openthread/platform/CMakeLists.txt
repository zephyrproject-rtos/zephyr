# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(openthread_platform)
zephyr_library_sources(
  alarm.c
  entropy.c
  flash.c
  logging.c
  misc.c
  platform.c
  radio.c
  spi.c
  )

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_SHELL shell.c)

# The source files here use header files from the OpenThread project
# target_link_libraries(app ot_lib) # Better?
add_dependencies(openthread_platform ot)
