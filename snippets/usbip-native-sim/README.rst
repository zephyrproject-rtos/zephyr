.. _snippet-usbip-native-sim:

USB/IP on Native Simulator Snippet (usbip-native-sim)
#####################################################

.. code-block:: console

   west build -S usbip-native-sim [...]

Overview
********

This snippet enables USB/IP support on a native simulator. When building with
this snippet, you need to provide DTC overlays using the EXTRA_DTC_OVERLAY_FILE
argument.

This snippet is experimental, the behavior may change without notice.
