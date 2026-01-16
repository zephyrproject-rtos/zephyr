.. zephyr:code-sample:: spi_master
   :name: SPI Master

   Demonstrate SPI master communication with DMA support on STM32 boards.

Overview
********

This sample demonstrates how to use the SPI driver in master mode on STM32
microcontrollers with DMA support. It continuously sends messages to an SPI
slave device and displays the received responses.

The sample showcases:

* SPI master configuration using devicetree
* GPIO-based chip select control
* DMA-accelerated SPI transfers
* Full-duplex SPI communication
* Zephyr logging system integration

Requirements
************

* STM32 board with SPI and DMA support (tested on NUCLEO-L4R5ZI)
* SPI slave device (another microcontroller or SPI peripheral)
* Proper wiring between master and slave:

  * MOSI (Master Out Slave In)
  * MISO (Master In Slave Out)
  * SCK (Serial Clock)
  * CS (Chip Select)
  * GND (Common Ground)

Wiring
******

For NUCLEO-L4R5ZI (SPI1):

.. list-table::
   :header-rows: 1
   :widths: 15 15 25 45

   * - Function
     - STM32 Pin
     - Arduino Connector
     - Description
   * - SCK
     - PA5
     - CN7 Pin 10
     - SPI Clock
   * - MISO
     - PA6
     - CN7 Pin 12
     - Master In Slave Out
   * - MOSI
     - PA7
     - CN7 Pin 14
     - Master Out Slave In
   * - CS
     - PA15
     - CN7 Pin 17
     - Chip Select (Active Low)
   * - GND
     - GND
     - CN7 Pin 8
     - Common Ground

Connect these pins to your SPI slave device accordingly. Ensure proper voltage
level compatibility between devices.

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/spi_master
   :board: nucleo_l4r5zi
   :goals: build flash
   :compact:

After flashing, open a serial terminal (115200 8N1) to see the output.

Sample Output
*************

The sample outputs the following to the console:

.. code-block:: console

   *** Booting Zephyr OS build v3.x.x ***
   [00:00:00.010,000] <inf> spi_master_sample: SPI Master Sample
   [00:00:00.015,000] <inf> spi_master_sample: SPI device ready
   [00:00:00.020,000] <inf> spi_master_sample: === SPI Configuration ===
   [00:00:00.025,000] <inf> spi_master_sample: Frequency: 312500 Hz
   [00:00:00.030,000] <inf> spi_master_sample: Operation: 0x00040000
   [00:00:00.035,000] <inf> spi_master_sample: Slave: 0
   [00:00:00.040,000] <inf> spi_master_sample: CS GPIO port: 0x48000000
   [00:00:00.045,000] <inf> spi_master_sample: CS GPIO pin: 15
   [00:00:00.050,000] <inf> spi_master_sample: CS delay: 10 us
   [00:00:00.055,000] <inf> spi_master_sample: ========================
   [00:00:00.060,000] <inf> spi_master_sample: Waiting for slave initialization...
   [00:00:02.065,000] <inf> spi_master_sample: Starting SPI communication loop
   [00:00:02.070,000] <inf> spi_master_sample: TX string: [Hello from master 1] Message Length: 32
   [00:00:02.075,000] <inf> spi_master_sample: RX string: [Response from slave] Message Length: 32

Configuration
*************

SPI Frequency
=============

Adjust the SPI frequency in ``src/main.c`` by modifying the ``spi_cfg`` structure:

.. code-block:: c

   static struct spi_config spi_cfg = {
       .frequency = 312500,  /* 312.5 kHz */
       ...
   };

Common frequencies:

* 312500 (312.5 kHz) - Default, safe for most slaves
* 1000000 (1 MHz) - Faster, ensure slave supports it
* 4000000 (4 MHz) - High speed, check signal integrity

Message Size
============

Modify ``MSG_BUF_SIZE`` in ``src/main.c``:

.. code-block:: c

   #define MSG_BUF_SIZE 32  /* Bytes per transaction */

Transmission Interval
=====================

Change the delay between transmissions in ``main()``:

.. code-block:: c

   k_sleep(K_SECONDS(1));  /* Send every 1 second */

DMA Configuration
=================

DMA is enabled by default for efficient transfers. To disable DMA, remove
these lines from ``prj.conf``:

.. code-block:: kconfig

   CONFIG_DMA=n
   CONFIG_DMA_STM32=n
   CONFIG_SPI_STM32_DMA=n

Troubleshooting
***************

No Output
=========

1. Verify UART connection (ST-Link virtual COM port)
2. Check baud rate: 115200 8N1
3. Ensure board is powered

SPI Transaction Fails
=====================

1. **Verify wiring**: Especially GND connection
2. **Check slave power**: Ensure slave device is powered and initialized
3. **Signal integrity**: Use shorter wires, add pull-ups if needed
4. **Enable debug logging**: Set ``CONFIG_SPI_LOG_LEVEL_DBG=y``
5. **Reduce frequency**: Try 100 kHz first, then increase

CS Not Working
==============

1. Verify CS pin in overlay matches your wiring
2. Check CS polarity (active low vs active high)
3. Measure CS signal with oscilloscope/logic analyzer

No Slave Response
=================

1. Ensure slave is in SPI slave mode
2. Check MOSI/MISO aren't swapped
3. Verify clock polarity/phase match between master and slave
4. Add delay before first transaction: ``k_sleep(K_SECONDS(2))``

Logic Analyzer
==============

For detailed signal analysis:

1. Connect logic analyzer to SCK, MOSI, MISO, CS
2. Configure: SPI Mode 0, 312.5 kHz, MSB first
3. Verify clock edges and data alignment
4. Check CS assertion timing (should precede data by ~10 µs)

References
**********

* `STM32L4 SPI Documentation <https://www.st.com/resource/en/reference_manual/rm0432-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf>`_
* `Zephyr SPI API <https://docs.zephyrproject.org/latest/hardware/peripherals/spi.html>`_
* `NUCLEO-L4R5ZI User Manual <https://www.st.com/resource/en/user_manual/um2179-stm32-nucleo144-boards-mb1312-stmicroelectronics.pdf>`_
