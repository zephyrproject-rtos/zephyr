add_definitions(-D__ZEPHYR_SUPERVISOR__)

add_library(arch_${ARCH} STATIC "")
target_link_libraries(arch_${ARCH} PUBLIC zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE arch_${ARCH})

add_subdirectory(common)

if( ( ${ARCH} STREQUAL arm ) OR
    ( ${ARCH} STREQUAL arc ) OR
    ( ${ARCH} STREQUAL posix ) OR
    ( ${ARCH} STREQUAL riscv32 ) OR
    ( ${ARCH} STREQUAL nios2 )
)
  include_relative(${ARCH}/CMakeLists.txt)
else()
  # Not updated archs
  add_subdirectory(${ARCH})
  # To ensure a file is always present in arch not updated yet.
  target_sources(arch_${ARCH} PRIVATE $ENV{ZEPHYR_BASE}/misc/empty_file.c)
endif()
