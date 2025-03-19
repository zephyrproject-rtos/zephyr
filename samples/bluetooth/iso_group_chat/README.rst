Overview
********

This folder contains three sample applications that demonstrate the Bluetooth
Low Energy Isochronous Broadcast functionality which has a custom and 
experimental enhancement that allows for multiple Broadcaster devices to transmit 
packets in the same BIG created by a device with a role called 'primary' which 
has 2 or more BISes.

The primary device will transmit packets in BIS Index 1 and the rest of the BISes
will not be sent to allow for secondary devices to transmit packets instead at
the same time slots and repeated as per the RTN.

There is another role called 'mixer' which listens to all the BISes and processes
and dsiplays the data.

Finally the secondary devices will transmit packets in the BISes other than BIS
Index 1 if it has data to send. This allows multiple secondary devices to transmit
data at the same time slots and repeated as per the RTN. If there is collision then
inband rejection (as per FM radio) will mean that if at the mixer one of the packets
has a clear 3db rssi advantage over the other packets then the packet with the
lower rssi will be rejected. For this project, the secondary device will always 
receive on BIS1 and transmit on BIS2.

This caters for the LE Audio use case where a Broadcaster source can transmit audio
on BIS index 1 and secondary devices can transmit on all other BISes (chosen using
a scheme not specified here) and the mixer device can receive all the BISes and
decode and render the audio. This means for example, the audience at a seminar can
listen to the speaker on BIS index 1 and ask questions on the other BISes and 
audio will rendered out of a PA system.

Therefore this folder has three subfolders called primary, secondary and mixer
which implies that at least 3 boards are required.

In these samples the data sent is not audio but counter values and id values that
are printed which can be used to verify that the data is being transmitted and
received correctly.

BLE Controller Stack Changes
****************************

The BLE Controller stack needs to be modified to support the custom enhancement 
for multiple Receive Sync devices to transmit packets in the same BIG created by a
device with a role called 'primary' which has 2 or more BISes.

The change to the BLE Controller stack has to be in the area of the BIG Sync 
so that when a BIG is synced, any BIS can be marked as either send or recieve
and will keep the BIG timing and structutre. Currently the specification only
allows for all BISes in a broadcast sink to be receive.

At the time of writing this, attempts to mark the stream direction as send results
in the sample app failing to sync.

Sample App - Secondary
***********************

The secondary sample app is a simple application that sends packets in BISes other
than BIS Index 1. 
At the time of writing this the author failed to tweaked the Zephyr Bluetooth Controller
stack to allow for the secondary device to transmit packets in the same BIG created by
a device with a role called 'primary' which has 2 or more BISes.

When the stack has been appropriately modified, this app needs to send data in BIS2.
This can be enabled by adding "#define ENABLE_SEND_ON_BIS2" at the beginning of the
main.c file.

New CONFIG options
******************

GROUPCHAT_COMMON
GROUPCHAT_PRIMARY
GROUPCHAT_MIXER
GROUPCHAT_SECONDARY

These options are used to enable the common code specific to the groupchat
feature. This means that the code specific to the primary, mixer and secondary 
devices can be compiled into the project by enabling the respective options and for 
all other bluetooth sample apps the tweaks related to the groupchat feature devices 
will not be compiled into those project.

Requirements
************

* 3 x Nordic nRF2840 devkits.
* Firmware is built with a tweaked Zephyr Bluetooth Controller stack.

Testing
*******

All three sample apps must be tested together and once a BIG is complete, the
Primary device will print details of all BISes transmited every 100th frame.

In the Mixer once a BIG is synced every there will be details of all BISes printed
every 50th frame.

Similarly, in the Secondary device, every 50th frame the data sent will be printed.

* Build and flash the Primary sample app into a nrf52840dk_nrf52840 board.
* Build and flash the Mixer sample app into a nrf52840dk_nrf52840 board.
* Build and flash the Secondary sample app into a nrf52840dk_nrf52840 board.

Expected Primary board log output::
    
    P(2)> 1:[1,1]
    P(2)> 1:[1,101]
    P(2)> 1:[1,201]
    P(2)> 1:[1,301]
    P(2)> 1:[1,401]
    P(2)> 1:[1,501]

where in "P(2)>" the P means that this is running on the Primary device and 
the "(2)" means that the primary created a BIG with 2 BISes and the ">" means
that the packet was transmitted.
In "1:[1,401]" the "1:" means that this is BIS1 data and "[1,401]" means
that the payload is marked with a source_id of 1 which means'primary and
the 401 is the counter value.

Expected Mixer board log output when there are no Secondary boards::
        
    M< 1:[1,1,17936,0] 2:[-,-,-,50]
    M< 1:[1,1,17986,0] 2:[-,-,-,50]
    M< 1:[1,1,18036,0] 2:[-,-,-,50]
    M< 1:[1,1,18086,0] 2:[-,-,-,50]
    M< 1:[1,1,18136,0] 2:[-,-,-,50]
    M< 1:[1,1,18186,0] 2:[-,-,-,50]

where in "M<" the M means that this is running on the Mixer device and the "<"
means that all the packet are received.

Where in "1:[1,1,17936,0]" the "1:" means that this is BIS1 data and the payload 
contains "[1,1,17936,0]" where the first 1 means that it was sent in BIS1 and the 
second 1 is source_id  which means that the payload is from the primary, the 
17936 is the counter value sent by the primary and the 0 means that there were 
no packets lost

Where the 50 in "2:[-,-,-,50]" means that for this frame there was no valid
BIS2 received packet and the 50 implies that there were in total 50 BIS2 packets 
lost since the last printed frame.

Expected Mixer board log output when there is a Secondary board::
            
    M< 1:[1,1,17936,0] 2:[2,10,1234,0]
    M< 1:[1,1,17986,0] 2:[2,10,1284,0]
    M< 1:[1,1,18036,0] 2:[2,10,1334,0]
    M< 1:[1,1,18086,0] 2:[2,10,1384,0]
    M< 1:[1,1,18136,0] 2:[2,10,1434,0]
    M< 1:[1,1,18186,0] 2:[2,10,1484,0]

where "2,10,1234,0" means Secondary device trasmitted in BIS2 and 10 is the 
source_id hence from a secondary device, the counter value is 1234 and the 
final 0 means no packets lost on BIS2