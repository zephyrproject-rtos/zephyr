set (PROJECT_VER_MAJOR  0)
set (PROJECT_VER_MINOR  1)
set (PROJECT_VER_PATCH  0)
set (PROJECT_VER        0.1.0)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

if (NOT CMAKE_INSTALL_LIBDIR)
  set (CMAKE_INSTALL_LIBDIR "lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

if (NOT CMAKE_INSTALL_BINDIR)
  set (CMAKE_INSTALL_BINDIR "bin")
endif (NOT CMAKE_INSTALL_BINDIR)

set (_host "${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_HOST_SYSTEM_PROCESSOR}")
message ("-- Host:    ${_host}")

set (_target "${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
message ("-- Target:  ${_target}")

if (NOT DEFINED MACHINE)
  set (MACHINE "Generic")
endif (NOT DEFINED MACHINE)
message ("-- Machine: ${MACHINE}")

string (TOLOWER ${CMAKE_SYSTEM_NAME}      PROJECT_SYSTEM)
string (TOUPPER ${CMAKE_SYSTEM_NAME}      PROJECT_SYSTEM_UPPER)
string (TOLOWER ${CMAKE_SYSTEM_PROCESSOR} PROJECT_PROCESSOR)
string (TOUPPER ${CMAKE_SYSTEM_PROCESSOR} PROJECT_PROCESSOR_UPPER)
string (TOLOWER ${MACHINE}                PROJECT_MACHINE)
string (TOUPPER ${MACHINE}                PROJECT_MACHINE_UPPER)

# Select to build Remote proc master
option (WITH_REMOTEPROC_MASTER "Build as remoteproc master" OFF)
if (WITH_REMOTEPROC_MASTER)
  option (WITH_LINUXREMOTE  "The remote is Linux" ON)
endif (WITH_REMOTEPROC_MASTER)

# Select which components are in the openamp lib
option (WITH_PROXY          "Build with proxy(access device controlled by other processor)" ON)
option (WITH_APPS           "Build with sample applicaitons" OFF)
option (WITH_PROXY_APPS     "Build with proxy sample applicaitons" OFF)
if (WITH_APPS)
  if (WITH_PROXY)
    set (WITH_PROXY_APPS ON)
  elseif ("${PROJECT_SYSTEM}" STREQUAL "linux")
    set (WITH_PROXY_APPS ON)
  endif (WITH_PROXY)
  option (WITH_BENCHMARK    "Build benchmark app" OFF)
endif (WITH_APPS)
option (WITH_OBSOLETE       "Build obsolete system libs" OFF)

# Set the complication flags
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

if (WITH_LINUXREMOTE)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENAMP_REMOTE_LINUX_ENABLE")
endif (WITH_LINUXREMOTE)

if (WITH_BENCHMARK)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPENAMP_BENCHMARK_ENABLE")
endif (WITH_BENCHMARK)

option (WITH_STATIC_LIB "Build with a static library" ON)

if ("${PROJECT_SYSTEM}" STREQUAL "linux")
  option (WITH_SHARED_LIB "Build with a shared library" ON)
endif ("${PROJECT_SYSTEM}" STREQUAL "linux")

if (WITH_ZEPHYR)
  option (WITH_ZEPHYR_LIB "Build open-amp as a zephyr library" OFF)
endif (WITH_ZEPHYR)

option (WITH_LIBMETAL_FIND "Check Libmetal library can be found" ON)
option (WITH_EXT_INCLUDES_FIND "Check other external includes are found" ON)

message ("-- C_FLAGS : ${CMAKE_C_FLAGS}")
# vim: expandtab:ts=2:sw=2:smartindent
