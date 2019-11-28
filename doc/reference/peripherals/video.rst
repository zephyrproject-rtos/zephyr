.. _video:


Video
#####

The video driver API offers a generic interface to video devices.

Basic Operation
***************

Video Device
============

A video device is the abstraction of a hardware or software video function,
which can produce, process, consume or transform video data. The video API is
designed to offer flexible way to create, handle and combine various video
devices.

Endpoint
========

Each video device can have one or more endpoints. Output endpoints configure
video output function and generate data. Input endpoints configure video input
function and consume data.

Video Buffer
============

A video buffer provides the transport mechanism for the data. There is no
particular requirement on the content. The requirement for the content is
defined by the endpoint format. A video buffer can be queued to a device
endpoint for filling (input ep) or consuming (output ep) operation, once
the operation is achieved, buffer can be dequeued for post-processing,
release or reuse.

Controls
========

A video control is accessed and identified by a CID (control identifier). It
represents a video control property. Different devices will have different
controls available which can be generic, related to a device class or vendor
specific. The set/get control functions provide a generic scalable interface
to handle and create controls.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_VIDEO`

API Reference
*************

.. doxygengroup:: video_interface
   :project: Zephyr

.. doxygengroup:: video_controls
   :project: Zephyr
