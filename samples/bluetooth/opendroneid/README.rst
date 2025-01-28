.. zephyr:code-sample:: bluetooth_opendroneid
   :name: OpenDroneID
   :relevant-api: bluetooth

   Advertise ODID Messages using GAP Broadcaster role.

Overview
********

Open Drone ID messages let's you distinguish between the Unmanned Aerial Vehicles.
It also has UAV's real-time location/altitude, UA serial number, operator ID/location etc.
The message format is defined in the ASTM F3411 Remote ID and the
ASD-STAN prEN 4709-002 Direct Remote ID specifications.

This application demonstrates ODID Message transmission over Bluetooth in GAP Broadcaster role.
Currently this program supports transmitting static drone data via Bluetooth Beacon.
But this could easily be extended to simulate a bit more dynamic flight example.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/opendroneid` in the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.
