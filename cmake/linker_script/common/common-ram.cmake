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
  zephyr_iterable_section(NAME pm_device_slots GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

zephyr_iterable_section(NAME log_dynamic GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)

if(CONFIG_USERSPACE)
  # All kernel objects within are assumed to be either completely
  # initialized at build time, or initialized automatically at runtime
  # via iteration before the POST_KERNEL phase.
  #
  # These two symbols only used by gen_kobject_list.py
  #   _static_kernel_objects_begin = .;
endif()

zephyr_iterable_section(NAME k_timer GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_mem_slab GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_heap GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_mutex GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_stack GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_msgq GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_mbox GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_pipe GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_sem GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_queue GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_condvar GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_event GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
zephyr_iterable_section(NAME k_fifo GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)

zephyr_iterable_section(NAME net_buf_pool GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)

if(CONFIG_NETWORKING)
  zephyr_iterable_section(NAME net_if     GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME net_if_dev GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME net_l2     GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME eth_bridge GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

if(CONFIG_SENSING)
  zephyr_iterable_section(NAME sensing_sensor GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
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

if(CONFIG_RTIO)
  zephyr_iterable_section(NAME rtio GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME rtio_iodev GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

#if(CONFIG_USERSPACE)
#	_static_kernel_objects_end = .;
#endif()
#

if(CONFIG_ZTEST)
  zephyr_iterable_section(NAME ztest_suite_node GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME ztest_suite_stats GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME ztest_unit_test GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME ztest_test_rule GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME ztest_expected_result_entry GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

if(CONFIG_ZBUS)
  zephyr_iterable_section(NAME zbus_channel_observation_mask GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
endif()

if(CONFIG_UVB)
  zephyr_iterable_section(NAME uvb_node GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()


if(CONFIG_LOG)
  zephyr_iterable_section(NAME log_mpsc_pbuf GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME log_msg_ptr GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

if(CONFIG_PCIE)
  zephyr_iterable_section(NAME pcie_dev GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

if(CONFIG_USB_DEVICE_STACK OR CONFIG_USB_DEVICE_STACK_NEXT)
  zephyr_iterable_section(NAME usb_cfg_data GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME usbd_context GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME usbd_class_fs GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME usbd_class_hs GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

if(CONFIG_USB_HOST_STACK)
  zephyr_iterable_section(NAME usbh_contex GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
  zephyr_iterable_section(NAME usbh_class_data GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()

if(CONFIG_DEVICE_MUTABLE)
  zephyr_iterable_section(NAME device_mutable GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()
