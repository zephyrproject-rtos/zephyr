.. _kscan_api:


KSCAN
#####

Overview
********
The kscan driver (keyboard scan matrix) is used for detecting a key press in a
connected matrix keyboard or any device with buttons such as joysticks.
Typically, matrix keyboards are implemented using a two-dimensional
configuration in order to sense several keys.  This allows interfacing to
many keys through fewer physical pins. Keyboard matrix
drivers read the rows while applying power through the columns one at a time
with the purpose of detecting key events.
There is no correlation between the physical and electrical layout of keys.
For, example, the physical layout may be one array of 16 or fewer keys, which
may be electrically connected to a 4 x 4 array. In addition, key values are
defined by a keymap provided by the keyboard manufacturer.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_KSCAN`

API Reference
*************

.. doxygengroup:: kscan_interface
