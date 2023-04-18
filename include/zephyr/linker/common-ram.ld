/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_NETWORKING)
#ifndef NETWORK_RAM_SECTIONS
#define NETWORK_RAM_SECTIONS \
	ITERABLE_SECTION_RAM(net_if, 4) \
	ITERABLE_SECTION_RAM(net_if_dev, 4) \
	ITERABLE_SECTION_RAM(net_l2, 4) \
	ITERABLE_SECTION_RAM(eth_bridge, 4)
#endif
#endif /* NETWORKING */

#if defined(CONFIG_BT_MESH)
	ITERABLE_SECTION_RAM(bt_mesh_ext_adv, 4)
#endif

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
#endif

        SECTION_DATA_PROLOGUE(device_states,,)
        {
		/* Device states used by the device objects. */
                __device_states_start = .;
		KEEP(*(".z_devstate"));
		KEEP(*(".z_devstate.*"));
                __device_states_end = .;
        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if CONFIG_PM_DEVICE
	SECTION_DATA_PROLOGUE(pm_device_slots, (NOLOAD),)
	{
		__pm_device_slots_start = .;
		KEEP(*(".z_pm_device_slots"));
		__pm_device_slots_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#if defined(CONFIG_HAS_DYNAMIC_DEVICE_HANDLES)
	SECTION_DATA_PROLOGUE(device_handles,,)
	{
#include "device-handles.ld"
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif 	/* CONFIG_HAS_DYNAMIC_DEVICE_HANDLES */

	SECTION_DATA_PROLOGUE(initshell,,)
	{
		/* link in shell initialization objects for all modules that
		 * use shell and their shell commands are automatically
		 * initialized by the kernel.
		 */
		__shell_module_start = .;
		KEEP(*(".shell_module_*"));
		__shell_module_end = .;
		__shell_cmd_start = .;
		KEEP(*(".shell_cmd_*"));
		__shell_cmd_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_RAM_GC_ALLOWED(log_mpsc_pbuf, 4)
	ITERABLE_SECTION_RAM(log_msg_ptr, 4)

	SECTION_DATA_PROLOGUE(log_dynamic_sections,,)
	{
		__log_dynamic_start = .;
		KEEP(*(SORT(.log_dynamic_*)));
		__log_dynamic_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_USERSPACE
	/* All kernel objects within are assumed to be either completely
	 * initialized at build time, or initialized automatically at runtime
	 * via iteration before the POST_KERNEL phase.
	 *
	 * These two symbols only used by gen_kobject_list.py
	 */

	_static_kernel_objects_begin = .;
#endif /* CONFIG_USERSPACE */

	ITERABLE_SECTION_RAM_GC_ALLOWED(k_timer, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_mem_slab, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_heap, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_mutex, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_stack, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_msgq, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_mbox, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_pipe, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_sem, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_event, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_queue, 4)
	ITERABLE_SECTION_RAM_GC_ALLOWED(k_condvar, 4)

	SECTION_DATA_PROLOGUE(_net_buf_pool_area,,SUBALIGN(4))
	{
		_net_buf_pool_list = .;
		KEEP(*(SORT_BY_NAME("._net_buf_pool.static.*")))
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_NETWORKING)
	NETWORK_RAM_SECTIONS
#endif /* NETWORKING */

#if defined(CONFIG_PCIE)
	ITERABLE_SECTION_RAM(pcie_dev, 4)
#endif /* PCIE */

#if defined(CONFIG_UART_MUX)
	SECTION_DATA_PROLOGUE(uart_mux,,SUBALIGN(4))
	{
		__uart_mux_start = .;
		*(".uart_mux.*")
		KEEP(*(SORT_BY_NAME(".uart_mux.*")))
		__uart_mux_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#if defined(CONFIG_USB_DEVICE_STACK)
	SECTION_DATA_PROLOGUE(usb_descriptor,,SUBALIGN(1))
	{
		__usb_descriptor_start = .;
		*(".usb.descriptor")
		KEEP(*(SORT_BY_NAME(".usb.descriptor*")))
		__usb_descriptor_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_RAM(usb_cfg_data, 4)
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
	ITERABLE_SECTION_RAM(rtio, 4)
	ITERABLE_SECTION_RAM(rtio_iodev, 4)
#endif /* CONFIG_RTIO */


#ifdef CONFIG_USERSPACE
	_static_kernel_objects_end = .;
#endif
