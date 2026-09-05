.. zephyr:board:: mr_mcxn_t1

Overview
********

The NXP MR-MCXN-T1 is a dual-core mobile-robotics controller built around
the NXP MCXN947 MCU. The MCXN947 pairs two Arm Cortex-M33 cores running at up
to 150 MHz. The cores operate asymmetrically (AMP) and SMP is not supported.
On this board CPU0 runs a TrustZone-secure image and CPU1 acts as a secondary
core.

The board provides 100BASE-T1 single-pair Ethernet, dual CAN, an on-board
inertial measurement unit, and a stacking header that mates a series of
expansion boards (optical flow, GNSS and flex IO).

Hardware
********

- MCXN947 MCU

  - Dual Cortex-M33 at up to 150 MHz (AMP, each core runs its own image)
  - 2 MB flash
  - 512 KB SRAM

- Ethernet

  - Synopsys DesignWare Ethernet MAC 10/100/1G Quality-of-Service
    controller (NXP ENET QoS) with 100BASE-T1 single-pair Ethernet through a
    TJA1103 PHY and IEEE 1588 hardware timestamping

- CAN

  - Two FlexCAN controllers, each behind a TJA1462 CAN transceiver

- Sensors

  - ICM-45686 6-axis IMU (hub)
  - BMP581 barometer (I3C)
  - BMM350 magnetometer (I3C)

- Indicator

  - APA102 addressable LED driven over FlexIO

- Expansion

  - Stacking header exposing SPI, I2C, UART, PWM and ADC buses plus GPIO
    control lines

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The stacking header is described in the devicetree by the
``mr_mcxn_t1_header`` GPIO nexus (compatible ``mr-mcxn-t1-header``) and by
labeled bus nodes for each exposed peripheral. Expansion-board overlays
reference these nexus names rather than raw SoC controllers.

FlexCOMM3 and FlexCOMM7 are multiplexed across expansion boards, so only one
function per instance (SPI, UART or I2C) can be active at a time.

I3C is disabled by default. The I3C bus stalls in a busy-wait at startup, and
keeping it off frees flexcomm1 for the physical LPUART1 console. Re-enable the
bus and its two sensors once the startup behavior is resolved.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board is programmed and debugged with the on-board or an external SWD
probe. Build and flash the ``hello_world`` sample for CPU0 with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mr_mcxn_t1/mcxn947/cpu0
   :goals: flash

Open a serial terminal at 115200 8N1 on the LPUART1 console to see the output.

References
**********

- :zephyr:board:`frdm_mcxn947` for the MCXN947 SoC reference board
- `MR-MCXN-T1 hardware design files (KiCad) <https://github.com/CogniPilot/spinali_mcxn_t1_hub>`_
