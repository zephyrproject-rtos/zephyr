.. zephyr:board:: sr100_rdk

Overview
********

The Synaptics Astra™ SR100 Series of AI MCUs is designed to deliver
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
  * 2x USB 2.0 Type-C™ ports: Peripheral mode (Hi-Speed) and system power input
  * Push buttons for system reset and wake-up
  * Slide switches for image programming, mute control, and debugging
  * Several LEDs, including two user-controllable LEDs

* Daughter card interface options:

  * 2x MIPI CSI-2® 2-lane RX interfaces (1.5 Gb/s max bandwidth): CSI0 on
    Samtec™ connector (shared with DVP), CSI1 on 15-pin FPC connector
  * 1x MIPI CSI-2® TX interface (1.5 Gb/s max bandwidth) on 15-pin FPC connector
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

Connections and IOs
===================

A detailed description on the hardware connections, IOs and peripherals can be found on the
`Synaptics Astra MCU website`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

To build an application, use the standard Zephyr command. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sr100_rdk
   :goals: build

Flashing
========

Connect the board using the USB-C connector to your host machine. Make sure you can see the
CMSIS-DAP endpoint using ``lsusb``:

.. code-block:: console

   $ lsusb | grep CMSIS-DAP
   Bus 001 Device 058: ID cafe:4006 Synaptics, Inc SR100 CMSIS-DAP

Next, run the ``west debugserver`` command.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sr100_rdk
   :goals: debugserver

Now, in a separate terminal, run the flashing script provided by Synaptics:

.. code-block:: console

   python openocd_flash.py build/zephyr/zephyr.bin 0x0 0x0 1


References
**********

.. target-notes::

.. _Synaptics website: https://www.synaptics.com/assets/product-brief/sr-series
.. _Synaptics Astra MCU website: https://synaptics-astra-mcu.github.io/doc/v/latest/platform/Astra_Machina_Micro_SR100_Series_Evaluation_Platform_Kit_RevC_UG_511-001445-02_RevA.html
