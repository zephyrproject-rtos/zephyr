.. _usb_guidelines:

Common guidelines for USB tests and samples
###########################################

Overview
********

In general, all USB samples and tests are platform agnostic and should not require a
platform-specific overlay. Though there may be exceptions to that already in the tree, the
goal is to avoid platform-specific overlays. There is no obligation for a USB sample or
USB test to support a specific platform.

Board configuration
*******************

Default USB device and host controller
======================================

USB support uses the ``zephyr_udc0`` node label to assign the default USB device controller
and the ``zephyr_uhc0`` node label to assign the default USB host controller. Depending on
what a board supports, it has to assign these node labels to be able to execute samples and
tests in the tree.

Supported features for the board metadata
=========================================

There are only two supported features for the board metadata file that are used by
Twister to pick up a sample or a test. For a board with a USB controller supporting device
mode, the feature is ``usbd``. For a board with a USB controller supporting host mode, the
feature is ``usbh``.

See :ref:`twister_board_configuration` for more details.

Deprecated features
-------------------

The feature ``usb_device`` should not be used anymore. This feature belongs to the legacy
USB device stack, which is deprecated and will be removed.

Tests Twister configuration
***************************

USB tests may require additional hardware or software setup. These tests should use
:ref:`fixtures <twister_fixtures>` described in the table below.

+-----------------------------+-------------------------------------------------------------+
| Fixture                     | Use case                                                    |
+=============================+=============================================================+
| ``usb_host_connected``      | The test implements USB device functionality, and requires  |
|                             | a USB host to be connected to the board under test.         |
+-----------------------------+-------------------------------------------------------------+
| ``usb_device_connected``    | The test implements USB host functionality, and requires    |
|                             | a USB device to be connected to the board under test.       |
+-----------------------------+-------------------------------------------------------------+
