.. _usb_device_stack_next:

New USB device support
######################

Overview
********

USB device support consists of the USB device controller (UDC) drivers
, :ref:`udc_api`, and USB device stack, :ref:`usbd_api`.
The :ref:`udc_api` provides a generic and vendor independent interface to USB
device controllers, and although, there a is clear separation between these
layers, the purpose of :ref:`udc_api` is to serve new Zephyr's USB device stack
exclusively.

The new device stack supports multiple device controllers, meaning that if a
SoC has multiple controllers, they can be used simultaneously. Full and
high-speed device controllers are supported. It also provides support for
registering multiple function or class instances to a configuration at runtime,
or changing the configuration later. It has built-in support for several USB
classes and provides an API to implement custom USB functions.

The new USB device support is considered experimental and will replace
:ref:`usb_device_stack`.

Built-in functions
==================

The USB device stack has built-in USB functions. Some can be used directly in
the user application through a special API, such as HID or Audio class devices,
while others use a general Zephyr RTOS driver API, such as MSC and CDC class
implementations. The *Identification string* identifies a class or function
instance (`n`) and is used as an argument to the :c:func:`usbd_register_class`.

+-----------------------------------+-------------------------+-------------------------+
| Class or function                 | User API (if any)       | Identification string   |
+===================================+=========================+=========================+
| USB Audio 2 class                 | :ref:`uac2_device`      | :samp:`uac2_{n}`        |
+-----------------------------------+-------------------------+-------------------------+
| USB CDC ACM class                 | :ref:`uart_api`         | :samp:`cdc_acm_{n}`     |
+-----------------------------------+-------------------------+-------------------------+
| USB CDC ECM class                 | Ethernet device         | :samp:`cdc_ecm_{n}`     |
+-----------------------------------+-------------------------+-------------------------+
| USB Mass Storage Class (MSC)      | :ref:`usbd_msc_device`  | :samp:`msc_{n}`         |
+-----------------------------------+-------------------------+-------------------------+
| USB Human Interface Devices (HID) | :ref:`usbd_hid_device`  | :samp:`hid_{n}`         |
+-----------------------------------+-------------------------+-------------------------+
| Bluetooth HCI USB transport layer | :ref:`bt_hci_raw`       | :samp:`bt_hci_{n}`      |
+-----------------------------------+-------------------------+-------------------------+

Samples
=======

* :zephyr:code-sample:`usb-hid-keyboard`

* :zephyr:code-sample:`uac2-explicit-feedback`

* :zephyr:code-sample:`uac2-implicit-feedback`

Samples ported to new USB device support
----------------------------------------

To build a sample that supports both the old and new USB device stack, set the
configuration ``-DCONF_FILE=usbd_next_prj.conf`` either directly or via
``west``.

* :zephyr:code-sample:`bluetooth_hci_usb`

* :zephyr:code-sample:`usb-cdc-acm`

* :zephyr:code-sample:`usb-cdc-acm-console`

* :zephyr:code-sample:`usb-mass`

* :zephyr:code-sample:`usb-hid-mouse`

* :zephyr:code-sample:`zperf` To build the sample for the new device support,
  set the configuration overlay file
  ``-DDEXTRA_CONF_FILE=overlay-usbd_next_ecm.conf`` and devicetree overlay file
  ``-DDTC_OVERLAY_FILE="usbd_next_ecm.overlay`` either directly or via ``west``.

How to configure and enable USB device support
**********************************************

For the USB device support samples in the Zephyr project repository, we have a
common file for instantiation, configuration and initialization,
:zephyr_file:`samples/subsys/usb/common/sample_usbd_init.c`. The following code
snippets from this file are used as examples. USB Samples Kconfig options used
in the USB samples and prefixed with ``SAMPLE_USBD_`` have default values
specific to the Zephyr project and the scope is limited to the project samples.
In the examples below, you will need to replace these Kconfig options and other
defaults with values appropriate for your application or hardware.

The USB device stack requires a context structure to manage its properties and
runtime data. The preferred way to define a device context is to use the
:c:macro:`USBD_DEVICE_DEFINE` macro. This creates a static
:c:struct:`usbd_context` variable with a given name. Any number of contexts may
be instantiated. A USB controller device can be assigned to multiple contexts,
but only one context can be initialized and used at a time. Context properties
must not be directly accessed or manipulated by the application.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc device instantiation start
   :end-before: doc device instantiation end

Your USB device may have manufacturer, product, and serial number string
descriptors. To instantiate these string descriptors, the application should
use the appropriate :c:macro:`USBD_DESC_MANUFACTURER_DEFINE`,
:c:macro:`USBD_DESC_PRODUCT_DEFINE`, and
:c:macro:`USBD_DESC_SERIAL_NUMBER_DEFINE` macros. String descriptors also
require a single instantiation of the language descriptor using the
:c:macro:`USBD_DESC_LANG_DEFINE` macro.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc string instantiation start
   :end-before: doc string instantiation end

String descriptors must be added to the device context at runtime before
initializing the USB device with :c:func:`usbd_add_descriptor`.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc add string descriptor start
   :end-before: doc add string descriptor end

USB device requires at least one configuration instance per supported speed.
The application should use :c:macro:`USBD_CONFIGURATION_DEFINE` to instantiate
a configuration. Later, USB device functions are assigned to a configuration.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc configuration instantiation start
   :end-before: doc configuration instantiation end

Each configuration instance for a specific speed must be added to the device
context at runtime before the USB device is initialized using
:c:func:`usbd_add_configuration`. Note :c:enumerator:`USBD_SPEED_FS` and
:c:enumerator:`USBD_SPEED_HS`. The first full-speed or high-speed
configuration will get ``bConfigurationValue`` one, and then further upward.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc configuration register start
   :end-before: doc configuration register end


Although we have already done a lot, this USB device has no function. A device
can have multiple configurations with different set of functions at different
speeds. A function or class can be registered on a USB device before
it is initialized using :c:func:`usbd_register_class`. The desired
configuration is specified using :c:enumerator:`USBD_SPEED_FS` or
:c:enumerator:`USBD_SPEED_HS` and the configuration number.  For simple cases,
:c:func:`usbd_register_all_classes` can be used to register all available
instances.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc functions register start
   :end-before: doc functions register end

The last step in the preparation is to initialize the device with
:c:func:`usbd_init`. After this, the configuration of the device cannot be
changed. A device can be deinitialized with :c:func:`usbd_shutdown` and all
instances can be reused, but the previous steps must be repeated. So it is
possible to shutdown a device, register another type of configuration or
function, and initialize it again.  At the USB controller level,
:c:func:`usbd_init` does only what is necessary to detect VBUS changes. There
are controller types where the next step is only possible if a VBUS signal is
present.

A function or class implementation may require its own specific configuration
steps, which should be performed prior to initializing the USB device.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc device init start
   :end-before: doc device init end

The final step to enable the USB device is :c:func:`usbd_enable`, after that,
if the USB device is connected to a USB host controller, the host can start
enumerating the device. The application can disable the USB device using
:c:func:`usbd_disable`.

.. literalinclude:: ../../../../samples/subsys/usb/hid-keyboard/src/main.c
   :language: c
   :dedent:
   :start-after: doc device enable start
   :end-before: doc device enable end

USB Message notifications
=========================

The application can register a callback using :c:func:`usbd_msg_register_cb` to
receive message notification from the USB device support subsystem. The
messages are mostly about the common device state changes, and a few specific
types from the USB CDC ACM implementation.

.. literalinclude:: ../../../../samples/subsys/usb/common/sample_usbd_init.c
   :language: c
   :dedent:
   :start-after: doc device init-and-msg start
   :end-before: doc device init-and-msg end

The helper function :c:func:`usbd_msg_type_string()` can be used to convert
:c:enumerator:`usbd_msg_type` to a human readable form for logging.

If the controller supports VBUS state change detection, the battery-powered
application may want to enable the USB device only when it is connected to a
host. A generic application should use :c:func:`usbd_can_detect_vbus` to check
for this capability.

.. literalinclude:: ../../../../samples/subsys/usb/hid-keyboard/src/main.c
   :language: c
   :dedent:
   :start-after: doc device msg-cb start
   :end-before: doc device msg-cb end
