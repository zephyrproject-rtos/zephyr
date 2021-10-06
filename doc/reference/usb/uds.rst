.. _usb_device_stack:

USB device stack
################

The USB device stack is a hardware independent interface between USB
device controller driver and USB device class drivers or customer applications.
It is a port of the LPCUSB device stack and has been modified and expanded
over time. It provides the following functionalities:

* Uses the APIs provided by the device controller drivers to interact with
  the USB device controller.
* Responds to standard device requests and returns standard descriptors,
  essentially handling 'Chapter 9' processing, specifically the standard
  device requests in table 9-3 from the universal serial bus specification
  revision 2.0.
* Provides a programming interface to be used by USB device classes or
  customer applications. The APIs is described in
  :zephyr_file:`include/usb/usb_device.h`

The device stack has few limitations with which it is not possible to support
more than one controller instance at runtime, and only one USB device
configuration is supported.

Supported USB classes:

* USB Audio (experimental)
* USB CDC ACM
* USB CDC ECM
* USB CDC EEM
* RNDIS
* USB MSC
* USB DFU
* Bluetooth HCI over USB
* USB HID class

:ref:`List<usb-samples>` of samples for different purposes.
CDC ACM and HID samples have configuration overlays for composite configuration.

Implementing a non-standard USB class
*************************************

The configuration of USB Device is done in the stack layer.

The following structures and callbacks need to be defined:

* Part of USB Descriptor table
* USB Endpoint configuration table
* USB Device configuration structure
* Endpoint callbacks
* Optionally class, vendor and custom handlers

For example, for the USB loopback application:

.. literalinclude:: ../../../subsys/usb/class/loopback.c
   :language: c
   :start-after: usb.rst config structure start
   :end-before: usb.rst config structure end
   :linenos:

Endpoint configuration:

.. literalinclude:: ../../../subsys/usb/class/loopback.c
   :language: c
   :start-after: usb.rst endpoint configuration start
   :end-before: usb.rst endpoint configuration end
   :linenos:

USB Device configuration structure:

.. literalinclude:: ../../../subsys/usb/class/loopback.c
   :language: c
   :start-after: usb.rst device config data start
   :end-before: usb.rst device config data end
   :linenos:


The vendor device requests are forwarded by the USB stack core driver to the
class driver through the registered vendor handler.

For the loopback class driver, :c:func:`loopback_vendor_handler` processes
the vendor requests:

.. literalinclude:: ../../../subsys/usb/class/loopback.c
   :language: c
   :start-after: usb.rst vendor handler start
   :end-before:  usb.rst vendor handler end
   :linenos:

The class driver waits for the :makevar:`USB_DC_CONFIGURED` device status code
before transmitting any data.

.. _testing_USB_native_posix:

USB Vendor and Product identifiers
**********************************

The USB Vendor ID for the Zephyr project is ``0x2FE3``.
This USB Vendor ID must not be used when a vendor
integrates Zephyr USB device support into its own product.

Each USB sample has its own unique Product ID.
The USB maintainer, if one is assigned, or otherwise the Zephyr Technical
Steering Committee, may allocate other USB Product IDs based on well-motivated
and documented requests.

When adding a new sample, add a new entry in :file:`samples/subsys/usb/usb_pid.Kconfig`
and a Kconfig file inside your sample subdirectory.
The following Product IDs are currently used:

* :kconfig:`CONFIG_USB_PID_CDC_ACM_SAMPLE`
* :kconfig:`CONFIG_USB_PID_CDC_ACM_COMPOSITE_SAMPLE`
* :kconfig:`CONFIG_USB_PID_HID_CDC_SAMPLE`
* :kconfig:`CONFIG_USB_PID_CONSOLE_SAMPLE`
* :kconfig:`CONFIG_USB_PID_DFU_SAMPLE`
* :kconfig:`CONFIG_USB_PID_HID_SAMPLE`
* :kconfig:`CONFIG_USB_PID_HID_MOUSE_SAMPLE`
* :kconfig:`CONFIG_USB_PID_MASS_SAMPLE`
* :kconfig:`CONFIG_USB_PID_TESTUSB_SAMPLE`
* :kconfig:`CONFIG_USB_PID_WEBUSB_SAMPLE`
* :kconfig:`CONFIG_USB_PID_BLE_HCI_H4_SAMPLE`

The USB device descriptor field ``bcdDevice`` (Device Release Number) represents
the Zephyr kernel major and minor versions as a binary coded decimal value.

API reference
*************

There are two ways to transmit data, using the 'low' level read/write API or
the 'high' level transfer API.

Low level API
  To transmit data to the host, the class driver should call usb_write().
  Upon completion the registered endpoint callback will be called. Before
  sending another packet the class driver should wait for the completion of
  the previous write. When data is received, the registered endpoint callback
  is called. usb_read() should be used for retrieving the received data.
  For CDC ACM sample driver this happens via the OUT bulk endpoint handler
  (cdc_acm_bulk_out) mentioned in the endpoint array (cdc_acm_ep_data).

High level API
  The usb_transfer method can be used to transfer data to/from the host. The
  transfer API will automatically split the data transmission into one or more
  USB transaction(s), depending endpoint max packet size. The class driver does
  not have to implement endpoint callback and should set this callback to the
  generic usb_transfer_ep_callback.

.. doxygengroup:: _usb_device_core_api
