.. _weact_ov2640_cam_module:

WeAct Studio MiniSTM32H7xx OV2640 Camera Sensor
###############################################

Overview
********

The OV2640 camera sensor is designed to interface with the WeAct Studio
MiniSTM32H7xx boards, providing camera sensor capabilities. This shield
integrates the OV2640 camera module, which is capable of capturing images
and video with a resolution of up to 2 megapixels.

.. figure:: ov2640.webp
   :align: center
   :alt: OV2640 Camera Sensor

More information about the OV2640 camera sensor can be found on the
`MiniSTM32H7xx GitHub`_ and in the `OV2640 datasheet`_.

Requirements
************

Your board needs to have a ``zephyr_camera_dvp`` device tree label to work with this shield.

Pin Assignments
===============

The shield connects to the WeAct Studio MiniSTM32H7xx board via the
following pins:

+--------------+-----------+-----------------------------+
| Shield Pin   | Board Pin | Function                    |
+==============+===========+=============================+
| DCMI_D0      | PC6       | DCMI Data Line 0            |
+--------------+-----------+-----------------------------+
| DCMI_D1      | PC7       | DCMI Data Line 1            |
+--------------+-----------+-----------------------------+
| DCMI_D2      | PE0       | DCMI Data Line 2            |
+--------------+-----------+-----------------------------+
| DCMI_D3      | PE1       | DCMI Data Line 3            |
+--------------+-----------+-----------------------------+
| DCMI_D4      | PE4       | DCMI Data Line 4            |
+--------------+-----------+-----------------------------+
| DCMI_D5      | PD3       | DCMI Data Line 5            |
+--------------+-----------+-----------------------------+
| DCMI_D6      | PE5       | DCMI Data Line 6            |
+--------------+-----------+-----------------------------+
| DCMI_D7      | PE6       | DCMI Data Line 7            |
+--------------+-----------+-----------------------------+
| DCMI_HSYNC   | PA4       | DCMI HSYNC                  |
+--------------+-----------+-----------------------------+
| DCMI_VSYNC   | PB7       | DCMI VSYNC                  |
+--------------+-----------+-----------------------------+
| DCMI_PIXCLK  | PA6       | DCMI Pixel Clock            |
+--------------+-----------+-----------------------------+
| I2C_SDA      | PB9       | I2C Data Line               |
+--------------+-----------+-----------------------------+
| I2C_SCL      | PB8       | I2C Clock Line              |
+--------------+-----------+-----------------------------+
| RCC_MCO1     | PA8       | Clock Output                |
+--------------+-----------+-----------------------------+
| SUPPLY       | PA7       | Power Supply Control (GPIO) |
+--------------+-----------+-----------------------------+

Programming
***********

Set ``--shield weact_ov2640_cam_module`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture_to_lvgl/
   :board: mini_stm32h743
   :shield: weact_ov2640_cam_module
   :gen-args: -DCONFIG_BOOT_DELAY=2000
   :goals: build

.. _MiniSTM32H7xx GitHub:
   https://github.com/WeActStudio/MiniSTM32H7xx

.. _OV2640 datasheet:
   https://www.uctronics.com/download/cam_module/OV2640DS.pdf
