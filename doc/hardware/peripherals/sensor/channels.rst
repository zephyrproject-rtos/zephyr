.. _sensor-channel:

Sensor Channels
###############

:dfn:`Channels`, enumerated in :c:enum:`sensor_channel`, are quantities that
a sensor device can measure.

Sensors may have multiple channels, either to represent different axes of
the same physical property (e.g. acceleration); or because they can measure
different properties altogether (ambient temperature, pressure and
humidity). Sensors may have multiple channels of the same measurement type to
enable measuring many readings of perhaps temperature, light intensity, amperage,
voltage, or capacitance for example.

A channel is specified in Zephyr using a :c:struct:`sensor_chan_spec` which is a
pair containing both the channel type (:c:enum:`sensor_channel`) and channel index.
At times only :c:enum:`sensor_channel` is used but this should be considered
historical since the introduction of :c:struct:`sensor_chan_spec` for Zephyr 3.7.
