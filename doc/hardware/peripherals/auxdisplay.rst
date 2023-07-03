.. _auxdisplay_api:

Auxiliary Display (auxdisplay)
##############################

Overview
********

Auxiliary Displays are text-based displays that have simple interfaces for
displaying textual, numeric or alphanumeric data, as opposed to the
:ref:`display_api`, auxiliary displays do not support custom
graphical output to displays (and and most often monochrome), the most
advanced custom feature supported is generation of custom characters.
These inexpensive displays are commonly found with various configurations
and sizes, a common display size is 16 characters by 2 lines.

This API is unstable and subject to change.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_AUXDISPLAY`
* :kconfig:option:`CONFIG_AUXDISPLAY_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: auxdisplay_interface
