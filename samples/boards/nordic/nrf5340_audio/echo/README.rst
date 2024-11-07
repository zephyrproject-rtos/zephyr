.. zephyr:code-sample:: nrf5340_audio-echo
   :name: NRF5340 Audio echo

   Process on-board microphone and forward to headphone to add an echo effect

Overview
********
This sample code is a streamlined version of the larger Nordic audio
application, designed for simplicity and ease of use. It enables basic
functionality for the onboard microphone/line-in and a connected headset.

The code creates a simple echo effect, where audio picked up by the microphone
is directly forwarded to the headset. For user convenience, buttons are
implemented to start and stop the echoing and to control the volume.

Line-in
=======
Line-in is supported; however, standard microphones typically have insufficient
voltage levels and are not detected by the system.

To enable line-in functionality, set `CONFIG_ONBOARD_MIC=n`.

Requirements
************
This sample is designed specifically for the :ref:`nrf5340_audio_dk_nrf5340`
(nrf5340_audio_dk_nrf5340_cpuapp) hardware. It is ready to run out of the box,
requiring no additional or custom overlay files. The application only supports
this hardware configuration.

The I2S codec used is the CS47L63, with drivers provided by Nordic.
The primary audio_i2s interactions were implemented by Packetcraft.


Building and Running
********************
The code can be found in :zephyr_file:`samples/boards/nordic/nrf5340_audio/echo`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/nrf5340_audio/echo
   :board: nrf5340_audio_dk_nrf5340_cpuapp
   :goals: build flash
   :compact:

Press `Play/Pause` button to start echoing. Press `Vol - to`` decrease volume
and use `Vol +` to increase volume.