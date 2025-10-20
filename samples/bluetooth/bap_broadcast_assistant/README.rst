.. zephyr:code-sample:: bluetooth_bap_broadcast_assistant
   :name: Basic Audio Profile (BAP) Broadcast Audio Assistant
   :relevant-api: bluetooth bt_audio bt_bap bt_conn

   Use BAP Broadcast Assistant functionality.

Overview
********

Application demonstrating the BAP Broadcast Assistant functionality.

The sample will automatically try to connect to a device in the BAP Scan Delegator
role (advertising support for the Broadcast Audio Scan Service (BASS)).
It will then search for a broadcast source and (if found) add the broadcast ID to
the BAP Scan Delegator.

Practical use of this sample requires a sink (e.g. the BAP Broadcast Audio Sink sample or
a set of BAP Broadcast capable earbuds) and a source (e.g. the BAP Broadcast Audio
Source sample).

Check the :zephyr:code-sample-category:`bluetooth` samples for general information.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support
* Broadcast Audio Source and Sink devices

Building and Running
********************

The application will act as a broadcast assistant with optionally preconfigured
filtering of broadcast sink and broadcast source names. By default, the application will
search for and connect to the first broadcast audio sink found (advertising PACS and
BASS UUIDs) and then search for and select the first broadcast audio source found
(advertising a broadcast ID).

Filter these by modifying the following configs:

``CONFIG_SELECT_SINK_NAME``: Substring of BT name of the sink.

and

``CONFIG_SELECT_SOURCE_NAME``: Substring of BT name or broadcast name of the source.

Building for an nrf52840dk
--------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/bap_broadcast_assistant/
   :board: nrf52840dk/nrf52840
   :goals: build
