add_definitions(-D__ZEPHYR_SUPERVISOR__)

add_library(arch_${ARCH} STATIC "")
target_link_libraries(arch_${ARCH} PUBLIC zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE arch_${ARCH})
zephyr_set_property(TARGET arch_${ARCH} MEMORY_SPACE kernel)

add_subdirectory(common)
include_relative(${ARCH}/CMakeLists.txt)
