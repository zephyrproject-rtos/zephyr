.. zephyr:code-sample:: pulse_io_byte_transfer
   :name: pulse_io byte transfer
   :relevant-api: pulse_io_interface

   Transfer bytes over a pulse_io controller using the codec helpers.

Overview
********

This sample encodes bytes into timed pulses with
:c:func:`pulse_io_encode_bytes`, transmits them on one pulse_io
channel and receives them back on a second channel of the same
controller. The received symbols are decoded with
:c:func:`pulse_io_decode_bytes` and compared against the sent value.

The board overlay routes both channels to the same pad with the input
path enabled, so the signal loops back internally and no external
wiring is needed. Running on another board needs an overlay that
selects the channels and their pins, either sharing one pad or with
the transmit pin wired to the receive pin.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/pulse_io/byte_transfer
   :board: esp32s3_devkitc/esp32s3/procpu
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   pulse_io byte transfer: TX channel 3, RX channel 7
   sent 0x00 received 0x00
   sent 0x01 received 0x01
   sent 0x02 received 0x02
   sent 0x03 received 0x03
   sent 0x04 received 0x04
   byte transfer done
