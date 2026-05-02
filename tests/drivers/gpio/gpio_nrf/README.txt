Nordic Semiconductor specific GPIO functions
############################################
The suite specified here tests NRF specific
GPIO pin drive strength settings.

The suite is perfomed with CONFIG_GPIO_NRFX_INTERRUPT=n
to ensure that the driver is able to properly handle
the GPIO manipulation without interrupt support.
