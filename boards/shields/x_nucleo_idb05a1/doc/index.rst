.. _x-nucleo-idb05a1:

X-NUCLEO-IDB05A1: Bluetooth Low Energy expansion board based on the SPBTLE-RF module
####################################################################################

Overview
********
The X-NUCLEO-IDB05A1 is a Bluetooth Low Energy evaluation board based on the
SPBTLE-RF BlueNRG-MS RF module to allow expansion of the STM32 Nucleo boards.
The SPBTLE-RF module is FCC (FCC ID: S9NSPBTLERF) and IC certified
(IC: 8976C-SPBTLERF).

The X-NUCLEO-IDB05A1 is compatible with the ST Morpho and Arduino UNO R3
connector layout (the user can mount the ST Morpho connectors, if required). The
X-NUCLEO-IDB05A1 interfaces with the host microcontroller via the SPI pin, and
the user can change the default SPI clock, the SPI chip select and SPI IRQ by
changing one resistor on the evaluation board.

IMPORTANT NOTICE : Some modification must be done out of the box, please refer
to "Hardware configuration" section.

.. image:: img/x-nucleo-idb05a1.jpg
     :width: 400px
     :height: 350px
     :align: center
     :alt: X-NUCLEO-IDB05A1

More information about the board can be found at the
`X-NUCLEO-IDB05A1 website`_.

Hardware configuration
**********************
To follow Arduino pinout, resistors R2 and R4 must be removed and please connect
instead R6 and R7.

Some boards might not support IRQ pin on Arduino pin A0, in this case resistor
R1 must be removed and please connect instead R8.

Known boards that need this IRQ pin modification are :
 - stm32mp157c_dk2

Hardware
********

X-NUCLEO-IDB05A1 provides a SPBTLE-RF chip with the following key features:
 - Bluetooth Low Energy FCC and IC certified module based on Bluetooth ® SMART
   4.1 network processor BlueNRG-MS
 - Integrated Balun (BALF-NRG-01D3)
 - Chip antenna

More information about X-NUCLEO-IDB05A1 can be found here:
       - `X-NUCLEO-IDB05A1 databrief`_

Programming
***********

To use the X-NUCLEO-IDB05A1 as a Bluetooth Low-Energy controller shield with a
SPI host controller interface (HCI-SPI):

Build your project and activate the presence of the shield for the build.
   You can do so by adding the matching -DSHIELD arg to CMake command

  .. zephyr-app-commands::
     :zephyr-app: your_app
     :board: your_board_name
     :shield: x_nucleo_idb05a1
     :goals: build

Alternatively, it could be set by default in a project's CMakeLists.txt:

.. code-block:: none

	set(SHIELD x_nucleo_idb05a1)

References
**********

.. target-notes::

.. _X-NUCLEO-IDB05A1 website:
   http://www.st.com/en/ecosystems/x-nucleo-idb05a1.html

.. _X-NUCLEO-IDB05A1 databrief:
   https://www.st.com/resource/en/data_brief/x-nucleo-idb05a1.pdf
