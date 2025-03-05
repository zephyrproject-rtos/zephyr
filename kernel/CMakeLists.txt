# SPDX-License-Identifier: Apache-2.0

# kernel is a normal CMake library and not a zephyr_library because it
# should usually not be --whole-archive'd

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/device.h
  ${ZEPHYR_BASE}/include/zephyr/kernel.h
  ${ZEPHYR_BASE}/include/zephyr/sys/kobject.h
  ${ZEPHYR_BASE}/include/zephyr/sys/time_units.h
)

if(NOT CONFIG_ERRNO_IN_TLS AND NOT CONFIG_LIBC_ERRNO)
  zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/sys/errno_private.h)
endif()

zephyr_syscall_header_ifdef(
  CONFIG_ATOMIC_OPERATIONS_C
  ${ZEPHYR_BASE}/include/zephyr/sys/atomic_c.h
)

zephyr_syscall_header_ifdef(
  CONFIG_MMU
  ${ZEPHYR_BASE}/include/zephyr/kernel/mm.h
  ${ZEPHYR_BASE}/include/zephyr/sys/mem_manage.h
)

zephyr_syscall_header_ifdef(
  CONFIG_DEMAND_PAGING
  ${ZEPHYR_BASE}/include/zephyr/kernel/mm/demand_paging.h
)

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

# FIXME: SHADOW_VARS: Remove this once we have enabled -Wshadow globally.
add_compile_options($<TARGET_PROPERTY:compiler,warning_shadow_variables>)

list(APPEND kernel_files
  main_weak.c
  banner.c
  busy_wait.c
  device.c
  errno.c
  fatal.c
  init.c
  init_static.c
  kheap.c
  mem_slab.c
  float.c
  version.c
  )

if(CONFIG_SCHED_CPU_MASK)
list(APPEND kernel_files
  cpu_mask.c
  )
endif()

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
  condvar.c
  thread.c
  sched.c
  )

if (CONFIG_SCHED_SCALABLE OR CONFIG_WAITQ_SCALABLE)
list(APPEND kernel_files priority_queues.c)
endif()

# FIXME: Once the prior pipe implementation is removed, this should be included in the above list
if(NOT CONFIG_PIPES)
list(APPEND kernel_files pipe.c)
endif() # NOT CONFIG_PIPES
if(CONFIG_SMP)
list(APPEND kernel_files
     smp.c
     ipi.c)
endif()
else() # CONFIG_MULTITHREADING
list(APPEND kernel_files
  nothread.c
  )
endif() # CONFIG_MULTITHREADING

if(CONFIG_TIMESLICING)
list(APPEND kernel_files
     timeslicing.c)
endif()

if(CONFIG_SPIN_VALIDATE)
list(APPEND kernel_files
     spinlock_validate.c)
endif()

if(CONFIG_IRQ_OFFLOAD)
list(APPEND kernel_files
  irq_offload.c
  )
endif()


if(CONFIG_THREAD_MONITOR)
list(APPEND kernel_files
     thread_monitor.c)
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

target_sources_ifdef(CONFIG_REQUIRES_STACK_CANARIES   kernel PRIVATE compiler_stack_protect.c)
target_sources_ifdef(CONFIG_SYS_CLOCK_EXISTS      kernel PRIVATE timeout.c timer.c)
target_sources_ifdef(CONFIG_ATOMIC_OPERATIONS_C   kernel PRIVATE atomic_c.c)
target_sources_ifdef(CONFIG_MMU                   kernel PRIVATE mmu.c)
target_sources_ifdef(CONFIG_POLL                  kernel PRIVATE poll.c)
target_sources_ifdef(CONFIG_EVENTS                kernel PRIVATE events.c)
target_sources_ifdef(CONFIG_PIPES                 kernel PRIVATE pipes.c)
target_sources_ifdef(CONFIG_SCHED_THREAD_USAGE    kernel PRIVATE usage.c)
target_sources_ifdef(CONFIG_OBJ_CORE              kernel PRIVATE obj_core.c)

if(${CONFIG_KERNEL_MEM_POOL})
  target_sources(kernel PRIVATE mempool.c)

  if(CONFIG_HEAP_MEM_POOL_IGNORE_MIN)
    set(final_heap_size ${CONFIG_HEAP_MEM_POOL_SIZE})
  else()
    # Import all custom HEAP_MEM_POOL size requirements
    import_kconfig(CONFIG_HEAP_MEM_POOL_ADD_SIZE_ ${DOTCONFIG} add_size_keys)

    # Calculate the sum of all "ADD_SIZE" requirements
    set(add_size_sum 0)
    foreach(add_size ${add_size_keys})
      math(EXPR add_size_sum "${add_size_sum} + ${${add_size}}")
    endforeach()

    if(CONFIG_HEAP_MEM_POOL_SIZE LESS "${add_size_sum}")
      # Only warn if default value 0 has been modified
      if(NOT CONFIG_HEAP_MEM_POOL_SIZE EQUAL 0)
        message(WARNING "
        CONFIG_HEAP_MEM_POOL_SIZE is less than requested minimum:
          ${CONFIG_HEAP_MEM_POOL_SIZE} < ${add_size_sum}
        Setting the system heap size to ${add_size_sum}")
      endif()

      set(final_heap_size ${add_size_sum})
    else()
      # CONFIG_HEAP_MEM_POOL_SIZE was greater than the sum of the requirements
      set(final_heap_size ${CONFIG_HEAP_MEM_POOL_SIZE})
    endif()

  endif()

  zephyr_compile_definitions(K_HEAP_MEM_POOL_SIZE=${final_heap_size})
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

if(${CONFIG_DYNAMIC_THREAD})
  target_sources(kernel PRIVATE dynamic.c)
else()
  target_sources(kernel PRIVATE dynamic_disabled.c)
endif()

target_include_directories(kernel PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

target_link_libraries(kernel zephyr_interface)

endif()

add_dependencies(kernel zephyr_generated_headers)

unset(libkernel)
