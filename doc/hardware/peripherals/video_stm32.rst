.. _video_stm32:

STM32 video capture
###################

STM32 camera capture is supported by the DCMI and DCMIPP video drivers, depending
on the SoC. DCMI captures data from a parallel camera interface using DMA.
DCMIPP supports parallel or CSI-2 input, depending on the platform, and provides
hardware processing pipelines, including an image signal processor on supported
pipelines:

.. code-block:: text

   +---------------+     +----------------+     +---------------------+
   | Camera sensor | --> | DCMI or DCMIPP | --> | Application buffers |
   +---------------+     +----------------+     +---------------------+
     parallel or CSI-2 pixels                    captured frames

The camera sensor produces frames. DCMI or DCMIPP captures them into application
buffers, with DCMIPP also providing processing on supported pipelines. The
application processes or forwards the captured frames.

JPEG and H.264 encoding use separate memory-to-memory devices:

.. code-block:: text

   +--------------------+     +------------+     +-------------+
   | NV12 or GREY input | --> | JPEG codec | --> | JPEG output |
   +--------------------+     +------------+     +-------------+

   +----------------------+     +------+     +-------------+
   | NV12 or RGB565 input | --> | VENC | --> | H.264 output |
   +----------------------+     +------+     +-------------+

STM32 video devices include:

* DCMI captures frames from a parallel camera.
* DCMIPP captures frames from a parallel or CSI-2 source and can process them on
  supported pipelines.
* JPEG codec encodes pixel buffers as JPEG.
* VENC encodes pixel buffers as H.264 video.

The JPEG codec and VENC are memory-to-memory devices and can be used independently
of camera capture.

Configuration
*************

* :dtcompatible:`st,stm32-dcmi`: configure parallel capture and optional
  snapshot mode.
* :dtcompatible:`st,stm32-dcmipp`: configure the capture and processing path.
* :zephyr_file:`drivers/video/Kconfig.stm32_dcmipp`: set the source frame
  dimensions and pixel format to match the sensor.
* :dtcompatible:`st,stm32-jpeg`: configure the hardware JPEG codec.
* :dtcompatible:`st,stm32-venc`: configure the hardware H.264 encoder.

Buffer ownership
****************

The application supplies buffers to the capture device. Enqueuing a buffer
transfers ownership to the device; dequeuing a completed buffer returns ownership
to the application, which can consume and requeue it.

For memory-to-memory encoding, the application supplies input and
output buffers; each is returned after the device finishes with it.
