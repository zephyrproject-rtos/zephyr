.. _sensor-attribute:

Sensor Attributes
#################

:dfn:`Attributes`, enumerated in :c:enum:`sensor_attribute`, are immutable and
mutable properties of a sensor and its channels.

Attributes allow for obtaining metadata and changing configuration of a sensor.
Common configuration parameters like channel scale, sampling frequency, adjusting
channel offsets, signal filtering, power modes, on chip buffers, and event
handling options are very common. Attributes provide a flexible API for
inspecting and manipulating such device properties.

Attributes are specified using :c:enum:`sensor_attribute` which can be used with
:c:func:`sensor_attr_get` and :c:func:`sensor_attr_set` to get and set a sensors
attributes.

A quick example...

.. code-block:: c

   const struct device *accel_dev = DEVICE_DT_GET(DT_ALIAS(accel0));
   struct sensor_value accel_sample_rate;
   int rc;

   rc = sensor_attr_get(accel_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &accel_sample_rate);
   if (rc != 0) {
                printk("Failed to get sampling frequency\n");
   }

   printk("Sample rate for accel %p is %d.06%d\n", accel_dev, accel_sample_rate.val1, accel_sample_rate.val2*1000000);

   accel_sample_rate.val1 = 2000;

   rc = sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, accel_sample_rate);
   if (rc != 0) {
                printk("Failed to set sampling frequency\n");
   }
