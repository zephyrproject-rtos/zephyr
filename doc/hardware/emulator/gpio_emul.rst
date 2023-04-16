.. _gpio_emul_api:


GPIO Emulator
###################################

Overview
********
The General-Purpose Input/Output (GPIO) emulator provides the ability to
programmatically set the value reported by the GPIO driver to assist in
testing without access to the real hardware. This includes triggering
interrupts tied to a GIPO pin or port.


Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_GPIO`
* :kconfig:option:`CONFIG_GPIO_EMUL`
* :kconfig:option:`CONFIG_GPIO_EMUL_SDL`

API Reference
*************

.. doxygengroup:: gpio_emul_interface
