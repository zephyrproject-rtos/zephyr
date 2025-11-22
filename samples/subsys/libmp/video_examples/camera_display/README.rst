.. zephyr:code-sample:: libmp_camera_display
   :name: Camera Display Example

   A sample pipeline composed of 3 elements: a camera source, a capsfilter and a display sink.

Description
***********

::

    +-----------------+     +--------------+     +----------------+
    |  Camera Source  | --> |  Capsfilter  | --> |  Display Sink  |
    +-----------------+     +--------------+     +----------------+

This example demonstrates a pipeline consisting of 3 elements. The camera source element generates
video frames. The capsfilter is used to enforce a video format and/or resolution and/or framerate.
This element is optional, without it, the pipeline is still working but with the default negotiated
format. The display sink then renders the video frames on the screen.

Requirements
************

* A board with input camera support
* A board with output display support
* Sufficient RAM for video buffering

This sample has been tested on the following boards:

- :zephyr:board:`native_sim`

- :zephyr:board:`mimxrt1170_evk`

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
   :board: mimxrt1170_evk@B/mimxrt1176/cm7
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
