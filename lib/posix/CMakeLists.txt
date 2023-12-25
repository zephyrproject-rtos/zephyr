# SPDX-License-Identifier: Apache-2.0

# Requirement of IEEE 1003.1-2017 (and earllier versions)
# Volume 1, Base Definitions, Issue 7
# Chapter 2 Conformance
# Section 2.2: Application Conformance
#
# For the C Programming Language, [a strictly conforming posix application] shall define
# _POSIX_C_SOURCE to be 200809L before any header is included.
#
# Rather than requiring every Zephyr app to be strictly conformant and specify
# _POSIX_C_SOURCE=<version>, and since any source file is free to specify a different (and
# possibly conflicting) value for _POSIX_C_SOURCE, require Zephyr apps to be non-strictly
# conformant when they select POSIX Kconfig options.
#
# To that end, leverage separate, indirect, (and unfortunately non-standard) conventional
# application conformance options specific to individual C libraries.
#
# This is explicitly used to allow Zephyr's Kconfig environment to signal to the C library that
# POSIX declarations should me made visible to the application at the default conformance level.
if(CONFIG_POSIX_ZEPHYR_SOURCE)
  if(CONFIG_MINIMAL_LIBC)
    zephyr_compile_definitions(_ZEPHYR_SOURCE)
    zephyr_compile_options(-include "${ZEPHYR_BASE}/lib/posix/zephyr_source.h")
  elseif(CONFIG_NEWLIB_LIBC)
    # Newlib does not recognize _ZEPHYR_SOURCE (https://bit.ly/41K5DHe)
    zephyr_compile_definitions(_DEFAULT_SOURCE)
  elseif(CONFIG_PICOLIBC)
    # Picolibc recognizes _ZEPHYR_SOURCE, but unfortunately hard-codes it (along with
    # _POSIX_C_SOURCE=200809 in multiple locations) for all Zephyr builds, even those that do not
    # use POSIX APIs. Additionally, it does not account for _ZEPHYR_SOURCE when defining
    # _DEFAULT_SOURCE.
    # https://bit.ly/48cR80Y
    # https://bit.ly/3GWSAs7
    # https://bit.ly/41zmwnX
    # https://bit.ly/3vlNi6Y
    zephyr_compile_definitions(_ZEPHYR_SOURCE)
    # Remove the definition below once the issues above are fixed.
    zephyr_compile_definitions(_DEFAULT_SOURCE)
  elseif(NOT CONFIG_NATIVE_LIBC)
    # external libc - cannot make assumptions, but need _POSIX_C_SOURCE visibility
    zephyr_compile_definitions(_POSIX_C_SOURCE=200809L)
  endif()
endif()

set(GEN_DIR ${ZEPHYR_BINARY_DIR}/include/generated)

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/posix/time.h
)

zephyr_interface_library_named(posix_subsys)

if(CONFIG_POSIX_API)
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)
endif()

if(CONFIG_POSIX_SIGNAL)
  set(STRSIGNAL_TABLE_H ${GEN_DIR}/posix/strsignal_table.h)

  add_custom_command(
    OUTPUT ${STRSIGNAL_TABLE_H}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_strsignal_table.py
    -i ${ZEPHYR_BASE}/include/zephyr/posix/signal.h
    -o ${STRSIGNAL_TABLE_H}
    DEPENDS ${ZEPHYR_BASE}/include/zephyr/posix/signal.h
  )
endif()

zephyr_library()
add_subdirectory_ifdef(CONFIG_GETOPT getopt)
zephyr_library_sources_ifdef(CONFIG_EVENTFD eventfd.c)
zephyr_library_sources_ifdef(CONFIG_FNMATCH fnmatch.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_API perror.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK clock.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK nanosleep.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK sleep.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_CLOCK timer.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_FS fs.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_MQUEUE mqueue.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_SIGNAL signal.c ${STRSIGNAL_TABLE_H})
zephyr_library_sources_ifdef(CONFIG_POSIX_UNAME uname.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC _common.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_BARRIER barrier.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_COND cond.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_KEY key.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_MUTEX mutex.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD pthread.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC rwlock.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC sched.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_IPC semaphore.c)
zephyr_library_sources_ifdef(CONFIG_PTHREAD_SPINLOCK spinlock.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_link_libraries(posix_subsys)
zephyr_library_property(ALLOW_EMPTY TRUE)
