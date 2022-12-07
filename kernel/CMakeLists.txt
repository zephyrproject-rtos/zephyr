# SPDX-License-Identifier: Apache-2.0

# kernel is a normal CMake library and not a zephyr_library because it
# should not be --whole-archive'd

# If a pre-built static library containing kernel code exists in
# this directory, libkernel.a, link it with the application code
# instead of building from source.
zephyr_library_get_current_dir_lib_name(${ZEPHYR_BASE} libkernel_stem)
set(libkernel ${CMAKE_CURRENT_SOURCE_DIR}/lib${libkernel_stem}${CMAKE_STATIC_LIBRARY_SUFFIX})
unset(libkernel_stem)

if(EXISTS ${libkernel})

add_library(kernel INTERFACE)
target_link_libraries(kernel INTERFACE ${libkernel})

else()

list(APPEND kernel_files
  main_weak.c
  banner.c
  device.c
  errno.c
  fatal.c
  init.c
  kheap.c
  mem_slab.c
  thread.c
  version.c
  )

if(CONFIG_MULTITHREADING)
list(APPEND kernel_files
  idle.c
  mailbox.c
  msg_q.c
  mutex.c
  queue.c
  sem.c
  stack.c
  system_work_q.c
  work.c
  sched.c
  condvar.c
  )

if(CONFIG_SMP)
list(APPEND kernel_files
     smp.c)
endif()

endif()

if(CONFIG_XIP)
list(APPEND kernel_files
     xip.c)
endif()

if(CONFIG_DEMAND_PAGING_STATS)
list(APPEND kernel_files
     paging/statistics.c)
endif()

add_library(kernel ${kernel_files})

# Kernel files has the macro __ZEPHYR_SUPERVISOR__ set so that it
# optimizes the code when userspace is enabled.

set_target_properties(
  kernel
  PROPERTIES
  COMPILE_DEFINITIONS
  __ZEPHYR_SUPERVISOR__
  )

target_sources_ifdef(CONFIG_STACK_CANARIES        kernel PRIVATE compiler_stack_protect.c)
target_sources_ifdef(CONFIG_SYS_CLOCK_EXISTS      kernel PRIVATE timeout.c timer.c)
target_sources_ifdef(CONFIG_ATOMIC_OPERATIONS_C   kernel PRIVATE atomic_c.c)
target_sources_ifdef(CONFIG_MMU                   kernel PRIVATE mmu.c)
target_sources_ifdef(CONFIG_POLL                  kernel PRIVATE poll.c)
target_sources_ifdef(CONFIG_EVENTS                kernel PRIVATE events.c)
target_sources_ifdef(CONFIG_PIPES                 kernel PRIVATE pipes.c)
target_sources_ifdef(CONFIG_SCHED_THREAD_USAGE    kernel PRIVATE usage.c)

if(${CONFIG_KERNEL_MEM_POOL})
  target_sources(kernel PRIVATE mempool.c)
endif()


# The last 2 files inside the target_sources_ifdef should be
# userspace_handler.c and userspace.c. If not the linker would complain.
# This order has to be maintained. Any new file should be placed
# above these 2 files.
target_sources_ifdef(
  CONFIG_USERSPACE
  kernel PRIVATE
  futex.c
  mem_domain.c
  userspace_handler.c
  userspace.c
  )

target_include_directories(kernel PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

target_link_libraries(kernel zephyr_interface)

endif()

add_dependencies(kernel zephyr_generated_headers)

unset(libkernel)
