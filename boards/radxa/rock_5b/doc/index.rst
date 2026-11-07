.. zephyr:board:: rock_5b

Overview
********

The `Radxa ROCK 5B`_ is a single-board computer based on the Rockchip RK3588
SoC. The SoC combines four Arm Cortex-A76 and four Arm Cortex-A55 CPU cores.
The board is available with up to 16 GB of LPDDR4X memory and provides eMMC,
microSD, PCIe, USB, Ethernet, and a 40-pin expansion header.

The SMP variant supports all four Cortex-A55 and four Cortex-A76 cores.

Hardware
********

The ROCK 5B includes the following hardware features:

* Rockchip RK3588 SoC
* Four Arm Cortex-A76 cores up to 2.4 GHz and four Arm Cortex-A55 cores up to 1.8 GHz
* Arm Mali-G610 MP4 GPU and neural processing unit (NPU) with up to 6 TOPS
* 64-bit LPDDR4X memory, available in 4 GB, 8 GB, and 16 GB variants
* eMMC connector and microSD card slot
* M.2 E Key connector with PCIe 2.1 x1, USB, SATA, SDIO, PCM, and UART signals
  for Wi-Fi 6 modules
* M.2 M Key connector with a PCIe 3.0 x4 interface for 2280 NVMe SSDs
* 2.5 Gigabit Ethernet with Power over Ethernet (PoE) support (requires an
  optional PoE HAT)
* Two USB 2.0 host ports and two USB 3.0 host/OTG ports
* Two HDMI outputs, one DisplayPort output over USB Type-C, and one MIPI DSI output
* One Micro HDMI input port
* One MIPI CSI camera interface
* 40-pin expansion header
* USB Type-C power delivery (PD 2.0)
* 100 mm x 75 mm board size

Supported Features
==================

.. zephyr:board-supported-hw::

The onboard Ethernet interface is not currently supported by Zephyr.

Serial Port
===========

Zephyr uses UART2 as the serial console at 1.5 Mbit/s. On the 40-pin
expansion header, connect GND to pin 6, the adapter RX input to the board TX
output on pin 8, and the adapter TX output to the board RX input on pin 10.
The adapter must support 1.5 Mbaud; use 1500000 baud, 8 data bits, no parity,
one stop bit, and no flow control. The `Radxa serial console documentation`_
describes the same connection in more detail.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications are built with the standard configuration, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: rock_5b
   :goals: build

To run Zephyr on all eight CPU cores, build the SMP variant:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: rock_5b//smp
   :goals: build

Create a legacy U-Boot image whose payload loads and runs at ``0x10000000``:

.. code-block:: console

   mkimage -C none -A arm64 -O linux -a 0x10000000 -e 0x10000000 \
     -d build/zephyr/zephyr.bin build/zephyr/zephyr.img

The Radxa U-Boot AArch64 Linux boot path requires a devicetree. Obtain
``rk3588-rock-5b.dtb`` from the Radxa Linux distribution and copy it and
``zephyr.img`` to a FAT partition accessible from U-Boot.

Check the temporary kernel and devicetree load addresses provided by U-Boot:

.. code-block:: console

   printenv kernel_addr_r fdt_addr_r

Load each file at its corresponding address, then pass the devicetree address
to ``bootm``. Adjust the MMC device and partition numbers for the selected boot
media:

.. code-block:: console

   fatload mmc 1:1 ${kernel_addr_r} zephyr.img
   fatload mmc 1:1 ${fdt_addr_r} rk3588-rock-5b.dtb
   bootm ${kernel_addr_r} - ${fdt_addr_r}

The legacy image header instructs U-Boot to move the Zephyr payload from
``kernel_addr_r`` to ``0x10000000`` before jumping to the entry point. Keeping
the temporary image, devicetree, and Zephyr runtime regions separate avoids
overwriting any of them during the handoff.

References
**********

More information is available from the `Radxa ROCK 5B documentation`_.

.. _Radxa ROCK 5B:
   https://radxa.com/products/rock5/5b

.. _Radxa ROCK 5B documentation:
   https://docs.radxa.com/en/rock5/rock5b

.. _Radxa serial console documentation:
   https://docs.radxa.com/en/rock5/rock5b/radxa-os/serial
