# can_b2b_xfer

CAN Board-to-Board Transfer

###########

## Overview
********

A simple sample that can be used with any board with `chosen {zephyr,canbus}` and `alias {SW0}` defined in its dts.
This demo assumes two boards with CAN bus connected with each other. When SW0 is pressed, Node A will transmit on ID 0x321 and wait node B to respond with ID 0x123.

## Building and Running
********************

This application can be built and executed on i.MX 93 EVK as follows:

```sh
west build -b mimx93_evk_a55 ../zephyr_samples/can_b2b_xfer
mkimage SOC=iMX9 REV=A1 flash_simgleboot
dd if=flash.bin of=/dev/sdX bs=1K seek=32 conv=fsync
```

To build for another board, change "mimx93_evk_a55" above to that board's name.

## Sample Output
=============

For node A:

```
*** Booting Zephyr OS build zephyr-v3.3.0-981-gb41519f8ffb8 ***
**** CAN b2b xfer demo ****
Please select local node as A or B via"can_b2b_xfer" shell cmd
Suspending...
zephyr:~$ can_b2b_xfer start a
starting node A
**** CAN b2b xfer demo ****
The selected node role is A
Set up button at gpio@43810000 pin 23
rx filter id: 0
Press button to trigger next transmission
Tx id: 0x321 data:0x00
Rx id: 0x123 data:0x00
Press button to trigger next transmission
Tx id: 0x321 data:0x01
Rx id: 0x123 data:0x01
Press button to trigger next transmission
Tx id: 0x321 data:0x02
Rx id: 0x123 data:0x02
Press button to trigger next transmission
Tx id: 0x321 data:0x03
Rx id: 0x123 data:0x03
Press button to trigger next transmission
Tx id: 0x321 data:0x04
Rx id: 0x123 data:0x04
Press button to trigger next transmission
Tx id: 0x321 data:0x05
Rx id: 0x123 data:0x05
```

For node B:

```
*** Booting Zephyr OS build zephyr-v3.3.0-981-gb41519f8ffb8 ***
**** CAN b2b xfer demo ****
Please select local node as A or B via"can_b2b_xfer" shell cmd
Suspending...
zephyr:~$ can_b2b_xfer start b
starting node B
**** CAN b2b xfer demo ****
The selected node role is B
Set up button at gpio@43810000 pin 23
rx filter id: 0
Rx id:0x321 data:0x00
Wait for node A...
Rx id:0x321 data:0x01
Wait for node A...
Rx id:0x321 data:0x02
Wait for node A...
Rx id:0x321 data:0x03
Wait for node A...
Rx id:0x321 data:0x04
Wait for node A...
Rx id:0x321 data:0x05
Wait for node A...
```
