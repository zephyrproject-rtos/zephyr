.. _fdc2x1x:

FDC2X1X: Capacitance-to-Digital Converter
#########################################

Overview
********

This sample application periodically reads frequency and capacitance data from the
FDC2X1X sensor in polling mode or optionally with data ready trigger. It is able
to read the 12-Bit and 28-Bit, as well as the 2-Channel and 4-Channel versions
(FDC2112, FDC2114, FDC2212, FDC2214). The 4-channel versions are chosen through
devicetree properties. The default in this sample is the 2-channel version.

Capacitive sensing is a low-power, low-cost, high-resolution contactless sensing
technique that can be applied to a variety of applications ranging from proximity
detection and gesture recognition to remote liquid level sensing. The sensor in
a capacitive sensing system is any metal or conductor, allowing for low cost and
highly flexible system design.
The main challenge limiting sensitivity in capacitive sensing applications is
noise susceptibility of the sensors. With the FDC2x1x innovative EMI resistant
architecture, performance can be maintained even in presence of high-noise environments.


Wiring
*******

This sample uses the FDC2X1X sensor controlled using the I2C interface.
Connect supply **VDD** and **GND**. The supply voltage can be in
the 2.7V to 3.6V range.

Connect **SD** to a GPIO to control the Shutdown Mode.

Connect Interface: **SDA**, **SCL** and optionally connect **INTB** to a
interrupt capable GPIO.

For detailed description refer to the `FDC2X1X datasheet`_
at pages 4-5.


Building and Running
********************

This sample outputs sensor data to the console and can be read by any serial
console program. It should work with any platform featuring a I2C interface.
The platform in use requires a custom devicetree overlay.
In this example the :ref:`nrf9160dk_nrf9160` board is used. The devicetree
overlay of this board provides example settings for evaluation, which
you can use as a reference for other platforms.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/fdc2x1x
   :board: nrf9160dk/nrf9160
   :goals: build flash
   :compact:

Sample Output: 2-Channel, 28-Bit (FDC2212)
==========================================

.. code-block:: console

        ch0: 5.318888 MHz ch1: 5.150293 MHz
        ch0: 49.742308 pF ch1: 53.052260 pF

        ch0: 5.318819 MHz ch1: 5.150307 MHz
        ch0: 49.743612 pF ch1: 53.051964 pF

        ch0: 5.318822 MHz ch1: 5.150200 MHz
        ch0: 49.743548 pF ch1: 53.054176 pF

        ch0: 5.318752 MHz ch1: 5.150265 MHz
        ch0: 49.744860 pF ch1: 53.052828 pF

        <repeats endlessly>


Sample Output: 4-Channel, 12-Bit (FDC2114)
==========================================

.. code-block:: console

        ch0: 4.966171 MHz ch1: 4.946465 MHz ch2: 4.985879 MHz ch3: 4.907051 MHz
        ch0: 57.059016 pF ch1: 57.514568 pF ch2: 56.608844 pF ch3: 58.442204 pF

        ch0: 4.966171 MHz ch1: 4.946465 MHz ch2: 4.985879 MHz ch3: 4.907051 MHz
        ch0: 57.059016 pF ch1: 57.514568 pF ch2: 56.608844 pF ch3: 58.442204 pF

        ch0: 4.966171 MHz ch1: 4.946465 MHz ch2: 4.985879 MHz ch3: 4.907051 MHz
        ch0: 57.059016 pF ch1: 57.514568 pF ch2: 56.608844 pF ch3: 58.442204 pF

        ch0: 4.966171 MHz ch1: 4.946465 MHz ch2: 4.985879 MHz ch3: 4.907051 MHz
        ch0: 57.059016 pF ch1: 57.514568 pF ch2: 56.608844 pF ch3: 58.442204 pF

        <repeats endlessly>


References
**********

FDC2X1X Datasheet and Product Info:
 https://www.ti.com/product/FDC2114

.. _FDC2X1X datasheet: https://www.ti.com/lit/gpn/fdc2114
