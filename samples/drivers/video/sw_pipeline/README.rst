.. zephyr:code-sample:: video_sw_pipeline
   :name: Video software pipeline sample
   :relevant-api: video_interface

   Capture video frames from the default camera stream then send video frames over USB.

Overview
********

This sample demonstrates how to use the ``zephyr,video-sw-pipeline`` device in order to process
the video stream from a camera, and send it elsewhere, by sending the video stream through USB.

The devkit acts as a webcam from which the transformed video stream can be studied.

Requirements
************

This sample uses the new USB device stack and requires the USB device
controller ported to the :ref:`udc_api`.

If a camera is not present in the system, :ref:`snippet-video-sw-generator`
can be used to see the result of the conversion on a test pattern.

Building and Running
********************

If a board is equipped with a supported video sensor, and ``zephyr,camera``
node is chosen for the board, it will be used as the video source.
The sample can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/sw_pipeline/
   :board: frdm_mcxn947/mcxn947/cpu0
   :snippets: video-sw-generator
   :goals: build flash
   :compact:

See the :zephyr:code-sample:`uvc` sample for examples of how to access the webcam stream.
