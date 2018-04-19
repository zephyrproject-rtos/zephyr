# use "Generic" as CMAKE_SYSTEM_NAME

if (WITH_ZEPHYR)
  set (CMAKE_SYSTEM_NAME "Generic"        CACHE STRING "")
  string (TOLOWER "Zephyr"                PROJECT_SYSTEM)
  string (TOUPPER "Zephyr"                PROJECT_SYSTEM_UPPER)
  set(IS_TEST 1)
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  if (CONFIG_CPU_CORTEX_M)
    set (MACHINE "cortexm" CACHE STRING "")
  endif (CONFIG_CPU_CORTEX_M)
endif (WITH_ZEPHYR)
