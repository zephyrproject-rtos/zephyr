.. _usb_device_cdc_acm:

USB device stack CDC ACM support
################################

The CDC ACM class is used as backend for different subsystems in Zephyr.
However, its configuration may not be easy for the inexperienced user.
Below is a description of the different use cases and some pitfalls.

The interface for CDC ACM user is :ref:`uart_api` driver API.
But there are two important differences in behavior to a real UART controller:

* Data transfer is only possible after the USB device stack has been initialized and started,
  until then any data is discarded
* If device is connected to the host, it still needs an application
  on the host side which requests the data

The devicetree compatible property for CDC ACM UART is
:dtcompatible:`zephyr,cdc-acm-uart`.
CDC ACM support is automatically selected when USB device support is enabled
and a compatible node in the devicetree sources is present. If necessary,
CDC ACM support can be explicitly disabled by :kconfig:`CONFIG_USB_CDC_ACM`.
About four CDC ACM UART instances can be defined and used,
limited by the maximum number of supported endpoints on the controller.

CDC ACM UART node is supposed to be child of a USB device controller node.
Since the designation of the controller nodes varies from vendor to vendor,
and our samples and application should be as generic as possible,
the default USB device controller is usually assigned an ``zephyr_udc0``
node label. Often, CDC ACM UART is described in a devicetree overlay file
and looks like this:

.. code-block:: devicetree

   &zephyr_udc0 {
   	cdc_acm_uart0: cdc_acm_uart0 {
   		compatible = "zephyr,cdc-acm-uart";
   		label = "CDC_ACM_0";
   	};
   };

Samples :ref:`usb_cdc-acm` and :ref:`usb_hid-cdc` have similar overlay files.
And since no special properties are present, it may seem overkill to use
devicetree to describe CDC ACM UART.  The motivation behind using devicetree
is the easy interchangeability of a real UART controller and CDC ACM UART
in applications.

Console over CDC ACM UART
*************************

With the CDC ACM UART node from above and ``zephyr,console`` property of the
chosen node, we can describe that CDC ACM UART is to be used with the console.
A similar overlay file is used by :ref:`cdc-acm-console`.

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

Before the application uses the console, it is recommended to wait for
the DTR signal:

.. code-block:: c

    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
    	return;
    }

    while (!dtr) {
    	uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    	k_sleep(K_MSEC(100));
    }

    printk("nuqneH\n");

CDC ACM UART as backend
***********************

As for the console sample, it is possible to configure CDC ACM UART as
backend for other subsystems by setting :ref:`devicetree-chosen-nodes`
properties.

List of few Zephyr specific chosen properties which can be used to select
CDC ACM UART as backend for a subsystem or application:

* ``zephyr,bt-c2h-uart`` used in Bluetooth,
  for example see :ref:`bluetooth-hci-uart-sample`
* ``zephyr,ot-uart`` used in OpenThread,
  for example see :ref:`coprocessor-sample`
* ``zephyr,shell-uart`` used by shell for serial backend,
  for example see :zephyr_file:`samples/subsys/shell/shell_module`
* ``zephyr,uart-mcumgr`` used by :ref:`smp_svr_sample`
