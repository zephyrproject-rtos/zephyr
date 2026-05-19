..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-FileCopyrightText: Copyright 2025 - 2026 Siemens Mobility GmbH
   SPDX-FileCopyrightText: Copyright 2025 Texas Instruments
   SPDX-License-Identifier: Apache-2.0

:orphan:

.. ti-k3-am64x-am243x-features

AM64x/AM243x features
=====================

The AM64x / AM243x is a SoC series that supports up to 2 Cortex-A53 cores (only
in AM64x SoCs) running with typically 1 GHz, up to 4 Cortex-R5F cores running
with typically 800 MHz and one Cortex-M4F core running with typically 400 MHz.
Inside Zephyr code both series are referenced as AM64x since they only differ in
whether Cortex-A cores are present. If you use an AM243x series SoC everything
related to Cortex-A cores doesn't apply to you. These things are sometimes still
documented due to shared documentation files.

The ARM cores and peripherals are separated into the MAIN and MCU domain whereby
only the ARM Cortex-M4F core is inside the MCU domain. By default both domains
can interact with each other but it is also possible to isolate both domains.

A non-exhaustive overview can be found in the following listing. Not all
peripherals are supported inside Zephyr as of now:

- Up to 2 ARM Cortex-A53 cores
- Up to 2 dualcore Cortex-R5F clusters
- 1x Cortex-M4F
- 2 MiB internal SRAM
- GPIO
- ADC
- UART
- SPI
- I2C
- Multiple Timers
- Multiple Watchdogs
- CAN-FD
- Ethernet
- USB
- PCIe
- External (LP)DDR4 RAM interface
- DMSC (centralized power / resource management)
- Crypto acceleration
- Secureboot
