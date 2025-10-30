.. _nxp_mx8_dsi_oled1a:

NXP MX8 DSI OLED1A Panel
#########################

Overview
********

The NXP MX8 DSI OLED1A Panel is a high-resolution OLED display panel
designed for use with NXP i.MX8 series processors. This panel provides
excellent color reproduction and contrast ratio through OLED technology.
The display connects via MIPI DSI interface and offers superior visual
performance for embedded applications.

More information about the panel can be found
at the `NXP MX8 DSI OLED1A Panel website`_.

Current supported displays
==========================

+--------------+------------------------------+
| Display      | Shield Designation           |
|              |                              |
+==============+==============================+
| MX8 DSI      | nxp_mx8_dsi_oled1a           |
| OLED1A       |                              |
+--------------+------------------------------+

Programming
***********

Correct shield designation (see the table above) for your display must
be entered when you invoke ``west build``.

For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: imx93_evk/mimx9352/m33/ddr
   :shield: nxp_mx8_dsi_oled1a
   :goals: build

References
**********

.. target-notes::

.. _NXP MX8 DSI OLED1A Panel website:
   https://www.nxp.com/part/MX8-DSI-OLED1A
