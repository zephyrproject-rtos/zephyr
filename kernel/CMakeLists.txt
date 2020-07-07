# SPDX-License-Identifier: Apache-2.0

# kernel is a normal CMake library and not a zephyr_library because it
# should not be --whole-archive'd
add_library(kernel
  device.c
  errno.c
  fatal.c
  idle.c
  init.c
  kheap.c
  mailbox.c
  mem_slab.c
  mempool.c
  msg_q.c
  mutex.c
  pipes.c
  queue.c
  sched.c
  sem.c
  stack.c
  system_work_q.c
  thread.c
  thread_abort.c
  version.c
  work_q.c
  smp.c
  )

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

if(${CONFIG_MEM_POOL_HEAP_BACKEND})
else()
  target_sources(kernel PRIVATE mempool_sys.c)
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
  cache_handlers.c
  userspace_handler.c
  userspace.c
  )

target_include_directories(kernel PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

add_dependencies(kernel zephyr_generated_headers)

target_link_libraries(kernel zephyr_interface)
