.. _flash_emul_api:


Flash Simulator
###################################

Overview
********
The Flash simulator provides the ability to simulate flash storage by creating
a mock software flash that can be queried to assist in testing without access
to the real hardware.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_FLASH`
* :kconfig:option:`CONFIG_FLASH_SIMULATOR`

API Reference
*************

.. doxygengroup:: flash_emul_interface
