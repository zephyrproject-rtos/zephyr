.. zephyr:board:: sr100_rdk

Overview
********

The Synaptics Astra |trade| SR100 Series of AI MCUs is designed to deliver
high-performance, AI-Native, multimodal compute to consumer, enterprise, and
industrial Internet of Things (IoT) workloads.

* The Astra Machina Micro SR100 Evaluation Platform Kit features the following
  key components:

  * Synaptics SR110 (122-FCCSP) Audio & Vision AI processor
  * Debug IC Synaptics SR100 (84-WCCSP)
  * Storage: 128 Mbit QSPI NOR Flash
  * PMIC: Buck-Boost DC/DC for SR100 VBAT
  * Highly sensitive ambient light sensor: TCS34303
  * 3-axis accelerometer: MC3479
  * M.2 E-key 2230 receptacle: Supports SDIO, UART, and PCM for Wi-Fi/BT modules
  * 2x USB 2.0 Type-C |trade| ports: Peripheral mode (Hi-Speed) and system power input
  * Push buttons for system reset and wake-up
  * Slide switches for image programming, mute control, and debugging
  * Several LEDs, including two user-controllable LEDs

* Daughter card interface options:

  * 2x MIPI CSI-2 |reg| 2-lane RX interfaces (1.5 Gb/s max bandwidth): CSI0 on
    Samtec |trade| connector (shared with DVP), CSI1 on 15-pin FPC connector
  * 1x MIPI CSI-2 |reg| TX interface (1.5 Gb/s max bandwidth) on 15-pin FPC connector
  * SWD JTAG daughter card for debugging
  * 2x 20-pin headers with GPIOs are for additional application
  * 4-pin header for UART debugging
  * 3-pin header for PIR

* System power supply:

  * USB Type-C
  * 2-pin, 2.0 mm pitch header for 1-cell Li-ion battery
  * 3-pin header for system power source selection

More information about Astra MCU series and the evaluation board can be found
at the `Synaptics website`_.

Supported Features
==================

.. zephyr:board-supported-hw::

The default board configuration can be found in
:zephyr_file:`boards/syna/sr100/sr100_rdk_defconfig`

References
**********

.. target-notes::

.. _Synaptics website: https://www.synaptics.com/assets/product-brief/sr-series
