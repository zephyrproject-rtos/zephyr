# SPDX-License-Identifier: Apache-2.0
# The contents of this file is based on include/zephyr/linker/common-ram.ld
# Please keep in sync

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
  if(CONFIG_SHARED_INTERRUPTS)
    zephyr_linker_section_configure(
      SECTION sw_isr_table
      INPUT ".gnu.linkonce.shared_sw_isr_table*"
    )
  endif()
endif()

zephyr_linker_section(NAME device_states GROUP DATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION device_states
  KEEP INPUT ".z_devstate" ".z_devstate.*"
)

if(CONFIG_PM_DEVICE)
  zephyr_iterable_section(NAME pm_device_slots GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

zephyr_iterable_section(NAME log_dynamic GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

if(CONFIG_USERSPACE)
  set(K_OBJECTS_GROUP "K_OBJECTS_IN_DATA_REGION")
  # All kernel objects within are assumed to be either completely
  # initialized at build time, or initialized automatically at runtime
  # via iteration before the POST_KERNEL phase.
  #
  zephyr_linker_group(NAME ${K_OBJECTS_GROUP} GROUP DATA_REGION SYMBOL SECTION)

  # gen_kobject_list.py expects the start and end symbols to be called
  # _static_kernel_objects_begin and _static_kernel_objects_end respectively...
  zephyr_linker_symbol(
    SYMBOL
    _static_kernel_objects_begin
    EXPR
    "(@__k_objects_in_data_region_start@)"
    )
  zephyr_linker_symbol(
    SYMBOL
    _static_kernel_objects_end
    EXPR
    "(@__k_objects_in_data_region_end@)"
    )
else()
  set(K_OBJECTS_GROUP "DATA_REGION")
endif()

zephyr_iterable_section(NAME k_timer GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_mem_slab GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_heap GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_mutex GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_stack GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_msgq GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_mbox GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_pipe GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_sem GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_event GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_queue GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_fifo GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_lifo GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME k_condvar GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
zephyr_iterable_section(NAME sys_mem_blocks_ptr GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

zephyr_iterable_section(NAME net_buf_pool GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

if(CONFIG_NETWORKING)
  zephyr_iterable_section(NAME net_if     GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME net_if_dev GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME net_l2     GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME eth_bridge GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_ARM_SCMI)
  zephyr_iterable_section(NAME scmi_protocol GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_SENSING)
  zephyr_iterable_section(NAME sensing_sensor GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_USB_DEVICE_STACK)
  zephyr_linker_section(NAME usb_descriptor GROUP ${K_OBJECTS_GROUP} NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
  zephyr_linker_section_configure(SECTION usb_descriptor
    KEEP SORT NAME INPUT ".usb.descriptor*"
  )

  zephyr_iterable_section(NAME usb_cfg_data GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_USB_DEVICE_BOS)
  zephyr_linker_section(NAME usb_bos_desc GROUP ${K_OBJECTS_GROUP} NOINPUT ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
  zephyr_linker_section_configure(SECTION usb_data
    KEEP SORT NAME INPUT ".usb.bos_desc"
  )
endif()

if(CONFIG_RTIO)
  zephyr_iterable_section(NAME rtio GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME rtio_iodev GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME rtio_sqe_pool GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME rtio_cqe_pool GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_SENSING)
  zephyr_iterable_section(NAME sensing_sensor GROUP ${K_OBJECTS_GROUP} ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_ZBUS)
  zephyr_iterable_section(NAME zbus_channel_observation_mask GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 1)
endif()

if(CONFIG_UVB)
  zephyr_iterable_section(NAME uvb_node GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_VIDEO)
  zephyr_iterable_section(NAME video_device GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_LOG)
  zephyr_iterable_section(NAME log_mpsc_pbuf GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME log_msg_ptr GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_PCIE)
  zephyr_iterable_section(NAME pcie_dev GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_USB_DEVICE_STACK OR CONFIG_USB_DEVICE_STACK_NEXT)
  zephyr_iterable_section(NAME usbd_context GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME usbd_class_fs GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME usbd_class_hs GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_USB_HOST_STACK)
  zephyr_iterable_section(NAME usbh_contex GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
  zephyr_iterable_section(NAME usbh_class_data GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_DEVICE_MUTABLE)
  zephyr_iterable_section(NAME device_mutable GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()
