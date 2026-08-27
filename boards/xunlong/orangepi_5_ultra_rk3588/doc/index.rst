.. zephyr:board:: orangepi_5_ultra_rk3588

Overview
********

Orange Pi 5 Ultra uses Rockchip RK3588, a new generation of octa-core 64-bit ARM
processor, which includes quad-core A76 and quad-core A55. It adopts Samsung's 8nm
LP process technology, with a large core frequency of up to 2.4GHz. It integrates ARM
Mali-G610 MP4 GPU, embedded high-performance 3D and 2D image acceleration
modules, built-in AI accelerator NPU with up to 6 Tops computing power,
4GB/8GB/16GB (LPDDR5) memory, and has up to 8K display processing capabilities.

Orange Pi 5 Ultra introduces a wide range of interfaces, including HDMI output,
HDMI input, Wi-Fi6, M.2 M-key PCIe3.0x4, 2.5G network port, USB2.0, USB3.1
interface and 40pin expansion pin header, etc. It can be widely used in high-end tablets,
edge computing, artificial intelligence, cloud computing, AR/VR, smart security, smart
home and other fields, covering all industries of AIoT.

Orange Pi 5 Ultra supports Orange Pi OS, the official operating system developed by
Orange Pi. It also supports operating systems such as Android 13, Debian11, Debian12,
Ubuntu20.04 and Ubuntu22.04.

Hardware
********

- Hardware Features:

  - CPU:

    - Rockchip RK3588(8nm LP process)

    - 8-core 64-bit processor

    - Quad-core Cortex-A76 and quad-core Cortex-A55 big and
      small core architecture

    - The main frequency of the big core is up to 2.4GHz, and
      the main frequency of the small core is up to 1.8GHz

  - GPU:

    - Integrated ARM Mali-G610

    - OpenGL ES1.1/2.0/3.2, OpenCL 2.2 and Vulkan 1.2

  - NPU:

    - Built-in AI accelerator NPU with up to 6 Tops computing
      power

    - Support INT4/INT8/INT16 mixed operations

  - video:

    - 1 * HDMI 2.1, Maximum support 8K @60Hz

    - 1 * HDMI Input, up to 4K @60FPS

    - 1 * MIPI D-PHY TX 4Lane

  - Memory:

    - 4GB/8GB/16GB(LPDDR5)

  - Camera:

    - 2 * MIPI CSI 4Lane

    - 1 * MIPI D-PHY RX 4Lane

  - PMU:

    - RK806-1

  - Onboard storage:

    - eMMC socket, can be connected to external eMMC module

    - 16MB QSPI Nor FLASH

    - MicroSD(TF) Card Slot

    - PCIe3.0x4 M.2 M-KEY(SSD) Slots

  - Ethernet:

    - 1 * PCIe 2.5G Ethernet port(RTL8125BG)

  - WIFI+BT:

    - Onboard Wi-Fi 6E+BT 5.3/BLE Modules: AP6611

    - Wi-Fi interface: SDIO3.0

    - BT interface: UART/PCM

  - Audio:

    - 3.5mm Headphone jack Audio input/output

    - Onboard MIC input

    - 2 * HDMI Output

  - PCIe M.2 M-KEY:

    - PCIe 3.0 x 4 lanes, For connecting NVMe SSD

  - USB interface:

    - 1 * USB3.0 support Device or HOST model

    - 1 * USB3.0 HOST

    - 2 * USB2.0 HOST

  - 40pin Extension pin header:

    For expansion UART, PWM, I2C, SPI, CAN and GPIO interface

  - Debug serial port:

    Included in 40PIN expansion port

  - LED Light:

    RGB LED three-color indicator

  - button:

    - 1 * MaskROM Key

    - 1 * Power button

  - powered by:

    Type-C Interface power supply 5V/5A

Supported Features
==================

.. zephyr:board-supported-hw::

There are multiple serial ports on the board: Zephyr is using
uart2 as serial console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :host-os: unix
  :board: orangepi_5_ultra_rk3588
  :goals: build

This will build an image with the hello world sample app.

Build the zephyr image:

.. code-block:: console

   mkimage -C none -A arm64 -O linux -a 0x10000000 -e 0x10000000 -d build/zephyr/zephyr.bin build/zephyr/zephyr.img

Burn the image on the board:

.. code-block:: console

   dd bs=1M if=/dev/zero of=/dev/mmcblk1 count=1000 status=progress
   dd bs=1M if=zephyr.img of=/dev/mmcblk1 status=progress

Use U-Boot to load and run Zephyr:

.. code-block:: console

   mmc dev 1
   mmc read 0x10000000 0x0 0x10000
   bootm start 0x10000000
   bootm loados
   bootm go

0x10000 is the size (in number of sectors) or your image. Increase it if needed.

It will display the following console output:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-455-gddc43f0d353d ***
   Hello World! orangepi_5_ultra_rk3588/rk3588

Flashing
========

Zephyr image can be loaded in DDR memory at address 0x10000000 from SD Card,
EMMC, QSPI Flash or downloaded from network in u-boot.

References
==========

More information can refer to OrangePi official website:
`OrangePi website`_.

.. _OrangePi website:
   http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/details/Orange-Pi-5-Ultra.html
