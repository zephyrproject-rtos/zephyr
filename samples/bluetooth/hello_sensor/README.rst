.. _bluetooth_hello_sensor_sample:

Bluetooth: Hello Sensor
#######################

Overview
********

This sample demonstrates a Bluetooth Low Energy (BLE) peripheral device
implementing a custom "Hello Sensor" GATT service. The application advertises
the Hello Sensor service and supports two characteristics:

1. **Notify Characteristic** - A read-only characteristic with Notify and Indicate
   properties that sends periodic notifications on button press.
2. **Blink Characteristic** - A read/write characteristic that controls LED blinking
   when written by a peer client.

The sample is designed to work with boards that have LEDs and buttons defined in
their device trees (e.g., ``user_led_0``, ``user_led_1``, ``user_btn_0``).

Requirements
************

* A board with Bluetooth support and GPIO peripherals for LEDs and buttons
* CYW55513 EVK (or compatible Infineon AIROC device)

Building and Running
********************

Build the sample with the following commands:

.. code-block:: console

   west build -b cyw55513_evk/cyw20829b0lkml samples/bluetooth/hello_sensor

Flash the board:

.. code-block:: console

   west flash

Once running, the application will:

1. Initialize Bluetooth and advertise as "Hello"
2. Expose the Hello Sensor service with UUID ``2F81C7B6-7447-46C2-A4C7-1EA55F2E2838``
3. Listen for button presses to send notifications
4. Process LED blink requests from peer clients

Testing with AIROC Bluetooth Connect App
=========================================

Download the AIROC Bluetooth Connect App for iOS or Android and:

1. Scan for "Hello" device
2. Connect to the device
3. Enable notifications on the Notify characteristic
4. Press the user button to trigger notifications
5. Write values to the Blink characteristic to control LED blinking

Troubleshooting
***************

- If LEDs or buttons don't work, verify that the board has the corresponding
  device tree aliases: ``user_led_0``, ``user_led_1``, ``user_btn_0``
- Check the serial console output for initialization and connection logs
