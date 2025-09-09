.. zephyr:board:: roc_rk3588_pc

Overview
********

The ROC-RK3588-PC is an Octa-Core 64-Bit Mini Computer powered by Rockchip RK3588, which supports up to 32GB RAM.
It features M.2 PCIe3.0 interface for NVMe SSD expansion and provides rich interfaces including HDMI 2.1, DP1.4.
Supporting WiFi 6 wireless transmission and Gigabit Ethernet, it enables high-speed network connectivity.

RK3588 octa-core 64-bit processor (4×Cortex-A76+4×Cortex-A55), with 8nm lithography process,
has frequency up to 2.4GHz. Integrated with ARM Mali-G610 MP4 quad-core GPU and built-in AI accelerator NPU,
it provides 6Tops computing power. Zephyr OS is ported to run on it.


- Board features:

  - RAM: Up to 32GB LPDDR4/LPDDR4x/LPDDR5
  - Storage:

    - Up to 128GB eMMC
    - M.2 PCIe3.0 NVMe SSD (2242/2260/2280)
    - TF-Card Slot
  - Wireless:

    - Supports WiFi 6 (802.11 a/b/g/n/ac/ax)
    - Supports BT 5.0
  - Display:

    - HDMI 2.1 (8K@60fps)
    - HDMI 2.0 (4K@60fps)
    - DP 1.4 (8K@30fps)
  - USB:

    - Two USB 3.0
    - One USB 2.0
    - One USB-C (USB3.0 OTG / DP1.4)
  - Network:

    - 1x 1000Mbps Ethernet (RJ45)
  - Debug:

    - UART debug ports for board
  - Other:

    - NPU with 6 TOPS computing power
    - ARM Mali-G610 MP4 quad-core GPU
    - Support OpenGL ES3.2/OpenCL 2.2/Vulkan1.1


Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A76 cores run up to 2.4 GHz and Cortex-A55 cores run up to 1.8 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART2.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use U-Boot to load the zephyr.bin to the memory and kick it:

.. code-block:: console

    tftp 0x50000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x50000000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3588_pc
   :goals: run

For ``roc_rk3588_pc//smp`` support, use this configuration to run Zephyr smp applications and subsys tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3588_pc//smp
   :goals: run

References
==========

More information can refer to Firefly official website:
`Firefly website`_.

.. _Firefly website:
   https://en.t-firefly.com/product/industry/rocrk3588pc.html?theme=pc
