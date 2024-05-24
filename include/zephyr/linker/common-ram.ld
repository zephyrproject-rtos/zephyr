/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_NETWORKING)
#ifndef NETWORK_RAM_SECTIONS
#define NETWORK_RAM_SECTIONS \
	ITERABLE_SECTION_RAM(net_if, Z_LINK_ITERABLE_SUBALIGN) \
	ITERABLE_SECTION_RAM(net_if_dev, Z_LINK_ITERABLE_SUBALIGN) \
	ITERABLE_SECTION_RAM(net_l2, Z_LINK_ITERABLE_SUBALIGN) \
	ITERABLE_SECTION_RAM(eth_bridge, Z_LINK_ITERABLE_SUBALIGN)
#endif
#endif /* NETWORKING */

#if defined(CONFIG_GEN_SW_ISR_TABLE) && defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_DATA_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(_SW_ISR_TABLE_SECTION_SYMS)
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_SHARED_INTERRUPTS)
	SECTION_DATA_PROLOGUE(shared_sw_isr_table,,)
	{
		/* TODO: does this section require alignment? */
		KEEP(*(_SHARED_SW_ISR_TABLE_SECTION_SYMS))
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#endif

        SECTION_DATA_PROLOGUE(device_states,,)
        {
		/* Device states used by the device objects. */
                __device_states_start = .;
		KEEP(*(".z_devstate"));
		KEEP(*(".z_devstate.*"));
                __device_states_end = .;
        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_PM_DEVICE
	ITERABLE_SECTION_RAM(pm_device_slots, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_DEVICE_DEPS_DYNAMIC)
	SECTION_DATA_PROLOGUE(device_deps,,)
	{
#include "device-deps.ld"
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif 	/* CONFIG_DEVICE_DEPS_DYNAMIC */

	ITERABLE_SECTION_RAM_GC_ALLOWED(log_mpsc_pbuf, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM(log_msg_ptr, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM(log_dynamic, Z_LINK_ITERABLE_SUBALIGN)

#ifdef CONFIG_USERSPACE
	/* All kernel objects within are assumed to be either completely
	 * initialized at build time, or initialized automatically at runtime
	 * via iteration before the POST_KERNEL phase.
	 *
	 * These two symbols only used by gen_kobject_list.py
	 */

	_static_kernel_objects_begin = .;
#endif /* CONFIG_USERSPACE */

	ITERABLE_SECTION_RAM_GC_ALLOWED(k_timer, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_mem_slab, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_heap, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_mutex, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_stack, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_msgq, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_mbox, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_pipe, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_sem, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_event, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_queue, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_fifo, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_lifo, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_condvar, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM_GC_ALLOWED(sys_mem_blocks_ptr, Z_LINK_ITERABLE_SUBALIGN)

	ITERABLE_SECTION_RAM(net_buf_pool, Z_LINK_ITERABLE_SUBALIGN)

#if defined(CONFIG_NETWORKING)
	NETWORK_RAM_SECTIONS
#endif /* NETWORKING */

#if defined(CONFIG_PCIE)
	ITERABLE_SECTION_RAM(pcie_dev, Z_LINK_ITERABLE_SUBALIGN)
#endif /* PCIE */

#if defined(CONFIG_USB_DEVICE_STACK)
	SECTION_DATA_PROLOGUE(usb_descriptor,,SUBALIGN(1))
	{
		__usb_descriptor_start = .;
		*(".usb.descriptor")
		KEEP(*(SORT_BY_NAME(".usb.descriptor*")))
		__usb_descriptor_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_RAM(usb_cfg_data, Z_LINK_ITERABLE_SUBALIGN)
#endif /* CONFIG_USB_DEVICE_STACK */

#if defined(CONFIG_USB_DEVICE_BOS)
	SECTION_DATA_PROLOGUE(usb_bos_desc,,SUBALIGN(1))
	{
		__usb_bos_desc_start = .;
		*(".usb.bos_desc")
		KEEP(*(SORT_BY_NAME(".usb.bos_desc*")))
		__usb_bos_desc_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_USB_DEVICE_BOS */

#if defined(CONFIG_RTIO)
	ITERABLE_SECTION_RAM(rtio, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM(rtio_iodev, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM(rtio_sqe_pool, Z_LINK_ITERABLE_SUBALIGN)
	ITERABLE_SECTION_RAM(rtio_cqe_pool, Z_LINK_ITERABLE_SUBALIGN)
#endif /* CONFIG_RTIO */

#if defined(CONFIG_SENSING)
	ITERABLE_SECTION_RAM(sensing_sensor, Z_LINK_ITERABLE_SUBALIGN)
#endif /* CONFIG_SENSING */

#if defined(CONFIG_ZBUS)
	ITERABLE_SECTION_RAM(zbus_channel_observation_mask, 1)
#endif /* CONFIG_ZBUS */

#if defined(CONFIG_DEVICE_MUTABLE)
	ITERABLE_SECTION_RAM(device_mutable, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_BT_ZEPHYR_NUS)
	ITERABLE_SECTION_RAM(bt_nus_inst, Z_LINK_ITERABLE_SUBALIGN)
#endif

#ifdef CONFIG_USERSPACE
	_static_kernel_objects_end = .;
#endif
