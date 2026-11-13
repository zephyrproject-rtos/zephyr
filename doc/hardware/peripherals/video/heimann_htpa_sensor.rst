.. _video_htpa:

Heimann HTPA infrared cameras
#############################

Overview
********

The Heimann HTPA video driver supports the :dtcompatible:`heimann,htpa-120x84`
and :dtcompatible:`heimann,htpa-160x120` thermopile sensor arrays over SPI.

The driver supports each sensor at its native resolution and provides frames
in the ``VIDEO_PIX_FMT_Y16`` format.

The driver uses an instance-based design that allows a single SoC to operate
multiple HTPA cameras. Different supported camera types, such as a 120 by 84
camera and a 160 by 120 camera, can be instantiated on the same SoC.

Frame acquisition and format
****************************

The supported frame formats are:

* 120 by 84 pixels, with a 240-byte pitch and a 20,160-byte frame size.
* 160 by 120 pixels, with a 320-byte pitch and a 38,400-byte frame size.

Pixels are tightly packed, little-endian 16-bit values in row-major order. For
each frame, the driver reads both sensor halves and an electrical-offset block.
The corresponding electrical offset is subtracted from each sensor sample
before scaling. The output therefore contains processed, frame-relative sensor
values rather than raw sensor words or calibrated temperatures.

Frame scaling
*************

.. note::

   Frame scaling is an interim output method used until temperature conversion
   and the additional compensation steps are implemented. Scaled values must
   not be interpreted as calibrated temperature measurements.

Scaling is recalculated independently for every frame. Consequently, equal
output values in different frames do not necessarily represent equal physical
measurements.

The default histogram mode uses 256 bins, excludes approximately one percent
of the samples at each end of the histogram, maps the remaining range to 0
through 65535, and clamps values outside that range. A frame whose corrected
samples are all equal is output as zeros.

The legacy mode uses the original scan-order-dependent extrema heuristic
instead of calculating the exact minimum and maximum across the complete
frame.

Runtime behavior and limitations
********************************

Sensor acquisition starts during device initialization. The driver uses
**internal multi-buffering**, currently configured as double buffering, so
that it can process one frame while acquiring the next frame from the sensor.
This separates sensor acquisition from processing and output-buffer handling.
Acquisition pauses when all internal buffers are full and resumes as frames are
consumed. The first frames returned after starting or resuming a stream can
therefore have been acquired earlier.

The driver does not currently provide frame-rate controls, temperature
conversion, emissivity adjustment, supply-voltage or sensor-temperature
compensation, per-pixel calibration, or defective-pixel correction. Runtime
SPI failures and conversion timeouts are propagated by returning the affected
application output video buffer with ``bytesused`` set to zero.

Planned processing improvements are:

* Voltage-offset calibration.
* Per-pixel temperature calibration.
* Dead-pixel masking.
* Lens-sensitivity correction.
* Emissivity adjustment.
* Object-temperature calculation.

Devicetree configuration
************************

Each sensor stores its required, sensor-specific calibration values in an
attached flash device. The sensor node must reference this device using the
required ``calibration-flash`` devicetree property. During initialization, the
driver reads the sensor identifier and startup trim values from the flash. The
trim values configure the sensor but are not yet used to convert frames to
calibrated temperatures.

Applications which obtain their video device through the ``zephyr,camera``
chosen node must select the HTPA sensor there.

Configuration options
*********************

Enable the driver with :kconfig:option:`CONFIG_VIDEO_HEIMANN_HTPA`.
Histogram percentile scaling is used by default with
:kconfig:option:`CONFIG_VIDEO_HM_HTPA_AUTOSCALE_HISTOGRAM`. The legacy minimum
and maximum scaling algorithm can instead be selected with
:kconfig:option:`CONFIG_VIDEO_HM_HTPA_AUTOSCALE_LEGACY`.

Using the camera
****************

The :zephyr:board:`htpa_eval` board configures an HTPA camera as its
``zephyr,camera`` chosen device. It can stream the camera output using the
:zephyr:code-sample:`uvc` sample.
