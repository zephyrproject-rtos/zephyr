# SPDX-License-Identifier: Apache-2.0

# kernel is a normal CMake library and not a zephyr_library because it
# should not be --whole-archive'd

list(APPEND kernel_files
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
  pipes.c
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

if(CONFIG_CACHE_MANAGEMENT AND CONFIG_USERSPACE)
  target_sources(kernel PRIVATE cache_handlers.c)
endif()

target_include_directories(kernel PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

add_dependencies(kernel zephyr_generated_headers)

target_link_libraries(kernel zephyr_interface)
