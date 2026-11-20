.. _video_mcux:

NXP MCUX video capture
######################

The NXP MCUX video drivers support several camera capture interfaces. The
available interface depends on the SoC and board. For example, an i.MX CSI
interface can capture from a parallel camera or from a MIPI CSI-2 receiver:

.. code-block:: text

   +---------------+     +---------------------+     +-----+     +---------------------+
   | Camera sensor | --> | MIPI CSI-2 receiver | --> | CSI | --> | Application buffers |
   +---------------+     +---------------------+     +-----+     +---------------------+
      CSI-2 stream                                         captured frames

The camera sensor produces frames, the MIPI CSI-2 receiver accepts the serial
camera stream, and CSI captures frames into application buffers. The application
processes or forwards the captured frames.

For a parallel camera, CSI, FlexIO camera, or SmartDMA can provide the capture
interface, depending on the platform:

.. code-block:: text

   +---------------+     +-------------------------+     +---------------------+
   | Camera sensor | --> | CSI, FlexIO or SmartDMA | --> | Application buffers |
   +---------------+     +-------------------------+     +---------------------+
     parallel pixels                                      captured frames

JPEG and PNG decoding use separate memory-to-memory devices:

.. code-block:: text

   +------------+     +--------------+     +---------------------+
   | JPEG input | --> | JPEG decoder | --> | Pixel output        |
   +------------+     +--------------+     | (NV12, YUYV, BGR24, |
                                           | YUV24, GREY, ARGB32) |
                                           +---------------------+

   +-----------+     +-------------+     +---------------+
   | PNG input | --> | PNG decoder | --> | RGBA32 output |
   +-----------+     +-------------+     +---------------+

MCUX video devices include:

* CSI captures frames from a parallel camera or a MIPI CSI-2 receiver.
* MIPI CSI-2 receiver accepts a serial camera stream for the CSI capture path.
* FlexIO camera provides a parallel camera interface. It can also generate a
  clock for the sensor. See :dtcompatible:`nxp,flexio-camera`.
* SmartDMA captures camera frames using the SmartDMA peripheral. See
  :dtcompatible:`nxp,video-smartdma`.
* JPEG and PNG decoders convert compressed images into pixel buffers. They are
  memory-to-memory devices and can be used independently of camera capture.

Configuration
*************

* :dtcompatible:`nxp,imx-csi` and :dtcompatible:`nxp,mipi-csi2rx`: configure
  the CSI capture path and its serial receiver when used.
* :dtcompatible:`nxp,flexio-camera`: configure a FlexIO parallel camera path,
  including an optional sensor clock.
* :dtcompatible:`nxp,video-smartdma`: configure a SmartDMA camera capture path.
* :dtcompatible:`nxp,jpegdec` and :dtcompatible:`nxp,pngdec`: configure the
  hardware decoders for compressed images.

Buffer ownership
****************

The application supplies buffers to the capture device. Enqueuing a buffer
transfers ownership to the device; dequeuing a completed buffer returns ownership
to the application, which can consume and requeue it.

For memory-to-memory decoding, the application supplies both compressed input
buffers and decoded output buffers; each is returned after the device finishes
with it.
