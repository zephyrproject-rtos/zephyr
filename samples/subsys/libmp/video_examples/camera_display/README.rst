.. zephyr:code-sample:: video_examples/camera_display
        :name: Camera Display Example

        A sample pipeline composed of two elements: a camera source and a display sink.

Description
***********

|--[Camera source]--|--->---|--[Display sink]--|

This example demonstrates a pipeline consisting of two elements: source and sink.
The source element, which acts as a capture device, generates video frames and pushes
them to the sink element, which serves as a display device to render the content on screen.

Requirements
************

* A board with input camera support
* A board with output display support
* Sufficient RAM for video buffering

This sample has been tested on the following boards:
* native_sim/native/64
* mimxrt1170_evk@B/mimxrt1176/cm7

Building and Running
********************

For :zephyr:board:`native_sim`, build the sample with the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/libmp/video_examples/camera_display
   :board: native_sim/native/64
   :goals: build
   :snippets: video-sw-generator
   :compact:

For :zephyr:board:`mimxrt1170_evk`, build the sample with the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/libmp/video_examples/camera_display
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :goals: build
   :compact:

Wiring
******

On :zephyr:board:`native_sim`, no wiring is needed since it uses a software video generator
as source, however for the display output you need to ensure that your host system supports
SDL2 to emulate the display.


On :zephyr:board:`mimxrt1170_evk`, the OV5640 camera module should be plugged into the J2
camera connector, the display module RK055HDMIPI4MA0 should be plugged into the J48 MIPI LCD connector.
An additional USB cable is needed to connect USB debug port (J11) on the board to the host PC.


Sample Output
*************

The application will start the video pipeline and display the camera frames on the screen.
Check for any error messages during initialization:

.. code-block:: console

   *** Booting Zephyr OS build ***
   [00:00:00.321,000] <inf> mp_zdisp_sink: Display device: display-controller@40804000
   [00:00:00.373,000] <inf> mp_zvid_buffer_pool: Started streaming
