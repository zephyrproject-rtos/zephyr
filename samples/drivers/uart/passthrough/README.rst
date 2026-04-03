.. zephyr:code-sample:: uart-passthrough
   :name: UART Passthrough
   :relevant-api: uart_interface

   Pass data directly between the console and another UART interface.

Overview
********

This sample will virtually connect two UART interfaces together, as if Zephyr
and the processor were not present. Data read from the console is transmitted
to the "*other*" interface, and data read from the "*other*" interface is
relayed to the console.

The source code for this sample application can be found at:
:zephyr_file:`samples/drivers/uart/passthrough`.

Requirements
************

#. One UART interface, identified as Zephyr's console.
#. A second UART connected to something interesting (e.g: GPS), identified as
   the chosen ``uart,passthrough`` device - the pins and baudrate will need to
   be configured correctly.

Building and Running
********************

Build and flash the sample as follows, changing ``nucleo_l476rg`` for your
board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/passthrough
   :board: nucleo_l476rg
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.5.0-2988-gb84bab36b941 ***
    Console Device: 0x8003940
    Other Device:   0x800392c
    $GNGSA,A,3,31,29,25,26,,,,,,,,,11.15,10.66,3.29,1*06
    $GNGSA,A,3,,,,,,,,,,,,,11.15,10.66,3.29,2*0F
    $GNGSA,A,3,,,,,,,,,,,,,11.15,10.66,3.29,3*0E
    $GNGSA,A,3,,,,,,,,,,,,,11.15,10.66,3.29,4*09
    $GNGSA,A,3,,,,,,,,,,,,,11.15,10.66,3.29,5*08
