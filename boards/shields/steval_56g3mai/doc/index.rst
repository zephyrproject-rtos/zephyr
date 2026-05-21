.. _steval_56g3mai:

ST STEVAL-56G3MAI (VD56G3 S-Board)
##################################

Overview
********

The `STEVAL-56G3MAI`_ is a small camera daughter-board ("VD56G3 S-Board")
manufactured by STMicroelectronics. It hosts the
`VD56G3 <https://www.st.com/en/imaging-and-photonics-solutions/vd56g3.html>`_,
a 1.5 megapixel monochrome global-shutter CMOS image sensor, behind an
M12 lens holder. The board exposes a 22-pin Raspberry Pi-compatible FFC
CSI-2 connector and is shipped with the corresponding ribbon cable, which
makes it plug-and-play on any host board complying with that mechanical
and electrical convention (Raspberry Pi, STM32 Nucleo N6 / Discovery,
NVIDIA Jetson, etc.).

Sensor highlights:

* 1124 x 1364 monochrome pixels, 2.61 µm pitch, BSI + CDTI
* Global shutter, no rolling-shutter motion blur
* Sensitive in the visible and near-infrared (850 nm, 940 nm)
* MIPI CSI-2 D-PHY output, 1 or 2 lanes, RAW8 / RAW10
* I²C (CCI) control, 1.8 V logic
* In-sensor autoexposure, binning, cropping, mirror/flip, ISL

Requirements
************

This shield can be used on any Zephyr board that:

* exposes a 22-pin Raspberry Pi camera connector matching the
  ``raspberrypi,csi-connector`` binding,
* declares the standard labels ``csi_connector``, ``csi_i2c``,
  ``csi_interface``, ``csi_ep_in`` and ``csi_capture_port`` in its
  device tree (see for instance
  :zephyr:board:`stm32n6570_dk`),
* enables a Video subsystem capable of receiving MIPI CSI-2 (e.g. the
  STM32 DCMIPP driver via :kconfig:option:`CONFIG_VIDEO_STM32_DCMIPP`).

Wiring summary
==============

==============  ====================================================
Sensor pin      Routed via
==============  ====================================================
SDA / SCL       ``csi_i2c`` (Fast mode plus, 1 Mbit/s capable)
XSHUTDOWN       ``csi_connector CSI_IO0`` (active-low reset)
CLKIN           ``vd56g3_input_clock`` (12 MHz fixed-clock)
DATA1+/-, DATA2+/-, CLK+/-  ``csi_interface`` CSI-2 RX
==============  ====================================================

Usage
*****

The shield can be used in any application by setting ``SHIELD`` to
``steval_56g3mai`` on boards with the necessary device tree node labels.

For example, with the video capture sample on :zephyr:board:`stm32n6570_dk`:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: stm32n6570_dk
   :shield: steval_56g3mai
   :goals: build

Selecting the shield automatically:

* enables :kconfig:option:`CONFIG_VIDEO_VD56G3`,
* sets ``CONFIG_VIDEO_STM32_DCMIPP_SENSOR_PIXEL_FORMAT="Y10P"``,
* sets ``CONFIG_VIDEO_STM32_DCMIPP_SENSOR_WIDTH=1124`` /
  ``CONFIG_VIDEO_STM32_DCMIPP_SENSOR_HEIGHT=1364``,
* enables :kconfig:option:`CONFIG_GPIO_HOGS` so the STM32N6570 DK camera power
  rail (``csi_gpio1_hog``) comes up at boot.

.. note::

   Boards using the **Raspberry Pi connector standard** (22-pin FFC) often
   require MIPI data/clock lane P/N polarity inversion on the sensor side.
   The shield overlay enables ``st,csi-lane0-polarity-invert``,
   ``st,csi-lane1-polarity-invert``, and ``st,csi-clock-polarity-invert``
   for that reason.

   CSI-2 link frequency is not declared in the device tree endpoint yet
   (``link-frequencies`` needs 64-bit support in edtlib, see Zephyr issue
   `#109610 <https://github.com/zephyrproject-rtos/zephyr/issues/109610>`_).
   The driver selects the lane rate at runtime from the number of data lanes.

Programming and debugging
*************************

Programming and debugging are covered by the host board's documentation;
the shield itself is passive.

References
**********

.. _STEVAL-56G3MAI: https://www.st.com/en/evaluation-tools/steval-56g3mai.html

* `ST Linux V4L2 reference <https://github.com/STMicroelectronics/vd56g3-linux-driver>`_
* `datasheet <https://www.st.com/resource/en/datasheet/vd56g3.pdf>`_
