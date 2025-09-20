.. _snippet-hci-uart-native-sim:

Native Simulator support for hci_uart Snippet (hci-uart-native-sim)
###################################################################

.. code-block:: console

   west build -S hci-uart-native-sim [...]

Overview
********

This snippet allows to use hci_uart connected to the host computer
with the Native Simulator. It is useful for testing with a real
Bluetooth controller, such as a device running Zephyr.
