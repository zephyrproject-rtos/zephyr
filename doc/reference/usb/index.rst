.. _usb_api:

USB device stack
################

.. contents::
   :depth: 2
   :local:
   :backlinks: top

USB Vendor and Product identifiers
**********************************

The USB Vendor ID for the Zephyr project is 0x2FE3. The default USB Product
ID for the Zephyr project is 0x100. The USB bcdDevice Device Release Number
represents the Zephyr kernel major and minor versions as a binary coded
decimal value. When a vendor integrates the Zephyr USB subsystem into a
product, the vendor must use the USB Vendor and Product ID assigned to them.
A vendor integrating the Zephyr USB subsystem in a product must not use the
Vendor ID of the Zephyr project.

The USB maintainer, if one is assigned, or otherwise the Zephyr Technical
Steering Committee, may allocate other USB Product IDs based on well-motivated
and documented requests.

Each USB sample has its own unique Product ID.
When adding a new sample, add a new entry in :file:`samples/subsys/usb/usb_pid.Kconfig`
and a Kconfig file inside your sample subdirectory.
The following Product IDs are currently used:

* :option:`CONFIG_USB_PID_CDC_ACM_SAMPLE`
* :option:`CONFIG_USB_PID_CDC_ACM_COMPOSITE_SAMPLE`
* :option:`CONFIG_USB_PID_HID_CDC_SAMPLE`
* :option:`CONFIG_USB_PID_CONSOLE_SAMPLE`
* :option:`CONFIG_USB_PID_DFU_SAMPLE`
* :option:`CONFIG_USB_PID_HID_SAMPLE`
* :option:`CONFIG_USB_PID_HID_MOUSE_SAMPLE`
* :option:`CONFIG_USB_PID_MASS_SAMPLE`
* :option:`CONFIG_USB_PID_TESTUSB_SAMPLE`
* :option:`CONFIG_USB_PID_WEBUSB_SAMPLE`
* :option:`CONFIG_USB_PID_BLE_HCI_H4_SAMPLE`

USB device controller drivers
*****************************

The Device Controller Driver Layer implements the low level control routines
to deal directly with the hardware. All device controller drivers should
implement the APIs described in file usb_dc.h. This allows the integration of
new USB device controllers to be done without changing the upper layers.

USB Device Controller API
=========================

.. doxygengroup:: _usb_device_controller_api
   :project: Zephyr

USB device core layer
*********************

The USB Device core layer is a hardware independent interface between USB
device controller driver and USB device class drivers or customer applications.
It's a port of the LPCUSB device stack. It provides the following
functionalities:

* Responds to standard device requests and returns standard descriptors,
  essentially handling 'Chapter 9' processing, specifically the standard
  device requests in table 9-3 from the universal serial bus specification
  revision 2.0.
* Provides a programming interface to be used by USB device classes or
  customer applications. The APIs are described in the usb_device.h file.
* Uses the APIs provided by the device controller drivers to interact with
  the USB device controller.


USB Device Core Layer API
=========================

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
   :project: Zephyr

USB device class drivers
************************

Zephyr USB Device Stack supports many standard classes, such as HID, MSC
Ethernet over USB, DFU, Bluetooth.

Implementing non standard USB class
===================================

Configuration of USB Device is done in the stack layer.

The following structures and callbacks need to be defined:

* Part of USB Descriptor table
* USB Endpoint configuration table
* USB Device configuration structure
* Endpoint callbacks
* Optionally class, vendor and custom handlers

For example, for USB loopback application:

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

Testing USB over USP/IP in native_posix
***************************************

Virtual USB controller implemented through USB/IP might be used to test USB
Device stack. Follow general build procedure to build USB sample for
the native_posix configuration.

Run built sample with:

.. code-block:: console

   west build -t run

In a terminal window, run the following command to list USB devices:

.. code-block:: console

   $ usbip list -r localhost
   Exportable USB devices
   ======================
    - 127.0.0.1
           1-1: unknown vendor : unknown product (2fe3:0100)
              : /sys/devices/pci0000:00/0000:00:01.2/usb1/1-1
              : (Defined at Interface level) (00/00/00)
              :  0 - Vendor Specific Class / unknown subclass / unknown protocol (ff/00/00)

In a terminal window, run the following command to attach USB device:

.. code-block:: console

   $ sudo usbip attach -r localhost -b 1-1

The USB device should be connected to your Linux host, and verified with the following commands:

.. code-block:: console

   $ sudo usbip port
   Imported USB devices
   ====================
   Port 00: <Port in Use> at Full Speed(12Mbps)
          unknown vendor : unknown product (2fe3:0100)
          7-1 -> usbip://localhost:3240/1-1
              -> remote bus/dev 001/002
   $ lsusb -d 2fe3:0100
   Bus 007 Device 004: ID 2fe3:0100

USB Human Interface Devices (HID) support
*****************************************

HID Item helpers
================

HID item helper macros can be used to compose a HID Report Descriptor.
The names correspond to those used in the USB HID Specification.

Example of a HID Report Descriptor:

.. code-block:: c

    static const uint8_t hid_report_desc[] = {
        HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
        HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
        HID_COLLECTION(HID_COLLECTION_APPLICATION),
        HID_LOGICAL_MIN8(0),
        /* logical maximum 255 */
        HID_LOGICAL_MAX16(0xFF, 0x00),
        HID_REPORT_ID(1),
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(1),
        HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
        /* HID_INPUT (Data, Variable, Absolute)	*/
        HID_INPUT(0x02),
        HID_END_COLLECTION,
    };


.. doxygengroup:: usb_hid_items
   :project: Zephyr

.. doxygengroup:: usb_hid_types
   :project: Zephyr

HID Mouse and Keyboard report descriptors
=========================================

The pre-defined Mouse and Keyboard report descriptors can be used by
a HID device implementation or simply as examples.

.. doxygengroup:: usb_hid_mk_report_desc
   :project: Zephyr

HID Class Device API
********************

USB HID devices like mouse, keyboard, or any other specific device use this API.

.. doxygengroup:: usb_hid_device_api
   :project: Zephyr
