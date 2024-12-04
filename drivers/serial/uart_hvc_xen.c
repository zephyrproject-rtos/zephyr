/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/console.h>
#include <zephyr/xen/events.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/hvm.h>
#include <zephyr/xen/public/io/console.h>
#include <zephyr/xen/public/sched.h>
#include <zephyr/xen/public/xen.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_hvc_xen, CONFIG_UART_LOG_LEVEL);

static struct hvc_xen_data xen_hvc_data = {0};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void hvc_uart_evtchn_cb(void *priv);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int read_from_ring(const struct device *dev, char *str, int len)
{
	int recv = 0;
	struct hvc_xen_data *hvc_data = dev->data;
	XENCONS_RING_IDX cons = hvc_data->intf->in_cons;
	XENCONS_RING_IDX prod = hvc_data->intf->in_prod;
	XENCONS_RING_IDX in_idx = 0;

	compiler_barrier();
	__ASSERT((prod - cons) <= sizeof(hvc_data->intf->in),
			"Invalid input ring buffer");

	while (cons != prod && recv < len) {
		in_idx = MASK_XENCONS_IDX(cons, hvc_data->intf->in);
		str[recv] = hvc_data->intf->in[in_idx];
		recv++;
		cons++;
	}

	compiler_barrier();
	hvc_data->intf->in_cons = cons;

	notify_evtchn(hvc_data->evtchn);
	return recv;
}

static int write_to_ring(const struct device *dev, const char *str, int len)
{
	int sent = 0;
	struct hvc_xen_data *hvc_data = dev->data;
	XENCONS_RING_IDX cons = hvc_data->intf->out_cons;
	XENCONS_RING_IDX prod = hvc_data->intf->out_prod;
	XENCONS_RING_IDX out_idx = 0;

	compiler_barrier();
	__ASSERT((prod - cons) <= sizeof(hvc_data->intf->out),
			"Invalid output ring buffer");

	while ((sent < len) && ((prod - cons) < sizeof(hvc_data->intf->out))) {
		out_idx = MASK_XENCONS_IDX(prod, hvc_data->intf->out);
		hvc_data->intf->out[out_idx] = str[sent];
		prod++;
		sent++;
	}

	compiler_barrier();
	hvc_data->intf->out_prod = prod;

	if (sent) {
		notify_evtchn(hvc_data->evtchn);
	}

	return sent;
}

static int xen_hvc_poll_in(const struct device *dev,
			unsigned char *c)
{
	int ret = 0;
	char temp;

	ret = read_from_ring(dev, &temp, sizeof(temp));
	if (!ret) {
		/* Char was not received */
		return -1;
	}

	*c = temp;
	return 0;
}

static void xen_hvc_poll_out(const struct device *dev,
			unsigned char c)
{
	/* Not a good solution (notifying HV every time), but needed for poll_out */
	(void) write_to_ring(dev, &c, sizeof(c));
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int xen_hvc_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			 int len)
{
	int ret = 0, sent = 0;

	while (len) {
		sent = write_to_ring(dev, tx_data, len);

		ret += sent;
		tx_data += sent;
		len -= sent;

		if (len) {
			/* Need to be able to read it from another domain */
			HYPERVISOR_sched_op(SCHEDOP_yield, NULL);
		}
	}

	return ret;
}

static int xen_hvc_fifo_read(const struct device *dev, uint8_t *rx_data,
			 const int size)
{
	return read_from_ring(dev, rx_data, size);
}

static void xen_hvc_irq_tx_enable(const struct device *dev)
{
	/*
	 * Need to explicitly call UART callback on TX enabling to
	 * process available buffered TX actions, because no HV events
	 * will be generated on tx_enable.
	 */
	hvc_uart_evtchn_cb(dev->data);
}

static int xen_hvc_irq_tx_ready(const struct device *dev)
{
	return 1;
}

static void xen_hvc_irq_rx_enable(const struct device *dev)
{
	/*
	 * Need to explicitly call UART callback on RX enabling to
	 * process available buffered RX actions, because no HV events
	 * will be generated on rx_enable.
	 */
	hvc_uart_evtchn_cb(dev->data);
}

static int xen_hvc_irq_tx_complete(const struct device *dev)
{
	/*
	 * TX is performed by copying in ring buffer by fifo_fill,
	 * so it will be always completed.
	 */
	return 1;
}

static int xen_hvc_irq_rx_ready(const struct device *dev)
{
	struct hvc_xen_data *data = dev->data;

	/* RX is ready only if data is available in ring buffer */
	return (data->intf->in_prod != data->intf->in_cons);
}

static int xen_hvc_irq_is_pending(const struct device *dev)
{
	return xen_hvc_irq_rx_ready(dev);
}

static int xen_hvc_irq_update(const struct device *dev)
{
	/* Nothing needs to be updated before actual ISR */
	return 1;
}

static void xen_hvc_irq_callback_set(const struct device *dev,
		 uart_irq_callback_user_data_t cb, void *user_data)
{
	struct hvc_xen_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = user_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, xen_hvc_api) = {
	.poll_in = xen_hvc_poll_in,
	.poll_out = xen_hvc_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = xen_hvc_fifo_fill,
	.fifo_read = xen_hvc_fifo_read,
	.irq_tx_enable = xen_hvc_irq_tx_enable,
	.irq_tx_ready = xen_hvc_irq_tx_ready,
	.irq_rx_enable = xen_hvc_irq_rx_enable,
	.irq_tx_complete = xen_hvc_irq_tx_complete,
	.irq_rx_ready = xen_hvc_irq_rx_ready,
	.irq_is_pending = xen_hvc_irq_is_pending,
	.irq_update = xen_hvc_irq_update,
	.irq_callback_set = xen_hvc_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void hvc_uart_evtchn_cb(void *priv)
{
	struct hvc_xen_data *data = priv;

	if (data->irq_cb) {
		data->irq_cb(data->dev, data->irq_cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

int xen_console_init(const struct device *dev)
{
	int ret = 0;
	uint64_t console_pfn = 0;
	uintptr_t console_addr = 0;
	struct hvc_xen_data *data = dev->data;

	data->dev = dev;

	ret = hvm_get_parameter(HVM_PARAM_CONSOLE_EVTCHN, DOMID_SELF, &data->evtchn);
	if (ret) {
		LOG_ERR("%s: failed to get Xen console evtchn, ret = %d\n",
				__func__, ret);
		return ret;
	}

	ret = hvm_get_parameter(HVM_PARAM_CONSOLE_PFN, DOMID_SELF, &console_pfn);
	if (ret) {
		LOG_ERR("%s: failed to get Xen console PFN, ret = %d\n",
				__func__, ret);
		return ret;
	}

	console_addr = (uintptr_t) (console_pfn << XEN_PAGE_SHIFT);
	device_map(DEVICE_MMIO_RAM_PTR(dev), console_addr, XEN_PAGE_SIZE,
		K_MEM_CACHE_WB);

	data->intf = (struct xencons_interface *) DEVICE_MMIO_GET(dev);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	bind_event_channel(data->evtchn, hvc_uart_evtchn_cb, data);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	LOG_INF("Xen HVC inited successfully\n");

	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(xen_hvc), xen_console_init, NULL, &xen_hvc_data,
		NULL, PRE_KERNEL_1, CONFIG_XEN_HVC_INIT_PRIORITY,
		&xen_hvc_api);

#ifdef CONFIG_XEN_EARLY_CONSOLEIO
int xen_consoleio_putc(int c)
{
	char symbol = (char) c;

	HYPERVISOR_console_io(CONSOLEIO_write, sizeof(symbol), &symbol);
	return c;
}



int consoleio_hooks_set(void)
{

	/* Will be replaced with poll_in/poll_out by uart_console.c later on boot */
	__stdout_hook_install(xen_consoleio_putc);
	__printk_hook_install(xen_consoleio_putc);

	return 0;
}

SYS_INIT(consoleio_hooks_set, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_XEN_EARLY_CONSOLEIO */
