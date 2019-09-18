# SPDX-License-Identifier: Apache-2.0

# kernel is a normal CMake library and not a zephyr_library because it
# should not be --whole-archive'd
add_library(${KERNEL_LIBRARY}
  device.c
  errno.c
  fatal.c
  idle.c
  init.c
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
  ${KERNEL_LIBRARY}
  PROPERTIES
  COMPILE_DEFINITIONS
  __ZEPHYR_SUPERVISOR__
  )

target_sources_ifdef(CONFIG_STACK_CANARIES        ${KERNEL_LIBRARY} PRIVATE compiler_stack_protect.c)
target_sources_ifdef(CONFIG_SYS_CLOCK_EXISTS      ${KERNEL_LIBRARY} PRIVATE timeout.c timer.c)
target_sources_ifdef(CONFIG_ATOMIC_OPERATIONS_C   ${KERNEL_LIBRARY} PRIVATE atomic_c.c)
target_sources_if_kconfig(                        ${KERNEL_LIBRARY} PRIVATE poll.c)

# The last 2 files inside the target_sources_ifdef should be
# userspace_handler.c and userspace.c. If not the linker would complain.
# This order has to be maintained. Any new file should be placed
# above these 2 files.
target_sources_ifdef(
  CONFIG_USERSPACE
  ${KERNEL_LIBRARY} PRIVATE
  futex.c
  mem_domain.c
  userspace_handler.c
  userspace.c
  )


add_dependencies(${KERNEL_LIBRARY} ${OFFSETS_H_TARGET})

target_link_libraries(${KERNEL_LIBRARY} ${IMAGE}zephyr_interface)
