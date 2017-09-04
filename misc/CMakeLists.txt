zephyr_sources_ifdef(CONFIG_CPLUSPLUS
  cpp_virtual.c
  cpp_vtable.c
  cpp_init_array.c
  cpp_ctors.c
  cpp_dtors.c
)

zephyr_sources_if_kconfig(printk.c)
zephyr_sources_if_kconfig(reboot.c)
zephyr_sources_if_kconfig(ring_buffer.c)
