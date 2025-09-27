.. zephyr:code-sample:: tflite-micro-speech-openamp
   :name: Micro Speech OpenAMP

   Recognize speech commands from audio input received on Cortex-A cores and processed on the HiFi4 DSP of the i.MX8M Plus EVK board using TensorFlow Lite for Microcontrollers with a 20KB neural network.

Overview
********

This sample requires an application running on the Cortex-A cores of the i.MX8M Plus to capture audio and send it to the HiFi4 DSP using OpenAMP. The DSP processes the audio data and performs inference using TensorFlow Lite Micro that detects 2 speech commands ("yes" and "no"), as well as "noise" and "silence".

.. Note::
    This README and sample have been modified from
    `the TensorFlow Hello World sample`_, 
    `the OpenAMP using resource table from Zephyr`_ and
    `the Micro Speech Example from TensorFlow Lite for Microcontrollers`_.

.. _the TensorFlow Hello World sample:
    https://github.com/tensorflow/tflite-micro-arduino-examples/tree/main/examples/hello_world

.. _the OpenAMP using resource table from Zephyr:
    https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/ipc/openamp_rsc_table

.. _the Micro Speech Example from TensorFlow Lite for Microcontrollers:
    https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech
    

Building and Running
********************

DSP Firmware
------------

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

The sample can be built for the :zephyr:board:`imx8mp_evk/mimx8ml8/adsp` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/micro_speech
   :host-os: unix
   :board: imx8mp_evk/mimx8ml8/adsp
   :goals: run
   :compact: 

Linux Application
-----------------

The Linux application is not part of the Zephyr repository. It can be found in the `this repository`_.

.. _this repository:
   https://github.com/thong-phn/linux-app

Sample Output
*************

Linux Application
-----

Simulation with a WAV file as input

.. code-block:: console

    root@imx8mpevk:~# ./send default16.wav
    [L] Using TTY device: /dev/ttyRPMSG0
    [L] Expect audio frames: 500
    [L] Consumer:  Consumer thread started
    [L] Producer:  Producer thread started
    [L] Producer:  End of file reached
    [L] Producer:  Producer stopping
    [L] Consumer:  EOF frame received, stopping
    [L] Consumer:  EOF marker sent to Zephyr
    [L] Consumer:  Consumer thread finished

Real-time Recording

.. code-block:: console

    root@imx8mpevk:~# ./record hw:5,0 /dev/ttyRPMSG0
    [L] Using PCM device: hw:5,0
    [L] Using TTY device: /dev/ttyRPMSG0
    [L] PCM device hw:5,0 configured for 16kHz, S16_LE, Mono
    [L] Consumer:  Consumer thread started
    [L] Producer:  Producer thread started
    ^C
    [L] Ctrl+C detected. Stopping...
    [L] Producer:  Sending EOF to consumer
    [L] Producer:  Producer stopping
    [L] Consumer:  EOF frame received, stopping
    [L] Consumer:  EOF marker sent to Zephyr
    [L] Consumer:  Consumer thread finished
    [L] Application finished.

DSP Firmware
------------

.. code-block:: console

        [00:00:00.697,000] <inf> micro_speech_openamp: Starting Micro Speech OpenAMP application
        [00:00:01.231,000] <inf> micro_speech_openamp: Audio processing thread started
        [00:00:02.321,000] <inf> micro_speech_openamp: Audio processing thread started
        [00:00:03.591,000] <inf> model_runner: Initializing static interpreters
        [00:00:03.941,000] <inf> model_runner: Static interpreters initialized successfully
        [00:00:04.981,000] <inf> model_runner: Detected: yes
        [00:00:06.102,000] <inf> model_runner: Detected: no
        [00:00:07.202,000] <inf> model_runner: Detected: silence
        [00:00:09.102,000] <inf> model_runner: Detected: noise



Training
********
To train your own model for use in this sample, follow the instructions in `this link`_.

.. _this link:
   https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech/train

Limitations
********
The basic model uses an inference audio frame size of 1000 ms. 
As a result, there are some limitations:

#. If two commands are spoken within 1000 ms, the second command may not be detected.
#. If a command lasts longer than 1000 ms, it may be detected as two separate commands.

Potential solution: Retrain the model with a smaller input frame size.
