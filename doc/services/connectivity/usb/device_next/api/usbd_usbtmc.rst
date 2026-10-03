.. _usbd_usbtmc:

USBTMC device class API
#######################

The USB Test and Measurement Class (USBTMC) is used by laboratory
instruments, such as oscilloscopes, multimeters, and programmable power
supplies, to exchange device dependent messages with a host. USBTMC only
defines the transport, the payload is typically a command protocol like
IEEE 488.2 or SCPI, but interpretation of the messages is entirely left to
the application. The class implementation neither implements nor requires
SCPI.

The implementation supports the mandatory USBTMC 1.0 mechanisms, device
dependent messages in both directions, the INITIATE_ABORT_BULK_OUT,
CHECK_ABORT_BULK_OUT_STATUS, INITIATE_ABORT_BULK_IN,
CHECK_ABORT_BULK_IN_STATUS, INITIATE_CLEAR, CHECK_CLEAR_STATUS, and
GET_CAPABILITIES requests, and the optional INDICATOR_PULSE request. The
USB488 subclass, vendor specific messages, and Bulk-IN termination character
support are not implemented.

USBTMC devices are supported out of the box by common host software. On
Linux, the kernel ``usbtmc`` driver exposes a character device such as
:file:`/dev/usbtmc0`, and VISA implementations, such as NI-VISA, Keysight IO
Libraries, or PyVISA, access the device as a USB instrument resource. The
device side requires non-zero serial number, manufacturer, and product
string descriptors, hosts use them to build a unique instrument identifier.

A USBTMC instance is instantiated and configured using devicetree,

.. code-block:: devicetree

   &zephyr_udc0 {
       usbtmc0: usbtmc0 {
           compatible = "zephyr,usbtmc-device";
           label = "USBTMC instrument";
       };
   };

The application provides the instrument functionality and registers its
event handlers using :c:func:`usbd_usbtmc_register` before the USB device
support is initialized. Device dependent message data received from the host
is delivered in chunks by the ``msg_out`` handler, without requiring a
complete message to fit into the class buffers. Message data to be sent to
the host is submitted with :c:func:`usbd_usbtmc_msg_write`, the data is
copied to class buffers and sent when the host requests it, a message can be
submitted in multiple chunks. See the :zephyr:code-sample:`usbtmc` sample
for how the API is used.

API Reference
*************

USBTMC device specific API defined in
:zephyr_file:`include/zephyr/usb/class/usbd_usbtmc.h`.

.. doxygengroup:: usbd_usbtmc

Protocol definitions defined in
:zephyr_file:`include/zephyr/usb/class/usb_tmc.h`.

.. doxygengroup:: usb_tmc
