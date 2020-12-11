.. _bluetooth-mesh-onoff-level-lighting-vnd-sample:

Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
######################################################################
Overview
********
This is a application demonstrating a Bluetooth mesh node in
which Root element has following models

- Generic OnOff Server
- Generic OnOff Client
- Generic Level Server
- Generic Level Client
- Generic Default Transition Time Server
- Generic Default Transition Time Client
- Generic Power OnOff Server
- Generic Power OnOff Setup Server
- Generic Power OnOff Client
- Light Lightness Server
- Light Lightness Setup Server
- Light Lightness Client
- Light CTL Server
- Light CTL Setup Server
- Light CTL Client
- Vendor Model

And Secondary element has following models

- Generic Level Server
- Generic Level Client
- Light CTL Temperature Server

Prior to provisioning, an unprovisioned beacon is broadcast that contains
a unique UUID. It is obtained from the device address set by Nordic in the
Factory information configuration register (FICR).

Associations of Models with hardware
************************************
For the nRF52840-PDK board, these are the model associations:

* LED1 is associated with generic OnOff Server's state which is part of Root element
* LED2 is associated with Vendor Model which is part of Root element
* LED3 is associated with generic Level (ROOT) / Light Lightness Actual value
* LED4 is associated with generic Level (Secondary) / Light CTL Temperature value
* Button1 and Button2 are associated with gen. OnOff Client or Vendor Model which is part of Root element
* Button3 and Button4 are associated with gen. Level Client / Light Lightness Client / Light CTL Client which is part of Root element

States of Servers are bounded as per Bluetooth SIG Mesh Model Specification v1.0

After provisioning, the button clients must
be configured to publish and the LED servers to subscribe.
If a server is provided with a publish address, it will
also publish its relevant status.

Requirements
************
This sample has been tested on the Nordic nRF52840-PDK board, but would
likely also run on the nrf52dk_nrf52832 board.

Building and Running
********************
This sample can be found under :zephyr_file:`samples/boards/nrf/mesh/onoff_level_lighting_vnd_app` in the
Zephyr tree.

The following commands build the application.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/mesh/onoff_level_lighting_vnd_app
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Provisioning is done using the BlueZ meshctl utility. In this example, we'll use meshctl commands to bind:

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

The meshctl target for configuration must be the root element's unicast
address as it is the only one that has a configuration server model. If
meshctl is gracefully exited, it can be restarted and reconnected to
network 0x0.

The meshctl utility also supports a onoff model client that can be used to
change the state of any LED that is bound to application key 0x1.
This is done by setting the target to the unicast address of the element
that has that LED's model and issuing the onoff command.
Group addresses are not supported.

This application was derived from the sample mesh skeleton at
:zephyr_file:`samples/bluetooth/mesh`.

See :ref:`bluetooth samples section <bluetooth-samples>` for details.
