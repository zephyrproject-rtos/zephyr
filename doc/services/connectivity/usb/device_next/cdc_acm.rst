.. _usbd_cdc_acm:

USB device CDC ACM
##################

The CDC ACM function provided by the USB device stack only implements Abstract
Control Model Serial Emulation. Its sole purpose is to emulate serial lines, as
its name suggests. Most modern operating systems should provide support
for it out of the box.

The CDC ACM function is represented as a serial interface on both the host and
device sides, while the user or application interface is the :ref:`uart_api`
driver API. This allows applications that already use the UART API to use the
serial interface provided by the CDC ACM function without changing the code
responsible for data communication. Only additional configuration and USB
device stack initialization are required.

CDC ACM UART configuration
==========================

Like the real UART controller, the virtual CDC ACM UART is described in the
device tree. The devicetree compatible property for CDC ACM UART is
:dtcompatible:`zephyr,cdc-acm-uart`.

CDC ACM support is automatically selected when USB device support is enabled
and a compatible node in the devicetree sources is present. If necessary, CDC ACM
support can be explicitly disabled by :kconfig:option:`CONFIG_USBD_CDC_ACM_CLASS`.
The number of possible CDC ACM instances depends on the number of supported
endpoints on the USB device controller. Each CDC ACM instance requires three
endpoints: two bulk endpoints (one IN, one OUT) and one interrupt IN with a
MaxPacketSize of 16.
The CDC ACM node can use the ``label`` property to distinguish different interfaces
on the host side. Below is an example of the devicetree overlay file.

.. code-block:: devicetree

	&zephyr_udc0 {
		cdc_acm_uart0: cdc_acm_uart0 {
			compatible = "zephyr,cdc-acm-uart";
			label = "CDC_ACM_0";
		};
	};

Before the application uses CDC ACM UART, it may want to wait for the DTR
signal. Please refer to :zephyr:code-sample:`usb-cdc-acm` on how to implement
this functionality.

.. note::
  To communicate with the host, beside the UART configuration, the application
  must enable USB device stack, for that please refer to
  :ref:`usb_device_next_howto_configure` and read carefully the next chapter. For
  users and applications migrating from the legacy stack, this is the only part
  they need to adapt.

.. _cdc_acm_uart_as_serial_backend:

CDC ACM UART as serial backend
==============================

Using the example above and the ``zephyr,console`` chosen node property, you can
configure the CDC ACM UART as the console device.

.. code-block:: devicetree

	/ {
		chosen {
			zephyr,console = &cdc_acm_uart0;
		};
	};

	&zephyr_udc0 {
		cdc_acm_uart0: cdc_acm_uart0 {
			compatible = "zephyr,cdc-acm-uart";
			label = "CDC_ACM_0";
		};
	};

In the same way that the console in the above example is configured to use the
CDC ACM UART, ``zephyr,shell-uart`` chosen node property can be used to
configure the shell to use the CDC ACM UART as the serial backend. See sample
:zephyr:code-sample:`shell-module` and :ref:`chosen nodes documentation
<devicetree-chosen-nodes>`.

Since the use case as a serial backend is very common and no configuration is
necessary at runtime for the CDC ACM UART, the stack offers a helper that
performs the steps described in :ref:`usb_device_next_howto_configure`. The
helper is enabled by the :kconfig:option:`CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT`
and initializes the USB device stack with a single CDC ACM instance. Sample
:zephyr:code-sample:`usb-cdc-acm-console` demonstrates how to use it.

:kconfig:option:`CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT` should also be used
by the boards like :zephyr:board:`nrf52840dongle`, which do not have a debug
adapter but a USB device controller, and want to use CDC ACM UART as default
serial backend for logging and shell.
As the configuration would be identical for any board, there are common
:zephyr_file:`devicetree file <boards/common/usb/cdc_acm_serial.dtsi>` and
:zephyr_file:`Kconfig file <boards/common/usb/Kconfig.cdc_acm_serial.defconfig>`
that must be included in the board's devicetree and Kconfig.defconfig files.

Using CDC ACM UART in the application
=====================================

CDC ACM implements a virtual UART controller and provides Interrupt-driven UART
API and Polling UART API. The ASYNC API is not supported yet. If the
application wants to communicate over CDC ACM UART, the preferable way is to
use Interrupt-driven UART API. It is essential to understand API documentation,
nevertheless, some notes below.

Interrupt-driven UART API
-------------------------

Internally the CDC ACM UART implementation uses two ringbuffers. These take over the
function of the TX/RX FIFOs (TX/RX buffers) from the :ref:`uart_interrupt_api`.

As described in the :ref:`uart_interrupt_api`, the functions
:c:func:`uart_irq_update()`, :c:func:`uart_irq_is_pending`,
:c:func:`uart_irq_rx_ready()`, :c:func:`uart_irq_tx_ready()`
:c:func:`uart_fifo_read()`, and :c:func:`uart_fifo_fill()`
should be called from the interrupt handler, see
:c:func:`uart_irq_callback_user_data_set()`. To prevent undefined behavior,
the implementation of these functions checks in what context they are called
and fails if it is not an interrupt handler.

Also, as described in the UART API, :c:func:`uart_irq_is_pending`
:c:func:`uart_irq_rx_ready()`, and :c:func:`uart_irq_tx_ready()`
can only be called after :c:func:`uart_irq_update()`.

Simplified, application interrupt handler should look something like:

.. code-block:: c

	static void interrupt_handler(const struct device *dev, void *user_data)
	{
		while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
			if (uart_irq_rx_ready(dev)) {
				int len;
				int n;

				/* ... */
				n = uart_fifo_read(dev, buffer, len);
				/* ... */
			}

			if (uart_irq_tx_ready(dev)) {
				int len;
				int n;

				/* ... */
				n = uart_fifo_fill(dev, buffer, len);
			  /* ... */
			}
		}
	}

All these functions are not directly dependent on the status of the USB device.
Filling the TX FIFO does not mean that data is being sent to the host. And
successfully reading the RX FIFO does not mean that the device is still
connected to the host. If there is space in the TX FIFO, and the TX interrupt
is enabled, :c:func:`uart_irq_tx_ready()` will succeed. If there is data in the
RX FIFO, and the RX interrupt is enabled, :c:func:`uart_irq_rx_ready()` will
succeed. Function :c:func:`uart_irq_tx_complete()` is not implemented yet.

Polling UART API
----------------

The CDC ACM poll out implementation follows :ref:`uart_polling_api` and
blocks when the TX FIFO is full only if the hw-flow-control property is enabled
and called from a non-ISR context.
