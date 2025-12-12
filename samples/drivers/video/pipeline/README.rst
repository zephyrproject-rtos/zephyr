.. zephyr:code-sample:: video-pipeline
   :name: Video pipeline

        A video pipeline built on libMP to capture video frames, process them before
        displaying them on a screen

Description
***********

::

    +-----------------+     +--------------+     +-------------------+     +----------------+
    |  Camera Source  | --> |  Caps filter | --> |  Video Transform  | --> |  Display Sink  |
    +-----------------+     +--------------+     +-------------------+     +----------------+

This example demonstrates a video pipeline consisting of maximum 4 elements. The camera source
element generates video frames. The capsfilter is used to enforce a video format and/or resolution
and/or framerate. This element is optional, without it, the pipeline is still working but with the
default negotiated format. The video frames are then processed (e.g. rotated 90 degree) by an
(optional) video transformer and rendered by a display sink element.

Requirements
************

This sample needs a video capture device (e.g. a camera) but it is not mandatory.
Supported boards and camera modules include:

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
   :zephyr-app: samples/drivers/video/pipeline
   :board: mimxrt1064_evk
   :shield: dvp_fpc24_mt9m114,rk043fn66hs_ctg
   :goals: build
   :compact:

For :zephyr:board:`mimxrt1170_evk`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/pipeline
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :shield: nxp_btb44_ov5640,rk055hdmipi4ma0
   :goals: build
   :compact:

For :zephyr:board:`arduino_nicla_vision`, build this sample application with the following
commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/pipeline
   :board: arduino_nicla_vision/stm32h747xx/m7
   :goals: build
   :compact:

For :zephyr:board:`frdm_mcxn947`, build this sample application with the following commands,
using the :ref:`dvp_20pin_ov7670` and :ref:`lcd_par_s035` connected to the board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/pipeline
   :board: frdm_mcxn947/mcxn947/cpu0
   :shield: dvp_20pin_ov7670,lcd_par_s035_8080
   :goals: build
   :compact:

For :zephyr:board:`stm32h7b3i_dk`, build this sample application with the following commands,
using the :ref:`st_b_cams_omv_mb1683` shield with a compatible camera module:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/pipeline
   :board: stm32h7b3i_dk
   :shield: st_b_cams_omv_mb1683
   :goals: build
   :compact:

For testing purpose and without the need of any real video capture a video software pattern
generator is supported by the above build commands without specifying the shields, and using
:ref:`snippet-video-sw-generator`:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/pipeline
   :board: native_sim/native/64
   :snippets: video-sw-generator
   :goals: build
   :compact:

Sample Output
*************

.. code-block:: console

*** Booting Zephyr OS build v4.3.0-rc2-1649-gef3755ee080b ***
[00:00:00.366,000] <inf> mp_zvid_buffer_pool: Started buffer pool
[00:00:00.367,000] <inf> mp_zvid_buffer_pool: Started buffer pool
[00:00:07.128,000] <inf> main: EOS message from element 1

References
**********

.. target-notes::

.. _MT9M114 camera module: https://www.onsemi.com/PowerSolutions/product.do?id=MT9M114
.. _OV5640 camera module: https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf
