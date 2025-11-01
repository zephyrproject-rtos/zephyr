.. zephyr:code-sample:: video_examples/camera_transform_display
        :name: Camera Transform Display Example

        A sample pipeline composed of three elements: a camera source, a video transform, and a display sink.

Description
***********
|--[Source element]--|-->--[Transform element]--|-->--[Sink element]--|

This example demonstrates a pipeline consisting of three elements: source, transform, and sink.
The source element, which acts as a capture device, generates video frames and pushes them to
the transform element. The transform element then processes these frames and forwards them to
the sink element, which serves as a display device to render the content on screen.

Requirements
************

* A board with input camera support
* A board with output display support
* Sufficient RAM for video buffering

This sample has been tested on the following boards:
* mimxrt1170_evk@B/mimxrt1176/cm7

Building and Running
********************

For :zephyr:board:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/libmp/video_examples/camera_transform_display
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :goals: build flash
   :compact:

Wiring
******
On :zephyr:board:`mimxrt1170_evk`, the OV5640 camera module should be plugged into the J2
camera connector, the display module RK055HDMIPI4MA0 should be plugged into the J48 MIPI LCD connector.
An additional USB cable is needed to connect USB debug port (J11) on the board to the host PC.


Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build ***
   [00:00:00.321,000] <inf> mp_zdisp_sink: Display device: display-controller@40804000
   [00:00:00.373,000] <inf> mp_zvid_buffer_pool: Started streaming
