add_library(kernel
  alert.c
  device.c
  errno.c
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
  sys_clock.c
  system_work_q.c
  thread.c
  thread_abort.c
  version.c
  work_q.c
  ${PROJECT_BINARY_DIR}/include/generated/version.h
)

target_sources_ifdef(CONFIG_INT_LATENCY_BENCHMARK kernel PRIVATE int_latency_bench.c)
target_sources_ifdef(CONFIG_STACK_CANARIES        kernel PRIVATE compiler_stack_protect.c)
target_sources_ifdef(CONFIG_SYS_CLOCK_EXISTS      kernel PRIVATE timer.c)
target_sources_ifdef(CONFIG_LEGACY_KERNEL         kernel PRIVATE legacy_timer.c)
target_sources_ifdef(CONFIG_ATOMIC_OPERATIONS_C   kernel PRIVATE atomic_c.c)

target_sources_if_kconfig(                        kernel PRIVATE poll.c)

add_dependencies(kernel offsets_h)

target_link_libraries(kernel zephyr)
