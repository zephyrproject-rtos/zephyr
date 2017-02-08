target_sources_ifdef(CONFIG_CPLUSPLUS zephyr PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp_virtual.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp_vtable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp_init_array.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp_ctors.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cpp_dtors.c
)

target_sources_if_kconfig(zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/printk.c)
target_sources_if_kconfig(zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/reboot.c)
target_sources_if_kconfig(zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ring_buffer.c)
