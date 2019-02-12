cmake_minimum_required(VERSION 3.13.1)
set(small_freq_divider_frdm_k64f TRUE)
set(small_freq_divider_arduino_due TRUE)
set(small_freq_divider_qemu_cortex_m3 TRUE)

set(CONF_FILE prj.conf)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(latency_measure)

# eliminate timer interrupts during the benchmark
if(small_freq_divider_${BOARD})
  if(${CONFIG_CORTEX_M_SYSTICK})
    # calculate the lowest CONFIG_SYS_CLOCK_TICKS_PER_SEC which will not exceed 24 bit SysTick Reload Value
    math(EXPR CONFIG_SYS_CLOCK_TICKS_PER_SEC "((${CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC} & 0xFF000000) >> 24) + 1")
  else()
    message(FATAL_ERROR "Only platforms with cortex-m systick are supported for small freq divider")
  endif()
else()
  set(CONFIG_SYS_CLOCK_TICKS_PER_SEC 1)
  set(CONFIG_TICKLESS_KERNEL n)
endif()

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
