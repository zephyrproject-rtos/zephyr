# SPDX-License-Identifier: Apache-2.0

set(POSIX_VERSION 200809L)
set(GEN_DIR ${ZEPHYR_BINARY_DIR}/include/generated)

zephyr_compile_definitions(-D_POSIX_C_SOURCE=${POSIX_VERSION})

zephyr_syscall_header_ifdef(CONFIG_POSIX_CLOCK_SELECTION posix_clock.h)
zephyr_syscall_header_ifdef(CONFIG_POSIX_TIMERS posix_clock.h)
zephyr_syscall_header_ifdef(CONFIG_XSI_SINGLE_PROCESS posix_clock.h)

if(CONFIG_POSIX_SYSTEM_HEADERS)
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_SIGNALS)
  if(CONFIG_POSIX_SIGNALS)
    set(STRSIGNAL_TABLE_H ${GEN_DIR}/posix/strsignal_table.h)

    add_custom_command(
      OUTPUT ${STRSIGNAL_TABLE_H}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/gen_strsignal_table.py
      -i ${ZEPHYR_BASE}/include/zephyr/posix/posix_signal.h
      -o ${STRSIGNAL_TABLE_H}
      DEPENDS ${ZEPHYR_BASE}/include/zephyr/posix/posix_signal.h
    )
  endif()
endif()

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_EVENTFD eventfd.c)

if (NOT CONFIG_TC_PROVIDES_POSIX_ASYNCHRONOUS_IO)
  zephyr_library_sources_ifdef(CONFIG_POSIX_ASYNCHRONOUS_IO aio.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_BARRIERS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_BARRIERS barrier.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_CLOCK_SELECTION)
  zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK_SELECTION clock_selection.c)
endif()
if (CONFIG_POSIX_CLOCK_SELECTION)
  zephyr_compile_definitions(-D_POSIX_CLOCK_SELECTION=${POSIX_VERSION})
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_C_LIB_EXT)
  zephyr_library_sources_ifdef(CONFIG_POSIX_C_LIB_EXT
    fnmatch.c
    getentropy.c
    getopt/getopt.c
    getopt/getopt_common.c
  )
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_DEVICE_IO)
  zephyr_library_sources_ifdef(CONFIG_POSIX_DEVICE_IO
    # perror should be moved to the common libc
    perror.c
    device_io.c
  )
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_FD_MGMT)
  zephyr_library_sources_ifdef(CONFIG_POSIX_FD_MGMT
    fd_mgmt.c
  )
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_FILE_SYSTEM)
  zephyr_library_sources_ifdef(CONFIG_POSIX_FILE_SYSTEM fs.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_FILE_SYSTEM_R)
  zephyr_library_sources_ifdef(CONFIG_POSIX_FILE_SYSTEM_R file_system_r.c)
endif()
if (CONFIG_POSIX_FILE_SYSTEM_R)
  zephyr_compile_definitions(-D_POSIX_THREAD_SAFE_FUNCTIONS=${POSIX_VERSION})
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_MEMORY_PROTECTION)
zephyr_library_sources_ifdef(CONFIG_POSIX_MEMORY_PROTECTION mprotect.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_MAPPED_FILES)
  zephyr_library_sources_ifdef(CONFIG_POSIX_MAPPED_FILES mmap.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_MULTI_PROCESS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_MULTI_PROCESS
    sleep.c
    multi_process.c
  )
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_NETWORKING)
  zephyr_library_sources_ifdef(CONFIG_POSIX_NETWORKING net.c)
endif()

if (CONFIG_POSIX_REALTIME_SIGNALS)
  zephyr_compile_definitions(-D_POSIX_REALTIME_SIGNALS=${POSIX_VERSION})
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_SIGNALS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_SIGNALS signal.c ${STRSIGNAL_TABLE_H})
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_SINGLE_PROCESS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_SINGLE_PROCESS
    confstr.c
    env.c
    env_common.c
    sysconf.c
    uname.c
  )
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_SPIN_LOCKS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_SPIN_LOCKS spinlock.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_TIMERS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_TIMERS
    clock.c
    timer.c
    timespec_to_timeout.c
  )
endif()
if (CONFIG_POSIX_TIMERS)
  zephyr_compile_definitions(-D_POSIX_TIMERS=${POSIX_VERSION})
  # FIXME: Until we have a Kconfig for XSI_ADVANCED_REALTIME, define _POSIX_CPUTIME and
  # _POSIX_MONOTONIC_CLOCK with _POSIX_TIMERS.
  # For more information on the Advanced Realtime Option Group, please see
  # https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap02.html
  zephyr_compile_definitions(-D_POSIX_CPUTIME=${POSIX_VERSION})
  zephyr_compile_definitions(-D_POSIX_MONOTONIC_CLOCK=${POSIX_VERSION})
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_RW_LOCKS)
  zephyr_library_sources_ifdef(CONFIG_POSIX_RW_LOCKS rwlock.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_SEMAPHORES)
  zephyr_library_sources_ifdef(CONFIG_POSIX_SEMAPHORES semaphore.c)
endif()

if (NOT CONFIG_TC_PROVIDES_POSIX_THREADS)
  # Note: the Option is _POSIX_THREADS, while the Option Group is POSIX_THREADS_BASE.
  # We have opted to use POSIX_THREADS here to match the Option name.
  zephyr_library_sources_ifdef(CONFIG_POSIX_THREADS
    cond.c
    grp.c
    key.c
    mutex.c
    pthread.c
    pwd.c
  )
endif()
if (CONFIG_POSIX_THREADS)
  zephyr_compile_definitions(-D_POSIX_THREADS=${POSIX_VERSION})
endif()

if(NOT CONFIG_TC_PROVIDES_XSI_REALTIME)
  zephyr_library_sources_ifdef(CONFIG_POSIX_FSYNC fsync.c)
  zephyr_library_sources_ifdef(CONFIG_POSIX_MEMLOCK mlockall.c)
  zephyr_library_sources_ifdef(CONFIG_POSIX_MEMLOCK_RANGE mlock.c)
  zephyr_library_sources_ifdef(CONFIG_POSIX_MESSAGE_PASSING mqueue.c)
  zephyr_library_sources_ifdef(CONFIG_POSIX_PRIORITY_SCHEDULING sched.c)
  zephyr_library_sources_ifdef(CONFIG_POSIX_SHARED_MEMORY_OBJECTS shm.c)
endif()

zephyr_library_sources_ifdef(CONFIG_XOPEN_STREAMS stropts.c)

if (NOT CONFIG_TC_PROVIDES_XSI_SINGLE_PROCESS)
  zephyr_library_sources_ifdef(CONFIG_XSI_SINGLE_PROCESS
    env_common.c
    xsi_single_process.c
  )
endif()

if (NOT CONFIG_TC_PROVIDES_XSI_SYSTEM_LOGGING)
  zephyr_library_sources_ifdef(CONFIG_XSI_SYSTEM_LOGGING syslog.c)
endif()

zephyr_library_sources_ifdef(CONFIG_GETOPT_LONG
  getopt/getopt_long.c
)
zephyr_include_directories_ifdef(CONFIG_POSIX_C_LIB_EXT
  getopt/
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_compile_options(-U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=200809L)
