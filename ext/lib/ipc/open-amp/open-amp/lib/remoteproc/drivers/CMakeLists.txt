if ("${MACHINE}" STREQUAL "zynqmp_r5")
  collect (PROJECT_LIB_SOURCES zynqmp_remoteproc_a53.c)
endif ("${MACHINE}" STREQUAL "zynqmp_r5")

if ("${MACHINE}" STREQUAL "zynq7")
  collect (PROJECT_LIB_SOURCES zynq_remoteproc_a9.c)
  collect (PROJECT_LIB_SOURCES zynq_a9_trampoline.S)
endif ("${MACHINE}" STREQUAL "zynq7")

if ("${MACHINE}" STREQUAL "zynqmp")
  collect (PROJECT_LIB_SOURCES zynqmp_remoteproc_r5.c)
endif ("${MACHINE}" STREQUAL "zynqmp")

if ("${PROJECT_SYSTEM}" STREQUAL "linux")
  collect (PROJECT_LIB_SOURCES linux_remoteproc.c)
endif ("${PROJECT_SYSTEM}" STREQUAL "linux")
