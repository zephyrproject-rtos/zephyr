.. zephyr:board:: roc_rk3588_pc

Overview
********

The ROC-RK3588-PC is an Octa-Core 64-Bit Mini Computer powered by Rockchip RK3588, which supports up to 32GB RAM.
It features M.2 PCIe3.0 interface for NVMe SSD expansion and provides rich interfaces including HDMI 2.1, DP1.4.
Supporting WiFi 6 wireless transmission and Gigabit Ethernet, it enables high-speed network connectivity.
More information can refer to `Firefly website <firefly_website_>`_.

The RK3588 has a cluster with quad-core Cortex-A55 and quad-core Cortex-A76 of which the
cores are all single-threaded. Both Cortex-A55 and Cortex-A76 processors implement the ARMv8-A architecture.
And the processors support both the AArch32 and AArch64 execution states at all Exception levels (EL0 to EL3).
Zephyr currently supports running natively on bare-metal RK3588 with quad-core Cortex-A55, as well as running as a virtual machine in both AArch64 and AArch32 modes.

- Board features:

  - RAM: Up to 32GB LPDDR4/LPDDR4x/LPDDR5
  - Storage:

    - Up to 128GB eMMC
    - M.2 PCIe3.0 NVMe SSD (2242/2260/2280)
    - TF-Card Slot
  - Wireless:

    - Supports WiFi 6 (802.11 a/b/g/n/ac/ax)
    - Supports BT 5.0
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

This board configuration uses UART2 and UART3 as the serial ports.
When Zephyr runs in bare-metal mode, UART2 is used as the serial port.
When Zephyr runs in virtual machine, UART3 can be used as the serial port.
The Hypervisor is responsible for correctly initializing UART3.

Programming and Debugging
*************************

Running on Bare Metal or AArch64 Virtual Machine
=================================================

To run Zephyr on bare-metal RK3588 hardware or within an AArch64 virtual machine, use this configuration for basic Zephyr applications and kernel tests.
For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3588_pc/rk3588_aarch64
   :goals: build

Then use U-Boot to load the zephyr.bin to the memory and kick it:

.. code-block:: console

    tftp 0x50000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x50000000

For SMP (Symmetric Multiprocessing) support on ``roc_rk3588_pc//smp``, use this configuration to build Zephyr SMP applications.
For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3588_pc/rk3588_aarch64/smp
   :goals: build

To run Zephyr within an AArch64 virtual machine, use the same build commands as for bare-metal. However, the way to launch Zephyr will depend on the hypervisor.

Running within AArch32 Virtual Machine
======================================

To run Zephyr within an AArch32 virtual machine, use this configuration for basic Zephyr applications and kernel tests.
For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: roc_rk3588_pc/rk3588_aarch32
   :goals: build

Then launch Zephyr using a hypervisor that supports AArch32 guest mode, such as `hvisor <hvisor_website_>`_.

References
==========

.. target-notes::

.. _firefly_website: https://en.t-firefly.com/product/industry/rocrk3588pc.html?theme=pc
.. _hvisor_website: https://github.com/syswonder/hvisor
