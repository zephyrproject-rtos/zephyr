.. _bluetooth-mesh-onoff-level-sample:

Bluetooth: Mesh OnOff & Level Model
###################################

Overview
********

This is a simple application demonstrating a Bluetooth mesh node.

The Node's Root element has following models

- Generic OnOff Server

- Generic OnOff Client

- Generic Level Server

- Generic Level Client 

And Secondary element has following models

- Generic OnOff Server

- Generic OnOff Client

Prior to provisioning, an unprovisioned beacon is broadcast that contains
a unique UUID. It is obtained from the device address set by Nordic in the 
Factory information configuration register (FICR). Each button controls the state of its
corresponding LED and does not initiate any mesh activity.

Associations of Models with hardware
************************************

For the nRF52840-PDK board, these are the model associations:

* LED1 is associated with generic OnOff Server
* Button1 and Button2 are associated with generic OnOff Client: 

  * [Button1 : ON]
  * [Button2: OFF]
* LED3 is associated with generic Level Server:

  * [if (Level < 50%) LED3: OFF else LED3: ON]
* Button3 and Button4 are associated with generic Level Client: 

  * [Button3: publishes Level = 25%]
  * [Button4: publishes Level = 100%]

-------------------------------------------------------------------------------------------------------------------------

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

This sample can be found under :file:`samples/boards/nrf52/mesh/onoff_level_app` in the
Zephyr tree.

The following commands build the application.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf52/mesh/onoff_level_app
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:

Provisioning is done using the BlueZ meshctl utility. 

Here is an example that binds 

- Button1, Button2, and LED1 to application key 1. It then configures Button1 and Button2
  to publish to group 0xC000 and LED1 to subscribe to that group.

- Button3, Button4, and LED3 to application key 1. It then configures Button3 and Button4
  to publish to group 0xC000 and LED3 to subscribe to that group.

.. code-block:: console

   discover-unprovisioned on
   provision <discovered UUID>
   menu config
   target 0100

   appkey-add 1

   bind 0 1 1000               
   bind 0 1 1001
   bind 0 1 1002
   bind 0 1 1003

   sub-add 0100 c000 1000
   sub-add 0100 c000 1002

   pub-set 0100 c000 1 0 5 1001
   pub-set 0100 c000 1 0 5 1003 

The meshctl utility maintains a persistent JSON database containing
the mesh configuration. As additional nodes (boards) are provisioned, it
assigns sequential unicast addresses based on the number of elements
supported by the node. This example supports 2 elements per node.

The first or root element of the node contains models for configuration,
health, and above mentioned models. The secondary elements only
have models for onoff. The meshctl target for configuration must be the
root element's unicast address as it is the only one that has a
configuration server model.

If meshctl is gracefully exited, it can be restarted and reconnected to
network 0x0.

The meshctl utility also supports a onoff model client that can be used to
change the state of any LED that is bound to application key 0x1.
This is done by setting the target to the unicast address of the element
that has that LED's model and issuing the onoff command.
Group addresses are not supported.

This application was derived from the sample mesh skeleton at
:file:`samples/bluetooth/mesh`.

See :ref:`bluetooth setup section <bluetooth_setup>` for details.

