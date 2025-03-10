.. zephyr:board:: roc_rk3568_pc

Overview
********

The ROC-RK3568-PC is a Quad-Core 64-Bit Mini Computer, which supports 4G large RAM. M.2
and SATA3.0 interfaces enables expansion with large hard drives.
Providing dual Gigabit Ethernet ports, it supports WiFi 6 wireless transmission.
Control Port can be connected with RS485/RS232 devices.

RK3568 quad-core 64-bit Cortex-A55 processor, with brand new ARM v8.2-A architecture,
has frequency up to 2.0GHz. Zephyr OS is ported to run on it.


- Board features:

  - RAM: 4GB LPDDR4
  - Storage:

    - 32GB eMMC
    - M.2 PCIe 3.0 x 1 (Expand with 2242 / 2280 NVMe SSD)
    - TF-Card Slot
  - Wireless:

    - Supports WiFi 6 (802.11 AX)
    - Supports BT5.0
  - USB:

    - One USB 3.0
    - Two USB 2.0
    - One Type-C
  - Ethernet
  - M.2 PCIe3.0 (Expand with NVMe SSD)
  - LEDs:

    - 1x Power status LED
  - Debug

    - UART debug ports for board


Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 2.0 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART2.

Programming and Debugging
*************************

Use U-Boot to load the zephyr.bin to the memory and kick it:

.. code-block:: console

    tftp 0x40000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x40000000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3568_pc
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following ram console output:

.. code-block:: console

    ***  Booting Zephyr OS build bc695c6df5eb  ***
    thread_a: Hello World from cpu 0 on roc_rk3568_pc!
    thread_b: Hello World from cpu 0 on roc_rk3568_pc!
    thread_a: Hello World from cpu 0 on roc_rk3568_pc!
    thread_b: Hello World from cpu 0 on roc_rk3568_pc!


``roc_rk3568_pc//smp`` support, use this configuration to run Zephyr smp applications and subsys tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3568_pc//smp
   :goals: run

This will build an image with the shell_module sample app, boot it and
display the following ram console output:

.. code-block:: console

    ***  Booting Zephyr OS build bc695c6df5eb  ***
    I/TC: Secondary CPU 1 initializing
    I/TC: Secondary CPU 1 switching to normal world boot
    I/TC: Secondary CPU 2 initializing
    I/TC: Secondary CPU 2 switching to normal world boot
    I/TC: Secondary CPU 3 initializing
    I/TC: Secondary CPU 3 switching to normal world boot
    Secondary CPU core 1 (MPID:0x100) is up
    Secondary CPU core 2 (MPID:0x200) is up
    Secondary CPU core 3 (MPID:0x300) is up

    thread_a: Hello World from cpu 0 on roc_rk3568_pc!
    thread_b: Hello World from cpu 1 on roc_rk3568_pc!
    thread_a: Hello World from cpu 0 on roc_rk3568_pc!
    thread_b: Hello World from cpu 1 on roc_rk3568_pc!

References
==========

More information can refer to Firefly official website:
`Firefly website`_.

.. _Firefly website:
   https://en.t-firefly.com/product/industry/rocrk3568pc.html?theme=pc
