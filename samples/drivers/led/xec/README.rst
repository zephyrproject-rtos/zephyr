.. zephyr:code-sample:: led-xec
   :name: Breathing-blinking LED (BBLED)
   :relevant-api: led_interface

   Control a BBLED (Breathing-Blinking LED) using Microchip XEC driver.

Overview
********

This sample allows to test the Microchip led-xec driver which uses the
breathing-blinking LED (BBLED) controllers. The SoC design is fixed
allowing each BBLED control over one specific GPIO.

MEC15xx and MEC172x:

- BBLED controller 0 uses GPIO 0156.
- BBLED controller 1 uses GPIO 0157.
- BBLED controller 2 uses GPIO 0153.

MEC172x has a fourth instance of BBLED:

- BBLED controller 3 uses GPIO 0035

Test pattern
============

For each LEDs (one after the other):

- Turning on
- Turning off
- Blinking on: 0.1 sec, off: 0.1 sec
- Blinking on: 1 sec, off: 1 sec

Board Jumpers
*************

mec172xevb_assy6906 evaluation board
====================================

- BBLED0: GPIO 0156.
       - Connect GPIO 0156 to board LED4 by placing a wire from JP71-11 to J47-3.
       - Make sure there are no jumpers on JP54 1-2 and JP21 4-5

- BBLED1: GPIO 0157.
       - Connect GPIO 0156 to board LED5 by placing a wire from JP71-12 to J48-3.
       - Make sure there are no jumpers on JP54 3-4 and JP21 16-17

- BBLED2: GPIO 0153.
       - Connect GPIO 0153 to board LED7 by placing a wire from JP71-5 to JP146-5.

         JP146-5 is connected to MEC172x VCI_OUT1 without a jumper. Force VCI_OUT1
         high by forcing VCI_IN1 high: install jumper on J55 3-4 which pulls VCI_IN1
         to the VBAT rail via a 100K pull-up. Requires VBAT power rail is connected
         to VTR or some other power source.

- BBLED3: GPIO 0035.
       - Connect GPIO 0035 to board LED7 by placing a wire from JP67-19 to JP146-1.
       - Make sure there is no jumper on JP79 17-18.

         JP146-1 is connected to MEC172x VCI_OUT2 without a jumper. Force VCI_OUT2
         high by forcing VCI_IN2 high: install a jumper on J55 5-6 which pulls VCI_IN2
         to the VBAT rail via a 100K pull-up. Requires VBAT power rail is connected
         to VTR or some other power source.

mec15xxevb_assy6853 evaluation board
====================================

- BBLED0: GPIO 0156.
       - Add jumper on JP41 1-2 to connect GPIO 0156 to board LED2
       - Remove jumper on JP31 13-14

- BBLED1: GPIO 0157.
       - Add jumper on JP41 3-4 to connect GPIO 0157 to board LED3
       - Remove jumper on JP31 15-16

- BBLED2: GPIO 0153.
       - Add jumper on JP41 3-4 to connect GPIO 0153 to board LED4
       - Remove jumper on JP31 17-18

Building and Running
********************

This sample can be built and executed on all the boards with the LEDs connected.
The LEDs must be correctly described in the DTS. Each breathing-blinking LED
controller is fixed by chip design to connect to one GPIO. The bbled node must
have its PINCTRL properties set to the correct GPIO.
