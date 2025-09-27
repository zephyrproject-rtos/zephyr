.. _arduino_core:

ArduinoCore: universal snippet for ArduinoCore
##############################################

Overview
********

This snippet will enable the necessary settings for ArduinoCore.

Programming
***********

Correct snippet designation for ArduinoCore must
be entered when you invoke ``west build``.
For example:

.. code-block:: console

   west build -b arduino_nano_33_ble -S arduino_core <sample directory>
