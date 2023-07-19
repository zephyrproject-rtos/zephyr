.. _usb_hid_common:

Human Interface Devices (HID)
#############################

Common USB HID part that can be used outside of USB support, defined in
header file :zephyr_file:`include/zephyr/usb/class/hid.h`.

HID types reference
*******************

.. doxygengroup:: usb_hid_definitions

HID items reference
*******************

.. doxygengroup:: usb_hid_items

HID Mouse and Keyboard report descriptors
*****************************************

The pre-defined Mouse and Keyboard report descriptors can be used by
a HID device implementation or simply as examples.

.. doxygengroup:: usb_hid_mk_report_desc
