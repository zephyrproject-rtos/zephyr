.. zephyr:code-sample:: tad2144
   :name: TDK integrated angle sensor with TMR (Tunnel Magneto Resistance)
   :relevant-api: sensor_interface

   Get angle and temperature data from a TDK TAD2144 sensor using SPI, I2C and Encoder(angle data only).

Overview
********

This sample application shows how to use the angle sensor with TMR (Tunnel 
Magneto Resistance) features of TDK Invensense sensors. It consists of:

** Angle: TMR angle sensors with Configurable digital outputs.
SPI (1kHz~10MHz)
I2C (400kHz, 1MHz)
Encoder (A, B, Z)

Wiring
*******

This sample requires an external breakout for the sensor. You must provide a 
devicetree overlay to define the interface (SPI, I2C, Encoder).

**Case 1: I2C Interface**
- **SDA/SCL**: I2C bus pins.
- **Interrupt GPIO**: For Data Ready (DRDY) signals.
- **SA1/SA2 GPIO**: Device address selection pins.

**Case 2: SPI Interface**
- **MOSI/MISO/SCK**: SPI bus pins.
- **CS GPIO**: Chip Select pin.
- **Interrupt GPIO**: For Data Ready (DRDY) signals.

**Case 3: Encoder Interface**
- **Phase A (MOSI)**, **Phase B (MISO)**, **Index Z (SCK)**: Connect to the encoder input pins.

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tad2144
   :board: nrf52840dk/nrf52840
   :goals: build flash

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.x.x ***
    Found device "tad2144_encoder", getting sensor data
    Angle: -101.43, Temp: 28.00 C
    Angle: -101.42, Temp: 28.00 C


