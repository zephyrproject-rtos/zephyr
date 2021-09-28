.. _usb_device_hid:

USB Human Interface Devices (HID) support
#########################################

Since the USB HID specification is not only used by the USB subsystem, the USB HID API
is split into two header files :zephyr_file:`include/usb/class/hid.h`
and :zephyr_file:`include/usb/class/usb_hid.h`. The second includes a specific
part for HID support in the USB device stack.

HID Item helpers
****************

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


HID items reference
*******************

.. doxygengroup:: usb_hid_items

HID types reference
*******************

.. doxygengroup:: usb_hid_types

HID Mouse and Keyboard report descriptors
*****************************************

The pre-defined Mouse and Keyboard report descriptors can be used by
a HID device implementation or simply as examples.

.. doxygengroup:: usb_hid_mk_report_desc

HID Class Device API reference
******************************

USB HID devices like mouse, keyboard, or any other specific device use this API.

.. doxygengroup:: usb_hid_device_api
