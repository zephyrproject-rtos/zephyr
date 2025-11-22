.. _nxp_mx9_dsi_oled:

NXP MX9 DSI OLED Panel
#########################

Overview
********

The NXP MX9 DSI OLED Panel is a high-resolution OLED display panel
designed for use with NXP i.MX9 series processors. This panel provides
excellent color reproduction and contrast ratio through OLED technology.
The display connects via MIPI DSI interface and offers superior visual
performance for embedded applications.

More information about the panel can be found
at the `NXP MX9 DSI OLED Panel website`_.

Current supported displays
==========================

+--------------+------------------------------+
| Display      | Shield Designation           |
|              |                              |
+==============+==============================+
| MX9 DSI      | nxp_mx9_dsi_oled             |
| OLED         |                              |
+--------------+------------------------------+

Programming
***********

Correct shield designation (see the table above) for your display must
be entered when you invoke ``west build``.

For example:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/mipi_dsi/api
   :board: imx95_evk/mimx9596/m7
   :shield: nxp_mx9_dsi_oled
   :goals: build

References
**********

.. target-notes::

.. _NXP MX9 DSI OLED Panel website:
   https://www.nxp.com/part/X_MX8-DSI-OLED2
