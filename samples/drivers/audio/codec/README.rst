.. zephyr:code-sample:: codec
   :name: audio codec

   mic to speaker loopback.

Overview
********

A simple sample that to demo audio speaker play, and mic to speaker loopback functions.

Building and Running
********************

This application can be built and executed on board as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/audio/codec
   :host-os: unix
   :board: sf32lb52_devkit_lcd
   :goals: run
   :compact:

To build for another board, change "sf32lb52_devkit_lcd" above to that board's name.

Sample Output
=============

.. code-block:: console

    Audio codec sample
    codec device is ready
    codec playback example
    playback started
    codec transfer stopped
    codec loopback example
    loopback started
    loopback stopped
    Exiting
