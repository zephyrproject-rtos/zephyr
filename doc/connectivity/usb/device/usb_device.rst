.. _usb_device_stack:

USB device support
##################

.. contents::
    :local:
    :depth: 3

Overview
********

The USB device stack is a hardware independent interface between USB
device controller driver and USB device class drivers or customer applications.
It is a port of the LPCUSB device stack and has been modified and expanded
over time. It provides the following functionalities:

* Uses the :ref:`usb_dc_api` provided by the device controller drivers to interact with
  the USB device controller.
* Responds to standard device requests and returns standard descriptors,
  essentially handling 'Chapter 9' processing, specifically the standard
  device requests in table 9-3 from the universal serial bus specification
  revision 2.0.
* Provides a programming interface to be used by USB device classes or
  customer applications. The APIs is described in
  :zephyr_file:`include/zephyr/usb/usb_device.h`

The device stack and :ref:`usb_dc_api` have some limitations, such as not being
able to support more than one controller instance at runtime and only supporting
one USB device configuration. We are actively working on new USB support, which
means we will continue to maintain the device stack described here until all
supported USB classes are ported, but do not expect any new features or enhancements.

Supported USB classes
*********************

Audio
=====

There is an experimental implementation of the Audio class. It follows specification
version 1.00 (``bcdADC 0x0100``) and supports synchronous synchronisation type only.
See :zephyr:code-sample:`usb-audio-headphones-microphone` and
:zephyr:code-sample:`usb-audio-headset` samples for reference.

Bluetooth HCI USB transport layer
=================================

Bluetooth HCI USB transport layer implementation uses :ref:`bt_hci_raw`
to expose HCI interface to the host. It is not fully in line with the description
in the Bluetooth specification and consists only of an interface with the endpoint
configuration:

* HCI commands through control endpoint (host-to-device only)
* HCI events through interrupt IN endpoint
* ACL data through one bulk IN and one bulk OUT endpoints

A second interface for the voice channels has not been implemented as there is
no support for this type in :ref:`bluetooth`. It is not a big problem under Linux
if HCI USB transport layer is the only interface that appears in the configuration,
the btusb driver would not try to claim a second (isochronous) interface.
The consequence is that if HCI USB is used in a composite configuration and is
the first interface, then the Linux btusb driver will claim both the first and
the next interface, preventing other composite functions from working.
Because of this problem, HCI USB should not be used in a composite configuration.
This problem is fixed in the implementation for new USB support.

See :ref:`bluetooth-hci-usb-sample` sample for reference.

.. _usb_device_cdc_acm:

CDC ACM
=======

The CDC ACM class is used as backend for different subsystems in Zephyr.
However, its configuration may not be easy for the inexperienced user.
Below is a description of the different use cases and some pitfalls.

The interface for CDC ACM user is :ref:`uart_api` driver API.
But there are two important differences in behavior to a real UART controller:

* Data transfer is only possible after the USB device stack has been
  initialized and started, until then any data is discarded
* If device is connected to the host, it still needs an application
  on the host side which requests the data
* The CDC ACM poll out implementation follows the API and blocks when the TX
  ring buffer is full only if the hw-flow-control property is enabled and
  called from a non-ISR context.

The devicetree compatible property for CDC ACM UART is
:dtcompatible:`zephyr,cdc-acm-uart`.
CDC ACM support is automatically selected when USB device support is enabled
and a compatible node in the devicetree sources is present. If necessary,
CDC ACM support can be explicitly disabled by :kconfig:option:`CONFIG_USB_CDC_ACM`.
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

Samples :zephyr:code-sample:`usb-cdc-acm` and :zephyr:code-sample:`usb-hid-cdc` have similar overlay files.
And since no special properties are present, it may seem overkill to use
devicetree to describe CDC ACM UART.  The motivation behind using devicetree
is the easy interchangeability of a real UART controller and CDC ACM UART
in applications.

Console over CDC ACM UART
-------------------------

With the CDC ACM UART node from above and ``zephyr,console`` property of the
chosen node, we can describe that CDC ACM UART is to be used with the console.
A similar overlay file is used by the :zephyr:code-sample:`usb-cdc-acm-console` sample.

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

	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
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
-----------------------

As for the console sample, it is possible to configure CDC ACM UART as
backend for other subsystems by setting :ref:`devicetree-chosen-nodes`
properties.

List of few Zephyr specific chosen properties which can be used to select
CDC ACM UART as backend for a subsystem or application:

* ``zephyr,bt-c2h-uart`` used in Bluetooth,
  for example see :ref:`bluetooth-hci-uart-sample`
* ``zephyr,ot-uart`` used in OpenThread,
  for example see :zephyr:code-sample:`coprocessor`
* ``zephyr,shell-uart`` used by shell for serial backend,
  for example see :zephyr_file:`samples/subsys/shell/shell_module`
* ``zephyr,uart-mcumgr`` used by :zephyr:code-sample:`smp-svr` sample

POSIX default tty ECHO mitigation
---------------------------------

POSIX systems, like Linux, default to enabling ECHO on tty devices. Host side
application can disable ECHO by calling ``open()`` on the tty device and issuing
``ioctl()`` (preferably via ``tcsetattr()``) to disable echo if it is not desired.
Unfortunately, there is an inherent race between the ``open()`` and ``ioctl()``
where the ECHO is enabled and any characters received (even if host application
does not call ``read()``) will be echoed back. This issue is especially visible
when the CDC ACM port is used without any real UART on the other side because
there is no arbitrary delay due to baud rate.

To mitigate the issue, Zephyr CDC ACM implementation arms IN endpoint with ZLP
after device is configured. When the host reads the ZLP, which is pretty much
the best indication that host application has opened the tty device, Zephyr will
force :kconfig:option:`CONFIG_CDC_ACM_TX_DELAY_MS` millisecond delay before real
payload is sent. This should allow sufficient time for first, and only first,
application that opens the tty device to disable ECHO if ECHO is not desired.
If ECHO is not desired at all from CDC ACM device it is best to set up udev rule
to disable ECHO as soon as device is connected.

ECHO is particurarly unwanted when CDC ACM instance is used for Zephyr shell,
because the control characters to set color sent back to shell are interpreted
as (invalid) command and user will see garbage as a result. While minicom does
disable ECHO by default, on exit with reset it will restore the termios settings
to whatever was set on entry. Therefore, if minicom is the first application to
open the tty device, the exit with reset will enable ECHO back and thus set up
a problem for the next application (which cannot be mitigated at Zephyr side).
To prevent the issue it is recommended either to leave minicom without reset or
to disable ECHO before minicom is started.

DFU
===

USB DFU class implementation is tightly coupled to :ref:`dfu` and :ref:`mcuboot_api`.
This means that the target platform must support the :ref:`flash_img_api` API.

See :zephyr:code-sample:`usb-dfu` sample for reference.

USB Human Interface Devices (HID) support
=========================================

HID support abuses :ref:`device_model_api` simply to allow applications to use
the :c:func:`device_get_binding`. Note that there is no HID device API as such,
instead the interface is provided by :c:struct:`hid_ops`.
The default instance name is ``HID_n``, where n can be {0, 1, 2, ...} depending on
the :kconfig:option:`CONFIG_USB_HID_DEVICE_COUNT`.

Each HID instance requires a HID report descriptor. The interface to the core
and the report descriptor must be registered using :c:func:`usb_hid_register_device`.

As the USB HID specification is not only used by the USB subsystem, the USB HID API
reference is split into two parts, :ref:`usb_hid_common` and :ref:`usb_hid_device`.
HID helper macros from :ref:`usb_hid_common` should be used to compose a
HID report descriptor. Macro names correspond to those used in the USB HID specification.

For the HID class interface, an IN interrupt endpoint is required for each instance,
an OUT interrupt endpoint is optional. Thus, the minimum implementation requirement
for :c:struct:`hid_ops` is to provide ``int_in_ready`` callback.

.. code-block:: c

	#define REPORT_ID		1
	static bool configured;
	static const struct device *hdev;

	static void int_in_ready_cb(const struct device *dev)
	{
		static uint8_t report[2] = {REPORT_ID, 0};

		if (hid_int_ep_write(hdev, report, sizeof(report), NULL)) {
			LOG_ERR("Failed to submit report");
		} else {
			report[1]++;
		}
	}

	static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
	{
		if (status == USB_DC_RESET) {
			configured = false;
		}

		if (status == USB_DC_CONFIGURED && !configured) {
			int_in_ready_cb(hdev);
			configured = true;
		}
	}

	static const uint8_t hid_report_desc[] = {
		HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
		HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
		HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_LOGICAL_MIN8(0x00),
		HID_LOGICAL_MAX16(0xFF, 0x00),
		HID_REPORT_ID(REPORT_ID),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(1),
		HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
		HID_INPUT(0x02),
		HID_END_COLLECTION,
	};

	static const struct hid_ops my_ops = {
		.int_in_ready = int_in_ready_cb,
	};

	int main(void)
	{
		int ret;

		hdev = device_get_binding("HID_0");
		if (hdev == NULL) {
			return -ENODEV;
		}

		usb_hid_register_device(hdev, hid_report_desc, sizeof(hid_report_desc),
					&my_ops);

		ret = usb_hid_init(hdev);
		if (ret) {
			return ret;
		}

		return usb_enable(status_cb);
	}


If the application wishes to receive output reports via the OUT interrupt endpoint,
it must enable :kconfig:option:`CONFIG_ENABLE_HID_INT_OUT_EP` and provide
``int_out_ready`` callback.
The disadvantage of this is that Kconfig options such as
:kconfig:option:`CONFIG_ENABLE_HID_INT_OUT_EP` or
:kconfig:option:`CONFIG_HID_INTERRUPT_EP_MPS` apply to all instances. This design
issue will be fixed in the HID class implementation for the new USB support.

See :zephyr:code-sample:`usb-hid` or :zephyr:code-sample:`usb-hid-mouse` sample for reference.

Mass Storage Class
==================

MSC follows Bulk-Only Transport specification and uses :ref:`disk_access_api` to
access and expose a RAM disk, emulated block device on a flash partition,
or SD Card to the host. Only one disk instance can be exported at a time.

The disc to be used by the implementation is set by the
:kconfig:option:`CONFIG_MASS_STORAGE_DISK_NAME` and should be the same as the name
used by the disc access driver that the application wants to expose to the host.
SD card disk drivers use options :kconfig:option:`CONFIG_MMC_VOLUME_NAME` or
:kconfig:option:`CONFIG_SDMMC_VOLUME_NAME`, and flash and RAM disk drivers use
node property ``disk-name`` to set the disk name.

For the emulated block device on a flash partition, the flash partition and
flash disk to be used must be described in the devicetree. If a storage partition
is already described at the board level, application devicetree overlay must also
delete ``storage_partition`` node first. :kconfig:option:`CONFIG_MASS_STORAGE_DISK_NAME`
should be the same as ``disk-name`` property.

.. code-block:: devicetree

	/delete-node/ &storage_partition;

	&mx25r64 {
		partitions {
			compatible = "fixed-partitions";
			#address-cells = <1>;
			#size-cells = <1>;

			storage_partition: partition@0 {
				label = "storage";
				reg = <0x00000000 0x00020000>;
			};
		};
	};

	/ {
		msc_disk0 {
			compatible = "zephyr,flash-disk";
			partition = <&storage_partition>;
			disk-name = "NAND";
			cache-size = <4096>;
		};
	};

The ``disk-property`` "NAND" may be confusing, but it is simply how some file
systems identifies the disc. Therefore, if the application also accesses the
file system on the exposed disc, default names should be used, see
:zephyr:code-sample:`usb-mass` sample for reference.

Networking
==========

There are three implementations that work in a similar way, providing a virtual
Ethernet connection between the remote (USB host) and Zephyr network support.

* CDC ECM class, enabled with :kconfig:option:`CONFIG_USB_DEVICE_NETWORK_ECM`
* CDC EEM class, enabled with :kconfig:option:`CONFIG_USB_DEVICE_NETWORK_EEM`
* RNDIS support, enabled with :kconfig:option:`CONFIG_USB_DEVICE_NETWORK_RNDIS`

See :zephyr:code-sample:`zperf` or :zephyr:code-sample:`socket-dumb-http-server` for reference.
Typically, users will need to add a configuration file overlay to the build,
such as :zephyr_file:`samples/net/zperf/overlay-netusb.conf`.

Applications using RNDIS support should enable :kconfig:option:`CONFIG_USB_DEVICE_OS_DESC`
for a better user experience on a host running Microsoft Windows OS.

Binary Device Object Store (BOS) support
****************************************

BOS handling can be enabled with Kconfig option :kconfig:option:`CONFIG_USB_DEVICE_BOS`.
This option also has the effect of changing device descriptor ``bcdUSB`` to ``0210``.
The application should register descriptors such as Capability Descriptor
using :c:func:`usb_bos_register_cap`. Registered descriptors are added to the root
BOS descriptor and handled by the stack.

See :zephyr:code-sample:`webusb` sample for reference.

Implementing a non-standard USB class
*************************************

The configuration of USB device is done in the stack layer.

The following structures and callbacks need to be defined:

* Part of USB Descriptor table
* USB Endpoint configuration table
* USB Device configuration structure
* Endpoint callbacks
* Optionally class, vendor and custom handlers

For example, for the USB loopback application:

.. literalinclude:: ../../../../subsys/usb/device/class/loopback.c
   :language: c
   :start-after: usb.rst config structure start
   :end-before: usb.rst config structure end
   :linenos:

Endpoint configuration:

.. literalinclude:: ../../../../subsys/usb/device/class/loopback.c
   :language: c
   :start-after: usb.rst endpoint configuration start
   :end-before: usb.rst endpoint configuration end
   :linenos:

USB Device configuration structure:

.. literalinclude:: ../../../../subsys/usb/device/class/loopback.c
   :language: c
   :start-after: usb.rst device config data start
   :end-before: usb.rst device config data end
   :linenos:


The vendor device requests are forwarded by the USB stack core driver to the
class driver through the registered vendor handler.

For the loopback class driver, :c:func:`loopback_vendor_handler` processes
the vendor requests:

.. literalinclude:: ../../../../subsys/usb/device/class/loopback.c
   :language: c
   :start-after: usb.rst vendor handler start
   :end-before:  usb.rst vendor handler end
   :linenos:

The class driver waits for the :makevar:`USB_DC_CONFIGURED` device status code
before transmitting any data.

.. _testing_USB_native_sim:

Interface number and endpoint address assignment
************************************************

In USB terminology, a ``function`` is a device that provides a capability to the
host, such as a HID class device that implements a keyboard. A function
constains a collection of ``interfaces``; at least one interface is required. An
interface may contain device ``endpoints``; for example, at least one input
endpoint is required to implement a HID class device, and no endpoints are
required to implement a USB DFU class. A USB device that combines functions is
a multifunction USB device, for example, a combination of a HID class device
and a CDC ACM device.

With Zephyr RTOS USB support, various combinations are possible with built-in USB
classes/functions or custom user implementations. The limitation is the number
of available device endpoints. Each device endpoint is uniquely addressable.
The endpoint address is a combination of endpoint direction and endpoint
number, a four-bit value. Endpoint number zero is used for the default control
method to initialize and configure a USB device. By specification, a maximum of
``15 IN`` and ``15 OUT`` device endpoints are also available for use in functions.
The actual number depends on the device controller used. Not all controllers
support the maximum number of endpoints and all endpoint types. For example, a
device controller might support one IN and one OUT isochronous endpoint, but
only for endpoint number 8, resulting in endpoint addresses 0x88 and 0x08.
Also, one controller may be able to have IN/OUT endpoints on the same endpoint
number, interrupt IN endpoint 0x81 and bulk OUT endpoint 0x01, while the other
may only be able to handle one endpoint per endpoint number. Information about
the number of interfaces, interface associations, endpoint types, and addresses
is provided to the host by the interface, interface specifiec, and endpoint
descriptors.

Host driver for specific function, uses interface and endpoint descriptor to
obtain endpoint addresses, types, and other properties. This allows function
host drivers to be generic, for example, a multi-function device consisting of
one or more CDC ACM and one or more CDC ECM class implementations is possible
and no specific drivers are required.

Interface and endpoint descriptors of built-in USB class/function
implementations in Zephyr RTOS typically have default interface numbers and
endpoint addresses assigned in ascending order. During initialization,
default interface numbers may be reassigned based on the number of interfaces in
a given configuration. Endpoint addresses are reassigned based on controller
capabilities, since certain endpoint combinations are not possible with every
controller, and the number of interfaces in a given configuration. This also
means that the device side class/function in the Zephyr RTOS must check the
actual interface and endpoint descriptor values at runtime.
This mechanism also allows as to provide generic samples and generic
multifunction samples that are limited only by the resources provided by the
controller, such as the number of endpoints and the size of the endpoint FIFOs.

There may be host drivers for a specific function, for example in the Linux
Kernel, where the function driver does not read interface and endpoint
descriptors to check interface numbers or endpoint addresses, but instead uses
hardcoded values. Therefore, the host driver cannot be used in a generic way,
meaning it cannot be used with different device controllers and different
device configurations in combination with other functions. This may also be
because the driver is designed for a specific hardware and is not intended to
be used with a clone of this specific hardware. On the contrary, if the driver
is generic in nature and should work with different hardware variants, then it
must not use hardcoded interface numbers and endpoint addresses.
It is not possible to disable endpoint reassignment in Zephyr RTOS, which may
prevent you from implementing a hardware-clone firmware. Instead, if possible,
the host driver implementation should be fixed to use values from the interface
and endpoint descriptor.

Testing over USPIP in native_sim
********************************

A virtual USB controller implemented through USBIP might be used to test the USB
device stack. Follow the general build procedure to build the USB sample for
the :ref:`native_sim <native_sim>` configuration.

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

In a terminal window, run the following command to attach the USB device:

.. code-block:: console

   $ sudo usbip attach -r localhost -b 1-1

The USB device should be connected to your Linux host, and verified with the
following commands:

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

USB Vendor and Product identifiers
**********************************

The USB Vendor ID for the Zephyr project is ``0x2FE3``.
This USB Vendor ID must not be used when a vendor
integrates Zephyr USB device support into its own product.

Each USB :ref:`sample<usb-samples>` has its own unique Product ID.
The USB maintainer, if one is assigned, or otherwise the Zephyr Technical
Steering Committee, may allocate other USB Product IDs based on well-motivated
and documented requests.

The following Product IDs are currently used:

+----------------------------------------------------+--------+
| Sample                                             | PID    |
+====================================================+========+
| :zephyr:code-sample:`usb-cdc-acm`                  | 0x0001 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-cdc-acm-composite`        | 0x0002 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-hid-cdc`                  | 0x0003 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-cdc-acm-console`          | 0x0004 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-dfu` (Run-Time)           | 0x0005 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-hid`                      | 0x0006 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-hid-mouse`                | 0x0007 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-mass`                     | 0x0008 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`testusb-app`                  | 0x0009 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`webusb`                       | 0x000A |
+----------------------------------------------------+--------+
| :ref:`bluetooth-hci-usb-sample`                    | 0x000B |
+----------------------------------------------------+--------+
| :ref:`bluetooth-hci-usb-h4-sample`                 | 0x000C |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`wpan-usb`                     | 0x000D |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`uac2-explicit-feedback`       | 0x000E |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-dfu` (DFU Mode)           | 0xFFFF |
+----------------------------------------------------+--------+

The USB device descriptor field ``bcdDevice`` (Device Release Number) represents
the Zephyr kernel major and minor versions as a binary coded decimal value.
