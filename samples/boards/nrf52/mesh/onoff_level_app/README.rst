.. _bluetooth-mesh-onoff-sample:

Bluetooth: Mesh OnOff & Level Model
###########################

Overview
********

This is a simple application demonstrating a Bluetooth mesh node.
Root element has a Generic On Off Server, Generic On Off Client,
Generic Level Server, Generic Level Client models.

Prior to provisioning, an unprovisioned beacon is broadcast that contains
a unique UUID. It is obtained from the device address set by Nordic in the
FICR. Each button controls the state of its
corresponding LED and does not initiate any mesh activity.

In case of nRF52840-PDK Board,
 
	LED1 is associated with Gen. OnOFF Server
	Button1 & Button2 are associated with Gen. OnOFF Client [Button1 : ON & Button 2: OFF]
	LED1 is associated with Gen. Level Server [if (Level < 50) LED3: OFF ...if (Level >= 50) LED3: ON]
	Button3 & Button4 are associated Gen. Level Client [Button3: publishes Level = 25 .....Button4: publishes Level = 100]

After provisioning, the button clients must
be configured to publish and the LED servers to subscribe.

If a LED server is provided with a publish address, it will
also publish its status on an onoff state change.

Requirements
************

This sample has been tested on the Nordic nRF52840-PDK board, but would
likely also run on the nrf52_pca10040 board.

Building and Running
********************

This sample can be found under :file:`samples/boards/nrf52/mesh/onoff-app` in the
Zephyr tree.

The following commands build the application.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf52/mesh/onoff-app
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:

Prior to provisioning, each button controls its corresponding LED as one
would expect with an actual switch.

Provisioning is done using the BlueZ meshctl utility. Below is an example that
binds button 1&2 and LED 1 to application key 1. It then configures button 1&2
to publish to group 0xc000 and LED 1 to subscribe to that group.

.. code-block:: console

   discover-unprovisioned on
   provision <discovered UUID>
   menu config
   target 0100
   appkey-add 1
   bind 0 1 1000                # bind appkey 1 to LED server on element 0 (unicast 0100)
   sub-add 0100 c000 1000       # add subscription to group address c000 to the LED server
   bind 1 1 1001                # bind appkey 1 to button 2 on element 1 (unicast 0101)
   pub-set 0101 c000 1 0 0 1001 # publish button 2 to group address c000

The meshctl utility maintains a persistent JSON database containing
the mesh configuration. As additional nodes (boards) are provisioned, it
assigns sequential unicast addresses based on the number of elements
supported by the node. This example supports 4 elements per node.

The first or root element of the node contains models for configuration,
health, and onoff. The secondary elements only
have models for onoff. The meshctl target for configuration must be the
root element's unicast address as it is the only one that has a
configuration server model.

If meshctl is gracefully exited, it can be restarted and reconnected to
network 0x0. The board configuration is volatile and if the board is reset,
power cycled, or reprogrammed, it will have to be provisioned and configured
again.

The meshctl utility also supports a onoff model client that can be used to
change the state of any LED that is bound to application key 0x1.
This is done by setting the target to the unicast address of the element
that has that LED's model and issuing the onoff command.
Group addresses are not supported.

This application was derived from the sample mesh skeleton at
:file:`samples/bluetooth/mesh`.

See :ref:`bluetooth setup section <bluetooth_setup>` for details.
