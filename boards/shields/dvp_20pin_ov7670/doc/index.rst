.. _dvp_20pin_ov7670:

DVP 20-pin OV7670 Camera Module
###############################

Overview
********

This series of shields supports the camera modules which use a 18-pin connector compatible with
the :dtcompatible:`arducam,dvp-20pin-connector` to connect a devkit to an OV7670 image sensor via
DVP (Digital Video Port), also known as "parallel interface".

Only 18 pins out of the 20-pin connector are present.

It was originally produced by `Arducam`_ but is discontinuited, and now `Olimex`_ provides it.

Pins assignment
===============

+-----+--------------+-----+--------------+
| Pin | Function     | Pin | Function     |
+=====+==============+=====+==============+
| 1   | 3V3          | 2   | GND          |
+-----+--------------+-----+--------------+
| 3   | SCL          | 4   | SDA          |
+-----+--------------+-----+--------------+
| 5   | VS           | 6   | HS           |
+-----+--------------+-----+--------------+
| 7   | PCLK         | 8   | XCLK         |
+-----+--------------+-----+--------------+
| 9   | D7           | 10  | D6           |
+-----+--------------+-----+--------------+
| 11  | D5           | 12  | D4           |
+-----+--------------+-----+--------------+
| 13  | D3           | 14  | D2           |
+-----+--------------+-----+--------------+
| 15  | D1           | 16  | D0           |
+-----+--------------+-----+--------------+
| 17  | POWER_EN     | 18  | POWER_DOWN   |
+-----+--------------+-----+--------------+

Requirements
************

This shield can be used with any board that provides an 18 or 20-pin header spread over two rows
of 9 or 10 pins each with the above pinout, such as the `arduino Giga R1`_, `NXP FRDM-MCXN947`_,
ST boards with the `ST-CAMS-OMV`_ adapter, or any other board with a compatible connector.

Alternatively, it is possible to use jumper wires to connect the module to any devkit that
exposes their camera parallel port to pin headers.

Programming
***********

Set ``--shield dvp_20pin_ov7670`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: frdm_mcxn947
   :shield: dvp_20pin_ov7670
   :goals: build

References
**********

.. target-notes::

.. _ST-CAMS-OMV:
   https://www.st.com/en/evaluation-tools/b-cams-omv.html

.. _Arducam:
   https://docs.arducam.com/DVP-Camera-Module/Arduino-GIGA/Arduino-GIGA/Quick-Start-Guide/

.. _Arduino Giga R1:
   https://docs.arduino.cc/tutorials/giga-r1-wifi/giga-camera/

.. _NXP FRDM-MCXN947:
   https://www.nxp.com/docs/en/application-note/AN14191.pdf

.. _Olimex:
   https://www.olimex.com/Products/Components/Camera/CAMERA-OV7670/
