.. zephyr:code-sample:: video-capture
   :name: Video capture
   :relevant-api: video_interface

   Use the video API to retrieve video frames from a capture device.

Description
***********

This sample application uses the :ref:`video_api` to capture frames from a video capture
device then uses the :ref:`display_api` to display them onto an LCD screen (if any).

Requirements
************

This sample needs a video capture device (e.g. a camera) but it is not mandatory.
Supported boards and camera modules include:

- `Camera iMXRT`_

- :zephyr:board:`mimxrt1064_evk`
  with a `MT9M114 camera module`_

- :zephyr:board:`mimxrt1170_evk`
  with an `OV5640 camera module`_

- :zephyr:board:`frdm_mcxn947`
  with any ``arducam,dvp-20pin-connector`` camera module such as :ref:`dvp_20pin_ov7670`.

- :zephyr:board:`stm32h7b3i_dk`
  with the :ref:`st_b_cams_omv_mb1683` shield and a compatible camera module.

Also :zephyr:board:`arduino_nicla_vision` can be used in this sample as capture device, in that case
The user can transfer the captured frames through on board USB.

Wiring
******

On :zephyr:board:`mimxrt1064_evk`, the MT9M114 camera module should be plugged in the
J35 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J41) in order to get console output via the freelink interface.

On :zephyr:board:`mimxrt1170_evk`, the OV5640 camera module should be plugged into the
J2 camera connector. A USB cable should be connected from a host to the micro
USB debug connector (J11) in order to get console output via the daplink interface.

On :zephyr:board:`stm32h7b3i_dk`, connect the :ref:`st_b_cams_omv_mb1683` shield to the
board on CN7 connector. A USB cable should be connected from a host to the micro USB
connector in order to get console output.

For :zephyr:board:`arduino_nicla_vision` there is no extra wiring required.

Building and Running
********************

For :zephyr:board:`mimxrt1064_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114,rk043fn66hs_ctg
   :goals: build
   :compact:

For :zephyr:board:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :goals: build
   :compact:

For :zephyr:board:`arduino_nicla_vision`, build this sample application with the following
commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: arduino_nicla_vision/stm32h747xx/m7
   :goals: build
   :compact:

For :zephyr:board:`frdm_mcxn947`, build this sample application with the following commands,
using the :ref:`dvp_20pin_ov7670` and :ref:`lcd_par_s035` connected to the board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: dvp_20pin_ov7670,lcd_par_s035_8080
   :goals: build
   :compact:

For :zephyr:board:`stm32h7b3i_dk`, build this sample application with the following commands,
using the :ref:`st_b_cams_omv_mb1683` shield with a compatible camera module:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: stm32h7b3i_dk
   :shield: st_b_cams_omv_mb1683
   :goals: build
   :compact:

For testing purpose and without the need of any real video capture and/or display hardwares,
a video software pattern generator is supported by the above build commands without
specifying the shields, and using :ref:`snippet-video-sw-generator`:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :goals: build
   :compact:

For controlling the camera device using shell commands instead of continuously capturing the data,
append ``-DCONFIG_VIDEO_SHELL=y`` to the build command:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114,rk043fn66hs_ctg
   :gen-args: -DCONFIG_VIDEO_SHELL=y
   :goals: build
   :compact:

For :zephyr:board:`stm32h7b3i_dk` with shell commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: stm32h7b3i_dk
   :shield: st_b_cams_omv_mb1683
   :gen-args: -DCONFIG_VIDEO_SHELL=y
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Video device: csi@402bc000
    - Capabilities:
      RGBP width [480; 480; 0] height [272; 272; 0]
      YUYV width [480; 480; 0] height [272; 272; 0]
      RGBP width [640; 640; 0] height [480; 480; 0]
      YUYV width [640; 640; 0] height [480; 480; 0]
      RGBP width [1280; 1280; 0] height [720; 720; 0]
      YUYV width [1280; 1280; 0] height [720; 720; 0]
    - Default format: RGBP 480x272

    Display device: display-controller@402b8000
    - Capabilities:
      x_resolution = 480, y_resolution = 272, supported_pixel_formats = 40
      current_pixel_format = 32, current_orientation = 0

    Capture started
    Got frame 0! size: 261120; timestamp 249 ms
    Got frame 1! size: 261120; timestamp 282 ms
    Got frame 2! size: 261120; timestamp 316 ms
    Got frame 3! size: 261120; timestamp 350 ms
    Got frame 4! size: 261120; timestamp 384 ms
    Got frame 5! size: 261120; timestamp 418 ms
    Got frame 6! size: 261120; timestamp 451 ms

   <repeats endlessly>

If using the shell, the capture would not start, and instead it is possible to access the shell

.. code-block:: console

   uart:~$ video --help
   video - Video driver commands
   Subcommands:
     start    : Start a video device and its sources
                Usage: start <device>
     stop     : Stop a video device and its sources
                Usage: stop <device>
     capture  : Capture a given number of buffers from a device
                Usage: capture <device> <num-buffers>
     format   : Query or set the video format of a device
                Usage: format <device> <dir> [<fourcc> <width>x<height>]
     frmival  : Query or set the video frame rate/interval of a device
                Usage: frmival <device> [<n>fps|<n>ms|<n>us]
     ctrl     : Query or set video controls of a device
                Usage: ctrl <device> [<ctrl> <value>]
   uart:~$


References
**********

.. target-notes::

.. _Camera iMXRT: https://community.nxp.com/t5/i-MX-RT-Knowledge-Base/Connecting-camera-and-LCD-to-i-MX-RT-EVKs/ta-p/1122183
.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
.. _OV5640 camera module: https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf
