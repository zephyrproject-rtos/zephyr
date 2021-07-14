# originates from common-ram.ld

if(CONFIG_GEN_SW_ISR_TABLE AND CONFIG_DYNAMIC_INTERRUPTS)
  # ld align has been changed to subalign to provide identical behavior scatter vs. ld.
  zephyr_linker_section(NAME sw_isr_table
    VMA RAM LMA FLASH NOINPUT
    SUBALIGN ${CONFIG_ARCH_SW_ISR_TABLE_ALIGN}
  )
  zephyr_linker_section_configure(
    SECTION sw_isr_table
    INPUT ".gnu.linkonce.sw_isr_table*"
  )
  # GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
endif()

zephyr_linker_section(NAME device VMA RAM LMA FLASH)
zephyr_linker_section_obj_level(SECTION device LEVEL PRE_KERNEL_1)
zephyr_linker_section_obj_level(SECTION device LEVEL PRE_KERNEL_2)
zephyr_linker_section_obj_level(SECTION device LEVEL POST_KERNEL)
zephyr_linker_section_obj_level(SECTION device LEVEL APPLICATION)
zephyr_linker_section_obj_level(SECTION device LEVEL SMP)
# GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

zephyr_linker_section(NAME initshell VMA RAM LMA FLASH NOINPUT)
zephyr_linker_section_configure(SECTION initshell
  KEEP INPUT ".shell_module_*"
  SYMBOLS __shell_module_start __shell_module_end
)
zephyr_linker_section_configure(SECTION initshell
  KEEP INPUT ".shell_cmd_*"
  SYMBOLS __shell_cmd_start __shell_end_end
)
# GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

zephyr_linker_section(NAME log_dynamic VMA RAM LMA FLASH NOINPUT)
zephyr_linker_section_configure(SECTION log_dynamic KEEP INPUT ".log_dynamic_*")
# GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

zephyr_iterable_section(NAME _static_thread_data VMA RAM LMA FLASH SUBALIGN 4)
# Z_ITERABLE_SECTION_RAM(_static_thread_data, 4)

if(CONFIG_USERSPACE)
  # All kernel objects within are assumed to be either completely
  # initialized at build time, or initialized automatically at runtime
  # via iteration before the POST_KERNEL phase.
  #
  # These two symbols only used by gen_kobject_list.py
#	_static_kernel_objects_begin = .;
endif()
#
zephyr_iterable_section(NAME k_timer VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_timer, 4)
zephyr_iterable_section(NAME k_mem_slab VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mem_slab, 4)
zephyr_iterable_section(NAME k_mem_pool VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mem_pool, 4)
zephyr_iterable_section(NAME k_heap VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_heap, 4)
zephyr_iterable_section(NAME k_mutex VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mutex, 4)
zephyr_iterable_section(NAME k_stack VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_stack, 4)
zephyr_iterable_section(NAME k_msgq VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_msgq, 4)
zephyr_iterable_section(NAME k_mbox VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mbox, 4)
zephyr_iterable_section(NAME k_pipe VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_pipe, 4)
zephyr_iterable_section(NAME k_sem VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_sem, 4)
zephyr_iterable_section(NAME k_queue VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_queue, 4)
zephyr_iterable_section(NAME k_condvar VMA RAM LMA FLASH SUBALIGN 4)
#	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_condvar, 4)

zephyr_linker_section(NAME _net_buf_pool_area VMA RAM LMA FLASH NOINPUT SUBALIGN 4)
zephyr_linker_section_configure(SECTION _net_buf_pool_area
  KEEP SORT NAME INPUT "._net_buf_pool.static.*"
  SYMBOLS _net_buf_pool_list
)
# SECTION_DATA_PROLOGUE(_net_buf_pool_area,,SUBALIGN(4))
# GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#
#if(CONFIG_NETWORKING)
#	Z_ITERABLE_SECTION_RAM(net_if, 4)
#	Z_ITERABLE_SECTION_RAM(net_if_dev, 4)
#	Z_ITERABLE_SECTION_RAM(net_l2, 4)
#endif() /* NETWORKING */
#
#if(CONFIG_UART_MUX)
#	SECTION_DATA_PROLOGUE(uart_mux,,SUBALIGN(4))
#	{
#		__uart_mux_start = .;
#		*(".uart_mux.*")
#		KEEP(*(SORT_BY_NAME(".uart_mux.*")))
#		__uart_mux_end = .;
#	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif()
#
#if(CONFIG_USB_DEVICE_STACK)
#	SECTION_DATA_PROLOGUE(usb_descriptor,,SUBALIGN(1))
#	{
#		__usb_descriptor_start = .;
#		*(".usb.descriptor")
#		KEEP(*(SORT_BY_NAME(".usb.descriptor*")))
#		__usb_descriptor_end = .;
#	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#
#	SECTION_DATA_PROLOGUE(usb_data,,SUBALIGN(1))
#	{
#		__usb_data_start = .;
#		*(".usb.data")
#		KEEP(*(SORT_BY_NAME(".usb.data*")))
#		__usb_data_end = .;
#	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif() /* CONFIG_USB_DEVICE_STACK */
#
#if(CONFIG_USB_DEVICE_BOS)
#	SECTION_DATA_PROLOGUE(usb_bos_desc,,SUBALIGN(1))
#	{
#		__usb_bos_desc_start = .;
#		*(".usb.bos_desc")
#		KEEP(*(SORT_BY_NAME(".usb.bos_desc*")))
#		__usb_bos_desc_end = .;
#	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif() /* CONFIG_USB_DEVICE_BOS */
#
#if(CONFIG_USERSPACE)
#	_static_kernel_objects_end = .;
#endif()
#
