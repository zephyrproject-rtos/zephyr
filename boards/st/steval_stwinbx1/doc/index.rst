.. zephyr:board:: steval_stwinbx1

Overview
********

The STWIN.box (STEVAL-STWINBX1) is a development kit that features an Arm|reg| Cortex|reg|-M33 based STM32U585AI MCU
and is a reference design that simplifies prototyping and testing of advanced industrial sensing applications in
IoT contexts such as condition monitoring and predictive maintenance.

The STEVAL-STWINBX1 kit consists of an STWIN.box core system, a 480mAh LiPo battery, an adapter for the ST-LINK debugger,
a plastic case, an adapter board for DIL 24 sensors and a flexible cable.

More information about the board can be found at the `STEVAL-STWINBX1 Development kit website`_.


Supported Features
******************

The STEVAL-STWINBX1 provides motion, environmental, and audio
sensor data through either the built-in RS485 transceiver, BLE, Wi-Fi, and
NFC or USB protocols to a host application running on a smartphone/PC to implement applications such as:

- Multisensing wireless platform for vibration monitoring and ultrasound detection
- Baby crying detection with Cloud AI learning
- Barometer / environmental monitoring
- Vehicle / goods tracking
- Vibration monitoring
- Compass and inclinometer
- Sensor data logger

(see `Sensing`_ section for the complete lists of available
sensors on board)

.. zephyr:board-supported-hw::

Hardware
********

The STM32U585xx devices are an ultra-low-power microcontrollers family (STM32U5
Series) based on the high-performance Arm|reg| Cortex|reg|-M33 32-bit RISC core.
They operate at a frequency of up to 160 MHz.

- Ultra-low-power with FlexPowerControl (down to 300 nA Standby mode and 19.5 uA/MHz run mode)
- Core: ARM |reg| 32-bit Cortex |reg| -M33 CPU with TrustZone |reg| and FPU.
- Performance benchmark:

  - 1.5 DMPIS/MHz (Drystone 2.1)
  - 651 CoreMark |reg| (4.07 CoreMark |reg| /MHZ)

- Security and cryptography

  - Arm |reg|  TrustZone |reg| and securable I/Os memories and peripherals
  - Flexible life cycle scheme with RDP (readout protection) and password protected debug
  - Root of trust thanks to unique boot entry and secure hide protection area (HDP)
  - Secure Firmware Installation thanks to embedded Root Secure Services
  - Secure data storage with hardware unique key (HUK)
  - Secure Firmware Update support with TF-M
  - 2 AES coprocessors including one with DPA resistance
  - Public key accelerator, DPA resistant
  - On-the-fly decryption of Octo-SPI external memories
  - HASH hardware accelerator
  - Active tampers
  - True Random Number Generator NIST SP800-90B compliant
  - 96-bit unique ID
  - 512-byte One-Time Programmable for user data
  - Active tampers

- Clock management:

  - 4 to 50 MHz crystal oscillator
  - 32 kHz crystal oscillator for RTC (LSE)
  - Internal 16 MHz factory-trimmed RC ( |plusminus| 1%)
  - Internal low-power 32 kHz RC ( |plusminus| 5%)
  - 2 internal multispeed 100 kHz to 48 MHz oscillators, including one auto-trimmed by
    LSE (better than  |plusminus| 0.25 % accuracy)
  - 3 PLLs for system clock, USB, audio, ADC
  - Internal 48 MHz with clock recovery

- Power management

  - Embedded regulator (LDO)
  - Embedded SMPS step-down converter supporting switch on-the-fly and voltage scaling

- RTC with HW calendar and calibration
- Up to 136 fast I/Os, most 5 V-tolerant, up to 14 I/Os with independent supply down to 1.08 V
- Up to 24 capacitive sensing channels: support touchkey, linear and rotary touch sensors
- Up to 17 timers and 2 watchdogs

  - 2x 16-bit advanced motor-control
  - 2x 32-bit and 5 x 16-bit general purpose
  - 4x low-power 16-bit timers (available in Stop mode)
  - 2x watchdogs
  - 2x SysTick timer

- ART accelerator

  - 8-Kbyte instruction cache allowing 0-wait-state execution from Flash and
    external memories: up to 160 MHz, MPU, 240 DMIPS and DSP
  - 4-Kbyte data cache for external memories

- Memories

  - 2-Mbyte Flash memory with ECC, 2 banks read-while-write, including 512 Kbytes with 100 kcycles
  - 786-Kbyte SRAM with ECC OFF or 722-Kbyte SRAM including up to 322-Kbyte SRAM with ECC ON
  - External memory interface supporting SRAM, PSRAM, NOR, NAND and FRAM memories
  - 2 Octo-SPI memory interfaces

- Rich analog peripherals (independent supply)

  - 14-bit ADC 2.5-Msps, resolution up to 16 bits with hardware oversampling
  - 12-bit ADC 2.5-Msps, with hardware oversampling, autonomous in Stop 2 mode
  - 12-bit DAC, low-power sample and hold
  - 2 operational amplifiers with built-in PGA
  - 2 ultra-low-power comparators

- Up to 22 communication interfaces

  - USB Type-C / USB power delivery controller
  - USB OTG 2.0 full-speed controller
  - 2x SAIs (serial audio interface)
  - 4x I2C FM+(1 Mbit/s), SMBus/PMBus
  - 6x USARTs (ISO 7816, LIN, IrDA, modem)
  - 3x SPIs (5x SPIs with dual OCTOSPI in SPI mode)
  - 1x FDCAN
  - 2x SDMMC interface
  - 16- and 4-channel DMA controllers, functional in Stop mode
  - 1 multi-function digital filter (6 filters)+ 1 audio digital filter with
    sound-activity detection

- CRC calculation unit
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell |trade|
- True Random Number Generator (RNG)

- Graphic features

  - Chrom-ART Accelerator (DMA2D) for enhanced graphic content creation
  - 1 digital camera interface

- Mathematical co-processor

 - CORDIC for trigonometric functions acceleration
 - FMAC (filter mathematical accelerator)


More information about STM32U585AI can be found here:

- `STM32U585 on www.st.com`_
- `STM32U585 reference manual`_

Connectivity
************

   - **BlueNRG-M2SA** Bluetooth|reg| low energy v5.2 wireless technology module
     (`BlueNRG-M2 datasheet`_)
   - **MXCHIP EMW3080** (802.11 b/g/n compliant Wi-Fi module)
   - **ST25DV64K** dynamic NFC/RFID tag IC with 64-Kbit EEPROM
     (`st25dv64k datasheet`_)
   - USB Type-C|trade| connector (power supply and data)
   - STDC14 programming connector for **STLINK-V3MINI**
     (`stlink-v3mini`_)
   - microSD card socket

Sensing
*******

  - **ILPS22QS** MEMS pressure sensor
    (`ilps22qs datasheet`_)
  - **STTS22H** Digital temperature sensor
    (`stts22hh datasheet`_)
  - **TSV912** wide-bandwidth (8 MHz) rail-to-rail I/O op-amp
    (`tsv912 datasheet`_)
  - **ISM330DHCX** iNEMO IMU, 3D accelerometer and 3D gyroscope with Machine Learning Core and Finite State Machine
    (`ism330dhcx datasheet`_)
  - **IIS3DWB** wide bandwidth accelerometer
    (`iis3dwb datasheet`_)
  - **IIS2DLPC** high-performance ultra-low-power 3-axis accelerometer for industrial applications
    (`iis2dlpc datasheet`_)
  - **IIS2MDC** 3-axis magnetometer
    (`iis2mdc datasheet`_)
  - **IIS2ICLX** high-accuracy, high-resolution, low-power, 2-axis digital inclinometer with Machine Learning Core
    (`iis2iclx datasheet`_)
  - **IMP23ABSU** analog MEMS microphone
    (`imp23absu datasheet`_)
  - **IMP34DT05** digital MEMS microphone
    (`imp34dt05 datasheet`_)

Connections and IOs
*******************

- 2x user LEDs

  - **led0** (Green)
  - **led1** (Orange)

- 4x buttons/switch

  - **User** / **boot0** button, available to user application
    but useful to let the SensorTile.box PRO enter DFU mode
    if found pressed after h/w reset (see **rst** button and
    `Programming and Debugging`_ section)
  - **RESET** button, used to reset the board
  - **PWR** button, used to Power on/off the board


For more details please refer to `STEVAL-STWINBX1 board User Manual`_.

System Clock
------------

STEVAL-STWINBX1 System Clock could be driven by an internal or external oscillator,
as well as the main PLL clock. By default the System clock is driven by the PLL clock at 160MHz,
driven by 16MHz high speed external oscillator.
The internal AHB/APB1/APB2/APB3 AMBA buses are all clocked at 160MHz.

Serial Port
-----------

The USART2 is connected to JTAG/SWD connector
and may be used as console.

USB interface
-------------

STEVAL-STWINBX1 can be connected as a USB device to a PC host through its USB-C connector.
The final application may use it to declare STEVAL-STWINBX1 device as belonging to a
certain standard or vendor class, e.g. a CDC, a mass storage or a composite device with both
functions.

Console
-------

There are two possible options for Zephyr console output:


- through common CDC ACM UART backend configuration for all boards

- through USART2 which is available on SWD connector (CN4). In this case a JTAG adapter
  can be used to connect STEVAL-STWINBX1 and have both SWD and console lines available.

  To enable console and shell over UART:

  - in your prj.conf, override the board's default configuration by setting :code:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM=n`

  - add an overlay file named ``<board>.overlay``:

.. code-block:: dts

   / {
       chosen {
          zephyr,console = &usart2;
          zephyr,shell-uart = &usart2;
        };
     };


Console default settings are 115200 8N1.

Programming and Debugging
-------------------------

There are two alternative methods of flashing ST Sensortile.box Pro board:

1. Using DFU software tools

   This method requires to enter STM32U585 ROM bootloader DFU mode
   by powering up (or reset) the board while keeping the USER (BOOT0) button pressed.
   No additional hardware is required except a USB-C cable. This method is fully
   supported by :ref:`flash-debug-host-tools`.
   You can read more about how to enable and use the ROM bootloader by checking
   the application note `AN2606`_ (STM32U585xx section).

2. Using SWD hardware tools

   The STEVAL-STWINBX1 does not include a on-board debug probe.
   It requires to connect additional hardware, like a ST-LINK/V3
   embedded debug tool, to the board STDC14 connector (CN4) labeled ``MCU-/SWD``.


Install dfu-util
----------------

.. note::
   Required only to use dfu-util runner.

It is recommended to use at least v0.9 of dfu-util. The package available in
Debian and Ubuntu can be quite old, so you might have to build dfu-util from source.
Information about how to get the source code and how to build it can be found
at the `DFU-UTIL website`_

Install STM32CubeProgrammer
---------------------------

.. note::
   Required to program over DFU (default) or SWD.

It is recommended to use the latest version of `STM32CubeProgrammer`_


Flash an Application to STEVAL-STWINBX1
------------------------------------------

There are two ways to enter DFU mode:

1. USB-C cable not connected

   While pressing the USER button, connect the USB-C cable to the USB OTG STEVAL-STWINBX1
   port and to your computer.

2. USB-C cable connected

   While pressing the USER button, press the RESET button and release it.

With both methods, the board should be forced to enter DFU mode.

Check that the board is indeed in DFU mode:

.. code-block:: console

   $ sudo dfu-util -l
   dfu-util 0.9

   Copyright 2005-2009 Weston Schmidt, Harald Welte and OpenMoko Inc.
   Copyright 2010-2019 Tormod Volden and Stefan Schmidt
   This program is Free Software and has ABSOLUTELY NO WARRANTY
   Please report bugs to http://sourceforge.net/p/dfu-util/tickets/

   Found DFU: [0483:df11] ver=0200, devnum=58, cfg=1, intf=0, path="3-1", alt=2, name="@OTP Memory   /0x0BFA0000/01*512 e", serial="207136863530"
   Found DFU: [0483:df11] ver=0200, devnum=58, cfg=1, intf=0, path="3-1", alt=1, name="@Option Bytes   /0x40022040/01*64 e", serial="207136863530"
   Found DFU: [0483:df11] ver=0200, devnum=58, cfg=1, intf=0, path="3-1", alt=0, name="@Internal Flash   /0x08000000/256*08Kg", serial="207136863530"

You should see the following confirmation on your Linux host:

.. code-block:: console

   $ dmesg
   usb 3-1: new full-speed USB device number 16 using xhci_hcd
   usb 3-1: New USB device found, idVendor=0483, idProduct=df11, bcdDevice= 2.00
   usb 3-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
   usb 3-1: Product: DFU in FS Mode
   usb 3-1: Manufacturer: STMicroelectronics
   usb 3-1: SerialNumber: 207136863530

You can build and flash the provided sample application
(:zephyr:code-sample:`stwinbx1_sensors`) that reads sensors data and outputs
values on the console.

.. _STEVAL-STWINBX1 Development kit website:
   https://www.st.com/en/evaluation-tools/steval-stwinbx1.html

.. _STEVAL-STWINBX1 board User Manual:
   https://www.st.com/resource/en/user_manual/um2965-getting-started-with-the-stevalstwinbx1-sensortile-wireless-industrial-node-development-kit-stmicroelectronics.pdf

.. _STM32U585 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u575-585.html

.. _STM32U585 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0456-stm32u575585-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _DFU-UTIL website:
   http://dfu-util.sourceforge.net/

.. _AN2606:
   http://www.st.com/content/ccc/resource/technical/document/application_note/b9/9b/16/3a/12/1e/40/0c/CD00167594.pdf/files/CD00167594.pdf/jcr:content/translations/en.CD00167594.pdf

.. _BlueNRG-M2 datasheet:
   https://www.st.com/en/product/BlueNRG-M2

.. _st25dv64k datasheet:
   https://www.st.com/en/nfc/st25dv64k.html

.. _stlink-v3mini:
   https://www.st.com/en/development-tools/stlink-v3mini.html

.. _ilps22qs datasheet:
   https://www.st.com/en/mems-and-sensors/ilps22qs.html

.. _stts22hh datasheet:
   https://www.st.com/en/mems-and-sensors/stts22h.html

.. _tsv912 datasheet:
   https://www.st.com/en/automotive-analog-and-power/tsv912.html

.. _ism330dhcx datasheet:
   https://www.st.com/en/mems-and-sensors/ism330dhcx.html

.. _iis3dwb datasheet:
   https://www.st.com/en/mems-and-sensors/iis3dwb.html

.. _iis2dlpc datasheet:
   https://www.st.com/en/mems-and-sensors/iis2dlpc.html

.. _iis2mdc datasheet:
   https://www.st.com/en/mems-and-sensors/iis2mdc.html

.. _iis2iclx datasheet:
   https://www.st.com/en/mems-and-sensors/iis2iclx.html

.. _imp23absu datasheet:
   https://www.st.com/en/mems-and-sensors/imp23absu.html

.. _imp34dt05 datasheet:
   https://www.st.com/en/mems-and-sensors/imp34dt05.html
