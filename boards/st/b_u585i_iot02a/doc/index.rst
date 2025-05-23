.. zephyr:board:: b_u585i_iot02a

Overview
********

The B_U585I_IOT02A Discovery kit features an ARM Cortex-M33 based STM32U585AI MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the B_U585I_IOT02A Discovery kit:


- STM32U585AII6Q microcontroller featuring 2 Mbyte of Flash memory, 786 Kbytes of RAM in UFBGA169 package
- 512-Mbit octal-SPI Flash memory, 64-Mbit octal-SPI PSRAM, 256-Kbit I2C EEPROM
- USB FS, Sink and Source power, 2.5 W power capability
- 802.11 b/g/n compliant Wi-Fi® module from MXCHIP
- Bluetooth Low Energy from STMicroelectronics
- MEMS sensors from STMicroelectronics

  - 2 digital microphones
  - Relative humidity and temperature sensor
  - 3-axis magnetometer
  - 3D accelerometer and 3D gyroscope
  - Pressure sensor, 260-1260 hPa absolute digital output barometer
  - Time-of-flight and gesture-detection sensor
  - Ambient-light sensor

- 2 push-buttons (user and reset)
- 2 user LEDs

- Flexible power supply options:
    - ST-LINK/V3
    - USB Vbus
    - External sources


More information about the board can be found at the `B U585I IOT02A Discovery kit website`_.

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


Supported Features
==================

.. zephyr:board-supported-hw::

Zephyr board options
====================

The STM32U585i is an SoC with Cortex-M33 architecture. Zephyr provides support
for building for both Secure and Non-Secure firmware.

The BOARD options are summarized below:

+-------------------------------+-------------------------------------------+
| BOARD                         | Description                               |
+===============================+===========================================+
| b_u585i_iot02a                | For building Trust Zone Disabled firmware |
+-------------------------------+-------------------------------------------+
| b_u585i_iot02a/stm32u585xx/ns | For building Non-Secure firmware          |
+-------------------------------+-------------------------------------------+

Here are the instructions to build Zephyr with a non-secure configuration,
using :zephyr:code-sample:`tfm_ipc` sample:

   .. code-block:: bash

      $ west build -b b_u585i_iot02a/stm32u585xx/ns samples/tfm_integration/tfm_ipc/

Once done, before flashing, you need to first run a generated script that
will set platform option bytes config and erase platform (among others,
option bit TZEN will be set).

   .. code-block:: bash

      $ ./build/tfm/api_ns/regression.sh
      $ west flash

Please note that, after having run a TFM sample on the board, you will need to
run ``./build/tfm/api_ns/regression.sh`` once more to clean up the board from secure
options and get back the platform back to a "normal" state and be able to run
usual, non-TFM, binaries.
Also note that, even then, TZEN will remain set, and you will need to use
STM32CubeProgrammer_ to disable it fully, if required.

Connections and IOs
===================

B_U585I_IOT02A Discovery kit has 9 GPIO controllers (from A to I). These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `B U585I IOT02A board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA9/PA10 (ST-Link Virtual Port Com)
- LD1 : PH7
- LD2 : PH6
- user button : PC13
- SPI1 NSS/SCK/MISO/MOSI : PE12/P13/P14/P15 (Arduino SPI)
- I2C_1 SDA/SDL : PB9/PB8 (Arduino I2C)
- I2C_2 SDA/SDL : PH5/PH4
- DAC1 CH1 : PA4 (STMOD+1)
- ADC1_IN15 : PB0
- USB OTG : PA11/PA12
- PWM4 : CN14 PB6
- PWM3 : CN4 PE4

System Clock
------------

B_U585I_IOT02A Discovery System Clock could be driven by an internal or external oscillator,
as well as the main PLL clock. By default the System clock is driven by the PLL clock at 80MHz,
driven by 16MHz high speed internal oscillator.

Serial Port
-----------

B_U585I_IOT02A Discovery kit has 4 U(S)ARTs. The Zephyr console output is assigned to UART1.
Default settings are 115200 8N1.


Backup SRAM
-----------

In order to test backup SRAM you may want to disconnect VBAT from VDD. You can
do it by removing ``SB6`` jumper on the back side of the board.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

B_U585I_IOT02A Discovery kit includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash the board using various tools.


Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Connect the B_U585I_IOT02A Discovery kit to your host computer using the USB
port, then run a serial host program to connect with your Discovery
board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then, build and flash in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: b_u585i_iot02a
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! arm

Debugging
=========

Default flasher for this board is OpenOCD. It could be used in the usual way.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: b_u585i_iot02a
   :goals: debug

Disabling TrustZone |reg| on the board
======================================

If you have flashed a sample to the board that enables TrustZone, you will need
to disable it before you can flash and run a new non-TrustZone sample on the
board.

To disable TrustZone, it's necessary to change AT THE SAME TIME the ``TZEN``
and ``RDP`` bits. ``TZEN`` needs to get set from 1 to 0 and ``RDP``,
needs to be set from ``DC`` to ``AA`` (step 3 below).

This is docummented in the `AN5347, in section 9`_, "TrustZone deactivation".

However, it's possible that the ``RDP`` bit is not yet set to ``DC``, so you
first need to set it to ``DC`` (step 2).

Finally you need to set the "Write Protection 1 & 2" bytes properly, otherwise
some memory regions won't be erasable and mass erase will fail (step 4).

The following command sequence will fully deactivate TZ:

Step 1:

Ensure U23 BOOT0 switch is set to 1 (switch is on the left, assuming you read
"BOOT0" silkscreen label from left to right). You need to press "Reset" (B2 RST
switch) after changing the switch to make the change effective.

Step 2:

.. code-block:: console

   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob rdp=0xDC

Step 3:

.. code-block:: console

   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -tzenreg

Step 4:

.. code-block:: console

   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp1a_pstrt=0x7f
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp1a_pend=0x0
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp1b_pstrt=0x7f
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp1b_pend=0x0
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp2a_pstrt=0x7f
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp2a_pend=0x0
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp2b_pstrt=0x7f
   $ STM32_Programmer_CLI -c port=/dev/ttyACM0 -ob wrp2b_pend=0x0


.. _B U585I IOT02A Discovery kit website:
   https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html

.. _B U585I IOT02A board User Manual:
   https://www.st.com/resource/en/user_manual/um2839-discovery-kit-for-iot-node-with-stm32u5-series-stmicroelectronics.pdf

.. _STM32U585 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u575-585.html

.. _STM32U585 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0456-stm32u575585-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _STMicroelectronics customized version of OpenOCD:
   https://github.com/STMicroelectronics/OpenOCD

.. _AN5347, in section 9:
   https://www.st.com/resource/en/application_note/dm00625692-stm32l5-series-trustzone-features-stmicroelectronics.pdf
