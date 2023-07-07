.. _imx8qm_mek:

NXP i.MX8QM MEK (Cortex-A53)
############################

Overview
********
The i.MX8QuadMax(i.MX8QM) Multisensory Enablement Kit (MEK) is based on
the i.MX8QM applications processor, composed of a cluster of 4 Cortex-A53
cores, 2 Cortex-A72 cores and 2 Cortex-M4 cores.

- Board features:

  - RAM: 6GB LPDDR4
  - Storage:

    - 32GB eMMC5.1
    - SD Socket
  - USB:
    - 1 x USB 3.0 OTG
    - 1 x USB 2.0 OTG
  - Ethernet
  - PCI-E M.2
  - Debug:
    - JTAG connector
    - Serial to USB connector

Supported Features
==================

The Zephyr mimx8qm_mek board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v3    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

Devices
=======

System Clock
------------

This board configuration uses a system clock frequency of 8 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with
the CPU's LPUART2. The signals from the LPUART2 module are routed to the
BB.

Programming and Debugging
*************************

At the moment, mimx8qm_mek_a53 is only meant to be used in conjunction with
the Jailhouse hypervisor.

TODO: in the future, Zephyr should be able to run bare metal.

References
==========

For more information about the board please refer to NXP's official
website: `NXP website`_.

.. _NXP website:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/i-mx-8quadmax-multisensory-enablement-kit-mek:MCIMX8QM-CPU
