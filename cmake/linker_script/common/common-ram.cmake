# originates from common-ram.ld

if(CONFIG_GEN_SW_ISR_TABLE AND CONFIG_DYNAMIC_INTERRUPTS)
  # ld align has been changed to subalign to provide identical behavior scatter vs. ld.
  zephyr_linker_section(NAME sw_isr_table
    GROUP DATA_REGION
    ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_ARCH_SW_ISR_TABLE_ALIGN}
  )
  zephyr_linker_section_configure(
    SECTION sw_isr_table
    INPUT ".gnu.linkonce.sw_isr_table*"
  )
endif()

zephyr_linker_section(NAME device_states GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION device_states
  KEEP INPUT ".z_devstate" ".z_devstate.*"
)

if(CONFIG_PM_DEVICE)
  zephyr_linker_section(NAME pm_device_slots GROUP DATA_REGION TYPE NOLOAD NOINPUT ${XIP_ALIGN_WITH_INPUT})
  zephyr_linker_section_configure(SECTION pm_device_slots KEEP INPUT ".z_pm_device_slots")
endif()

zephyr_linker_section(NAME initshell GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION initshell
  KEEP INPUT ".shell_module_*"
  SYMBOLS __shell_module_start __shell_module_end
)
zephyr_linker_section_configure(SECTION initshell
  KEEP INPUT ".shell_cmd_*"
  SYMBOLS __shell_cmd_start __shell_end_end
)

zephyr_linker_section(NAME log_dynamic GROUP DATA_REGION NOINPUT)
zephyr_linker_section_configure(SECTION log_dynamic KEEP INPUT ".log_dynamic_*")

zephyr_iterable_section(NAME _static_thread_data GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)

if(CONFIG_USERSPACE)
  # All kernel objects within are assumed to be either completely
  # initialized at build time, or initialized automatically at runtime
  # via iteration before the POST_KERNEL phase.
  #
  # These two symbols only used by gen_kobject_list.py
  #   _static_kernel_objects_begin = .;
endif()

zephyr_iterable_section(NAME k_timer GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_mem_slab GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_mem_pool GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_heap GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_mutex GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_stack GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_msgq GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_mbox GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_pipe GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_sem GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_queue GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_iterable_section(NAME k_condvar GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)

zephyr_linker_section(NAME _net_buf_pool_area GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
zephyr_linker_section_configure(SECTION _net_buf_pool_area
  KEEP SORT NAME INPUT "._net_buf_pool.static.*"
  SYMBOLS _net_buf_pool_list
)

if(CONFIG_NETWORKING)
  zephyr_iterable_section(NAME net_if     GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
  zephyr_iterable_section(NAME net_if_dev GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
  zephyr_iterable_section(NAME net_l2     GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
  zephyr_iterable_section(NAME eth_bridge GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
endif()

if(CONFIG_UART_MUX)
  zephyr_linker_section(NAME uart_mux GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
  zephyr_linker_section_configure(SECTION uart_mux
    KEEP SORT NAME INPUT ".uart_mux.*"
  )
endif()

if(CONFIG_USB_DEVICE_STACK)
  zephyr_linker_section(NAME usb_descriptor GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
  zephyr_linker_section_configure(SECTION usb_descriptor
    KEEP SORT NAME INPUT ".usb.descriptor*"
  )

  zephyr_linker_section(NAME usb_data GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
  zephyr_linker_section_configure(SECTION usb_data
    KEEP SORT NAME INPUT ".usb.data*"
  )
endif()

if(CONFIG_USB_DEVICE_BOS)
  zephyr_linker_section(NAME usb_bos_desc GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
  zephyr_linker_section_configure(SECTION usb_data
    KEEP SORT NAME INPUT ".usb.bos_desc"
  )
endif()

#if(CONFIG_USERSPACE)
#	_static_kernel_objects_end = .;
#endif()
#

if(CONFIG_ZTEST)
  zephyr_iterable_section(NAME ztest_suite_node GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
endif()
