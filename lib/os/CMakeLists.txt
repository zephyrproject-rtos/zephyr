# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/sys/clock.h
  ${ZEPHYR_BASE}/include/zephyr/sys/mutex.h
)

zephyr_sources(
  cbprintf_packaged.c
  clock.c
  printk.c
  sem.c
  thread_entry.c
  )

if(CONFIG_ZVFS_OPEN_IGNORE_MIN)
  set(final_fd_size ${CONFIG_ZVFS_OPEN_MAX})
else()
  # Import all custom ZVFS_OPEN_ size requirements
  import_kconfig(CONFIG_ZVFS_OPEN_ADD_SIZE_ ${DOTCONFIG} add_size_keys)

  # Calculate the sum of all "ADD_SIZE" requirements
  set(add_size_sum 0)
  foreach(add_size ${add_size_keys})
    math(EXPR add_size_sum "${add_size_sum} + ${${add_size}}")
  endforeach()

  if(CONFIG_ZVFS_OPEN_MAX LESS "${add_size_sum}")
    # Only warn if default value 0 has been modified
    if(NOT CONFIG_ZVFS_OPEN_MAX EQUAL 0)
      message(WARNING "
        CONFIG_ZVFS_OPEN_MAX is less than requested minimum:
          ${CONFIG_ZVFS_OPEN_MAX} < ${add_size_sum}
        Setting the file descriptor size to ${add_size_sum}")
    endif()

    set(final_fd_size ${add_size_sum})
  else()
    # CONFIG_ZVFS_OPEN_MAX was greater than the sum of the requirements
    set(final_fd_size ${CONFIG_ZVFS_OPEN_MAX})
  endif()
endif()

zephyr_compile_definitions(ZVFS_OPEN_SIZE=${final_fd_size})

zephyr_sources_ifdef(CONFIG_FDTABLE fdtable.c)
zephyr_syscall_header_ifdef(CONFIG_FDTABLE
  ${ZEPHYR_BASE}/include/zephyr/sys/fdtable.h
)

zephyr_sources_ifdef(CONFIG_CBPRINTF_COMPLETE cbprintf_complete.c)
zephyr_sources_ifdef(CONFIG_CBPRINTF_NANO cbprintf_nano.c)
add_subdirectory_ifdef(CONFIG_CPU_LOAD_METRIC cpu_load)

if(NOT CONFIG_PICOLIBC)
  zephyr_sources(cbprintf.c)
endif()

if(CONFIG_ASSERT OR CONFIG_ASSERT_VERBOSE)
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
