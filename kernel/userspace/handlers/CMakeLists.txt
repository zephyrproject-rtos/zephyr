# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# System call verifiers moved out of the kernel sources. Each file is
# compiled under the same condition as the source it belongs to.
#
# This is a Zephyr library, linked with --whole-archive, on purpose: the
# system call table in userspace.c defines a weak fallback for every
# handler, and an archive member of libkernel.a that only defines handlers
# would never be extracted to override them.
zephyr_library_named(kernel_syscall_handlers)
zephyr_library_include_directories(${ZEPHYR_BASE}/kernel/include ${ARCH_DIR}/${ARCH}/include)
zephyr_library_compile_definitions(__ZEPHYR_SUPERVISOR__)

zephyr_library_sources(busy_wait.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING condvar.c)
zephyr_library_sources_ifdef(CONFIG_SCHED_DEADLINE deadline.c)
zephyr_library_sources(device.c)
if(CONFIG_DYNAMIC_THREAD)
  zephyr_library_sources(dynamic.c)
else()
  zephyr_library_sources(dynamic_disabled.c)
endif()
zephyr_library_sources_ifdef(CONFIG_EVENTS events.c)
zephyr_library_sources(float.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING msg_q.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING mutex.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING pipe.c)
zephyr_library_sources_ifdef(CONFIG_POLL poll.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING queue.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING scheduler.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING sem.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING sleep.c)
zephyr_library_sources_ifdef(CONFIG_MULTITHREADING stack.c)
