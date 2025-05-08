.. zephyr:code-sample:: video-shell
   :name: Video shell
   :relevant-api: video_interface

   Control video devices using Zephyr shell commands.

Description
***********

This sample application uses the Zephyr shell to control video devices and test capturing
a few frames and see the FPS statistics.

Requirements
************

If a video device is present, it will be listed as tab-completion candidates of the ``video``
shell command, and will be usable as argument.

Refer to the :zephyr:code-sample:`video-capture` for how to ensure a video device is present.

Building and Running
********************

For :zephyr:board:`mimxrt1064_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/shell
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114,rk043fn66hs_ctg
   :goals: build
   :compact:

For :zephyr:board:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/shell
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :goals: build
   :compact:

For :zephyr:board:`arduino_nicla_vision`, build this sample application with the following
commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/shell
   :board: arduino_nicla_vision/stm32h747xx/m7
   :goals: build
   :compact:

For :ref:`native_sim`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/shell
   :board: native_sim
   :goals: build
   :compact:

For testing purpose, without the need for any real video capture,
a video software pattern generator is supported by the any board without
specifying the shields.

Sample Output
=============

.. code-block:: console

   uart:~$ video
   video - Video driver commands
   Subcommands:
     start    : Start a video device and its sources
                Usage: video start <device>
     stop     : Stop a video device and its sources
                Usage: video stop <device>
     capture  : Capture a given number of frames from a device
                Usage: video capture <device> <num-frames>
     format   : Query or set the video format of a device
                Usage: video format <device> <ep> [<fourcc> <width>x<height>]
     frmival  : Query or set the video frame rate/interval of a device
                Usage: video frmival <device> <ep> [<n>fps|<n>ms|<n>us]
     ctrl     : Query or set video controls of a device
                Usage: video ctrl <device> [<ctrl> <value>]
   uart:~$

References
**********

.. target-notes::
