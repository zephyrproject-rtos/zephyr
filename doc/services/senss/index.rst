.. _senss_api:

Sensor Subsystem (SENSS)
########################

.. contents::
    :local:
    :depth: 2

Overview
********

Sensor subsystem is a high level sensor framework inside the OS user
space service layer.It's a framework focus on sensor fusion, clients
arbitration, sampling, timing, scheduling and sensor based power management.

It's key concepts including physical sensor and virtual sensor objects,
and a scheduling framework reflects sensor objects' reporting relationship.
Physical sensors not depends on any other sensor objects for input, and
will directly interact with existing zephyr sensor device drivers.
Virtual sensors rely on other sensor objects (physical or virtual) as
report inputs.

The sensor subsystem relies on existing zephyr sensor device APIs or V2
zephyr sensor device APIs (`PR#44098 <https://github.com/zephyrproject-rtos/zephyr/pull/44098>`_).

So it can leverage current existing zephyr sensor device drivers (100+).
And it's configurable, so, for some low cost IoT devices, may not need an
advanced sensor framework, but just need access some simple sensor
devices,can not configure and build this sensor subsystem, but just use
the exiting zephyr sensor device APIs to save memory resources.

Since the sensor subsystem is separated from device driver layer or
kernel space and could support various customizations and sensor
algorithms in user space with virtual sensor concepts. The existing
sensor device driver can focus on low layer device side works, can keep
simple as much as possible, just provide device HW abstraction and
operations etc. This is very good for system stability.

The sensor subsystem is decoupled with any sensor expose/transfer
protocols, the target is to support various up-layer frameworks and
Applications with different sensor expose/transfer protocols,
such as `CHRE <https://github.com/zephyrproject-rtos/chre>`_, HID sensors Applications, MQTT sensor Applications
according different products requirements. Or even support multiple
Applications with different up-layer sensor protocols at the same time
with it's multiple clients support design. For example can support CHRE
and other normal zephyr sensor application (can use HID etc) at
the same time.

Sensor subsystem can help build a unified Zephyr sensing architecture for
cross host OSes support and as well of IoT sensor solutions.

Below is the proposed solution with sensor subsystem:

.. image:: images/senss_solution.png
   :align: center
   :alt: Unified Zephyr sensing architecture.

Configurability
===============

* Reusable and configurable standalone subsystem.
* Based on Zephyr existing low-level Sensor API (reuse 100+ existing sensor device drivers)
* Provide Zephyr high-level Sensor Subsystem API for Applications.
* Separate option CHRE Sensor PAL Implementation module to support CHRE.
* Decoupled with any host link protocols, it's Zephyr Application's role to handle different
  protocols (MQTT, HID or Private, all configurable)

Configure Options
=================
* A: CHRE (optional, need config for Chrome host)
* B: CHRE Sensor PAL Implementation (optional, must if A configured)
* C: Sensor Subsystem (optional, must if B or D configured)
* D: HID (optional, need config for Windows/Linux host)

For simple low-cost sensor devices, can use existing Zephyr sensor driver API directly,  no
need configure A, B, C or D.

Main Features
=============

* Scope
    * Focus on framework for sensor fusion, multiple clients, arbitration, data sampling, timing
      management and scheduling.

* Sensor Abstraction
    * ``Physical sensor``: interact with Zephyr sensor device drivers, focus on data collecting.
    * ``Virtual sensor``: relies on other sensor(s), ``physical`` or ``virtual``, focus on data fusion.

* Data Driven Model
    * ``Polling mode``:  periodical sampling rate
    * ``Interrupt mode``:  data ready, threshold interrupt etc.

* Scheduling
    * single thread main loop for all sensor objects sampling and process.

* Buffer Mode for Batching
* Configurable Via Device Tree

API Design
**********

API Organization
===============

* Sensor Subsystem
    * Sensor Types (:zephyr_file:`include/zephyr/senss/senss_sensor_types.h`)
    * Data Types (:zephyr_file:`include/zephyr/senss/senss_datatypes.h`)
    * Sensor Subsystem API (:zephyr_file:`include/zephyr/senss/senss.h`)
    * Sensor API (:zephyr_file:`subsys/senss/include/senss_sensor.h`)

Below diagram shows the API position and scope:

.. image:: images/senss_api_org.png
   :align: center
   :alt: Sensor subsystem API organization.

``Sensor Subsystem API`` is for Applications. ``Sensor API`` is for development ``sensors``.

Sensor Types And Instance
=========================

Sensor subsystem use ``sensor type`` and ``sensor index`` (support multiple instance of same type) to unique identify a sensor instance.
``Sensor index`` 0 always indicate the default sensor instance of a ``sensor type`.

``Sensor type`` follows the `HID standard sensor types definition <https://usb.org/sites/default/files/hutrr39b_0.pdf>`_.

.. code-block:: c

    /**
     * sensor category light
     */
    #define SENSS_SENSOR_TYPE_LIGHT_AMBIENTLIGHT            0x41
    #define SENSS_SENSOR_TYPE_LIGHT_CONSUMER_INFRARED       0x42

    /**
     * sensor category motion
     */
    #define SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_1D       0x71
    #define SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_2D       0x72
    #define SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D       0x73
    #define SENSS_SENSOR_TYPE_MOTION_GYROMETER_1D           0x74
    #define SENSS_SENSOR_TYPE_MOTION_GYROMETER_2D           0x75
    #define SENSS_SENSOR_TYPE_MOTION_GYROMETER_3D           0x76
    #define SENSS_SENSOR_TYPE_MOTION_MOTION_DETECTOR        0x77
    #define SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER          0x79
    #define SENSS_SENSOR_TYPE_MOTION_GYROMETER              0x7A

    /**
     * sensor category other
     */
    #define SENSS_SENSOR_TYPE_OTHER_CUSTOM                  0xE1

    #define SENSS_SENSOR_TYPE_ALL                           0xFFFF

Sensor Instance Handler
=========================

Clients using a ``senss_sensor_handle_t`` type handler to handle a opened sensor
instance, and all subsequent operations on this sensor instance need use this handler, such as set configurations,
read sensor sample data, etc.

.. code-block:: c
    typedef uint16_t senss_sensor_handle_t;

For a sensor instance, could have two kinds of clients: ``Application clients`` and ``Sensor clients``.

``Application clients`` can use :c:func:`senss_open_sensor` to open a sensor instance and get it's handler.

For ``Sensor clients``, there is no open API for opening a reporter, because the client-report relationship is built at the sensor's registration stage with devicetree.  ``Sensor clients`` can get it's reporters' handlers via :c:func:`senss_sensor_get_reporters`.

Sensor Sample Value
==================================

* Data Structure

  Each sensor sample value data structure defined as a common ``header`` + ``readings[]`` structure.

  .. code-block:: c

      struct senss_sensor_value_xxx {
         struct senss_sensor_value_header header;
         struct data {
            uint32_t timestamp_delta;
            union {
                          ...
            };
         } readings[1];
      };

  The ``header`` definition:

  .. code-block:: c

      struct senss_sensor_value_header {
         /** base timestamp of this data readings, unit is micro seconds */
         uint64_t base_timestamp;
         /** count of this data readings */
         uint16_t reading_count;
      };


* Time Stamp

  Time stamp unit in sensor subsystem is ``micro seconds``.

  The ``header`` defined a **base_timestamp**, and each element in **readings[]** array defined **timestamp_delta**.

  Here use **base_timestamp** (``uint64_t``) and **timestampe_delta** (``uint32_t``) to
  save memory usage in batching mode.

  The **base_timestamp** is for ``readings[0]``, the **timestamp_delta** is relation
  to the previous readings.

  For example:

    * timestamp of ``readings[0]`` is ``header.base_timestamp`` + ``readings[0].timestamp_delta``.

    * timestamp of ``readings[1]`` is ``timestamp of readings[0]`` + ``readings[1].timestamp_delta``.

  Since timestamp unit is micro seconds, the max **timestamp_delta** (``uint32_t``) is ``4295`` seconds.

  If a sensor has batched data where two consecutive readings differ by more than ``4295`` seconds, the sensor subsystem runtime will split them across multiple instances of the readings structure, and send multiple events.

  This concept is referred from `CHRE <https://github.com/zephyrproject-rtos/chre/blob/zephyr/chre_api/include/chre_api/chre/sensor_types.h>`_.

* Data Unit

  Sensor subsystem will use scaled fixed point data structure for all sensor values,
  aligned the HID spec, using the format ``v*10^x`` to present the decimal value,
  where the ``v`` is integer number, either ``int8/uint8``, ``int16/uint6``, or ``int32/uint32``, depends on
  required sensor data precision.

  The scale unit exponent x is ``int8`` type with encoding meanings (page 68 of
  `HID spec <https://usb.org/sites/default/files/hutrr39b_0.pdf>`_):

  .. list-table:: Encoding Table
     :widths: 50 50
     :header-rows: 1

     * - Unit Exponet argument
       - Power of Ten (Scientific Notation)
     * - 0x00
       - 1 * 10E0
     * - 0x01
       - 1 * 10E1
     * - 0x02
       - 1 * 10E2
     * - 0x03
       - 1 * 10E3
     * - 0x04
       - 1 * 10E4
     * - 0x05
       - 1 * 10E5
     * - 0x06
       - 1 * 10E6
     * - 0x07
       - 1 * 10E7
     * - 0x08
       - 1 * 10E-8
     * - 0x09
       - 1 * 10E-7
     * - 0x0A
       - 1 * 10E-6
     * - 0x0B
       - 1 * 10E-5
     * - 0x0C
       - 1 * 10E-4
     * - 0x0D
       - 1 * 10E-3
     * - 0x0E
       - 1 * 10E-2
     * - 0x0F
       - 1 * 10E-1

  So, we can have below data present ranges:

  .. list-table:: Ranges Table
     :widths: 50 50
     :header-rows: 1

     * - Type of V
       - Range
     * - int8
       - [-128, 127] *10^[-8, 7]
     * - uint8
       - [0,  255] * 10^[-8, 7]
     * - int16
       - [-32768, 32767] * 10^[-8, 7]
     * - uint16
       - [0,  65535] * 10^[-8, 7]
     * - int32
       - [-2147483648,  2147483647] * 10^[-8, 7]
     * - uint32
       - [0,  4294967295] * 10^[-8, 7]
     * - int64
       - [-9223372036854775808,  9223372036854775807] * 10^[-8, 7]
     * - uint64
       - [0,  18446744073709551615] * 10^[-8, 7]

  To simple the data structure definition and save store memory, only keep `v` in code definitions,
  scale exponent `x` will defined in doc and spec,  but not explicitly present in code, for scenarios
  which need transfer to decimal value, such as in a algorithm process, need base on the sensor
  type and according the doc/spec to get the right scale exponent value `x`.

  An example in doc and spec can be like:

  .. list-table:: 3D Accelerometer
     :widths: 30 25 30 30 30 50
     :header-rows: 1

     * - Data Fields
       - Type
       - Unit
       - Unit Exponent
       - Typical Range
       - Description
     * - data[0]
       - int32
       - micro g
       - -6
       - +/-4*10^6
       - x axis acceleration
     * - data[1]
       - int32
       - micro g
       - -6
       - +/-4*10^6
       - y axis acceleration
     * - data[2]
       - int32
       - micro g
       - -6
       - +/-4*10^6
       - z axis acceleration |


  .. list-table:: Ambient Light
     :widths: 30 25 30 30 30 50
     :header-rows: 1

     * - Data Fields
       - Type
       - Unit
       - Unit Exponent
       - Typical Range
       - Description
     * - data[0]
       - uint32
       - milli lux
       - -3
       - [0, 10000] *10^3
       - Ambient light lux level

  The complete doc/spec should describe all supported sensors like above example.


Device Tree Configuration
*************************

Sensor subsystem using device tree to configuration all sensor instances and their properties,
reporting relationships.

Below is an example:

.. code-block:: devicetree

   /*
    * Copyright (c) 2022 Intel Corporation
    *
    * SPDX-License-Identifier: Apache-2.0
    *
    * Default device tree for sensor subsystem.
    */

   / {
      senss: senss-node {
         compatible = "zephyr,senss";
         status = "okay";
         label = "senss";

         motion_detector: motion-detector {
            compatible = "senss,motion-detector";
            status = "okay";
            sensor-type = <0x77>;
            sensor-index = <0>;
            label = "senss_sensor_motion_detector";
            vendor = "VND";
            model = "Test";
            friendly-name = "Motion Detector Sensor";
            reporters = <&lid_accel>;
            minimal-interval = <100000>;
         };

         hinge_angle: hinge-angle {
            compatible = "senss,hinge-angle";
            status = "okay";
            sensor-type = <0x20B>;
            sensor-index = <0>;
            label = "senss_sensor_hinge_angle";
            vendor = "VND";
            model = "Test";
            friendly-name = "Hinge Angle Sensor";
            reporters = <&base_accel &lid_accel &motion_detector>;
            minimal-interval = <10000>;
         };

         base_accel: base-accel {
            compatible = "senss,phy-accel";
            status = "okay";
            sensor-type = <0x73>;
            sensor-index = <0>;
            label = "senss_base_acccelerometer";
            vendor = "VND";
            model = "Test";
            friendly-name = "Base Accelerometer Sensor";
            underlying-device = <&bmi_spi>;
            minimal-interval = <10000>;
            rotation-matrix = <0 1 0
                     1 0 0
                     0 0 (-1)>;
         };

         lid_accel: lid-accel {
            compatible = "senss,phy-accel";
            status = "okay";
            sensor-type = <0x73>;
            sensor-index = <1>;
            label = "senss_lid_acccelerometer";
            vendor = "VND";
            model = "Test";
            friendly-name = "Lid Accelerometer Sensor";
            underlying-device = <&bmi_i2c>;
            minimal-interval = <10000>;
            rotation-matrix = <0 1 0
                     0 1 0
                     0 0 (-1)>;
         };
      };
   };

:zephyr_file:`dts/common/senss.dtsi`

:zephyr_file:`dts/bindings/sensor/zephyr,senss.yaml`

:zephyr_file:`dts/bindings/sensor/senss,phy-accel.yaml`

:zephyr_file:`dts/bindings/sensor/senss,motion-detector.yaml`

:zephyr_file:`dts/bindings/sensor/senss,hinge-angle.yaml`

Examples
*************************

Application Example
========================

:zephyr_file:`samples/subsys/senss/src/main.c`

.. code-block:: c

   /*
    * Copyright (c) 2022 Intel Corporation.
    *
    * SPDX-License-Identifier: Apache-2.0
    */
   #include <zephyr.h>
   #include <senss/senss.h>
   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>

   static int data_event_callback(
         senss_sensor_handle_t handle,
         void *buf, int size, void *param)
   {
      const struct senss_sensor_info *info = senss_get_sensor_info(handle);

      printf("Received sensor data from sensor: %s, type: 0x%x, sensor index: %d\n",
         info->name,
         info->type,
         info->sensor_index
      );

      if (info->type == SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D) {
         struct senss_sensor_value_3d_int32* value = buf;
         printf("Received sensor data readings : %d\n",
            value->header.reading_count);
         for (int i = 0; i < value->header.reading_count; i++) {
            printf("Accel sensor data : x: %d, y: %d, z: %d\n",
               value->readings[i].x,
               value->readings[i].y,
               value->readings[i].z);
         }
      } else if (info->type == SENSS_SENSOR_TYPE_MOTION_HINGE_ANGLE) {
         struct senss_sensor_value_int32* value = buf;
         printf("Received sensor data readings : %d\n",
            value->header.reading_count);
         for (int i = 0; i < value->header.reading_count; i++) {
            printf("hinge sensor data : %d\n",
               value->readings[i].v);
         }
      }

      return 0;
   }

   void main(void)
   {
      int ret;
      ret = senss_init();
      if (ret != 0 ) {
         printf("Failed to init sensor subsystem! \n");
         return;
      }

      /* Enumerate all sensor instances */
      int sensor_count;
      sensor_count = senss_get_sensor_count(SENSS_SENSOR_TYPE_ALL, -1);

      if (sensor_count > 0 ) {
         struct senss_sensor_info *sensor_info =
            malloc(sensor_count * sizeof(struct senss_sensor_info));

         ret = senss_find_sensor(SENSS_SENSOR_TYPE_ALL, -1,
            sensor_info, sensor_count);
         if (ret != 0) {
            for ( int i = 0; i < ret; i++ ) {
               printf("Find sensor: name : %s, type: 0x%x\n",
                  sensor_info[i].name,
                  sensor_info[i].type);
            }
         }
      }

      /* Open accelerometer instance with sensor index 0 and receive streaming data */
      senss_sensor_handle_t acc_handle;
      ret = senss_open_sensor(SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
         0, &acc_handle);
      if (!ret) {
         senss_register_data_event_callback(acc_handle,
            data_event_callback, NULL);

         /* Set 100 Hz frequency */
         senss_set_interval(acc_handle, 10 * USEC_PER_MSEC);

         /* Set streaming mode */
         senss_set_sensitivity(acc_handle, -1, 0);
      }

      senss_sensor_handle_t hinge_handle;
      ret = senss_open_sensor(SENSS_SENSOR_TYPE_MOTION_HINGE_ANGLE,
         0, &hinge_handle);
      if (!ret) {
         senss_register_data_event_callback(hinge_handle,
            data_event_callback, NULL);

         /* Set 100 Hz frequency */
         senss_set_interval(hinge_handle, 10 * USEC_PER_MSEC);

         /* Set streaming mode */
         senss_set_sensitivity(hinge_handle, -1, 0);
      }

      while (true) {
         k_sleep(K_SECONDS(10));
      }

   }

Sensors Examples
================

* Physical Sensor

  .. code-block:: c

     /*
      * Copyright (c) 2022 Intel Corporation.
      *
      * SPDX-License-Identifier: Apache-2.0
      */

     #include <senss_sensor.h>
     #include <zephyr/drivers/sensor.h>
     #include <stdio.h>

     static struct senss_sensor_register_info acc_reg = {
         .flags = SENSS_SENSOR_FLAG_REPORT_ON_CHANGE,
         .sample_size = sizeof(struct senss_sensor_value_3d_int32),
         .version.value = SENSS_SENSOR_VERSION(0, 8, 0, 0),
     };

     struct acc_context {
         struct senss_sensor_value_3d_int32 value;
         const struct device *underlying_dev;
     };

     static struct acc_context base_acc_ctx = {
     	.underlying_dev = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(base_accel), underlying_device))
     };

     static struct acc_context lid_acc_ctx = {
     	.underlying_dev = DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(lid_accel), underlying_device))
     };

     int acc_init(const struct device *dev, const struct senss_sensor_info *info,
         const senss_sensor_handle_t *reporter_handles, int reporters_count)
     {
         printf("Sensor instance name: %s\n", dev->name);

         /* reporter_handles should be NULL,  and reporters_count should be zero */

         struct acc_context *ctx = senss_sensor_get_ctx_data(dev);
         printf("Underlying device: %s\n", ctx->underlying_dev->name);

         return 0;
     }

     int acc_read_sample(const struct device *dev, void *buf, int size)
     {
         printf("Sensor instance name: %s\n", dev->name);

         struct sensor_value accel[3];

         struct acc_context *ctx = senss_sensor_get_ctx_data(dev);

         printf("Fetch: %s\n", ctx->underlying_dev->name);
         sensor_sample_fetch(ctx->underlying_dev);
         sensor_channel_get(ctx->underlying_dev, SENSOR_CHAN_ACCEL_XYZ, accel);

         /* Just for demo,  not checked buf size */
         struct senss_sensor_value_3d_int32 *v3d = buf;
         v3d->header.reading_count = 1;
         v3d->readings[0].x = accel[0].val1 * 1000000 + accel[0].val2;
         v3d->readings[0].y = accel[1].val1 * 1000000 + accel[1].val2;
         v3d->readings[0].z = accel[2].val1 * 1000000 + accel[2].val2;

         printf("Sample data :\n");
         printf("\t x: %d, y: %d, z: %d\n", v3d->readings[0].x,
               v3d->readings[0].y, v3d->readings[0].z);

         return 0;
     }

     static struct senss_sensor_api acc_api = {
         .init = acc_init,
         .get_interval = acc_get_interval,
         .set_interval = acc_set_interval,
         .read_sample = acc_read_sample,
     };


     SENSS_SENSOR_DT_DEFINE(DT_NODELABEL(base_accel), &acc_reg, &base_acc_ctx, &acc_api);

     SENSS_SENSOR_DT_DEFINE(DT_NODELABEL(lid_accel), &acc_reg, &lid_acc_ctx, &acc_api);

* Virtual Sensor

  .. code-block:: c

     /*
      * Copyright (c) 2022 Intel Corporation.
      *
      * SPDX-License-Identifier: Apache-2.0
      */

     #include <senss_sensor.h>
     #include <stdio.h>

     static struct senss_sensor_register_info hinge_reg = {
         .flags = SENSS_SENSOR_FLAG_REPORT_ON_CHANGE,
         .sample_size = sizeof(struct senss_sensor_value_int32),
         .version.value = SENSS_SENSOR_VERSION(1, 0, 0, 0),
     };

     struct hinge_angle_context {
         struct senss_sensor_value_3d_int32 base_acc_value;
         struct senss_sensor_value_3d_int32 lid_acc_value;
         struct senss_sensor_value_int32 md_value;
         struct senss_sensor_value_int32 hinge_value;

         senss_sensor_handle_t base_acc_handle;
         senss_sensor_handle_t lid_acc_handle;
         senss_sensor_handle_t md_handle;
     };

     static struct hinge_angle_context hinge_ctx;

     int hinge_init(const struct device *dev, const struct senss_sensor_info *info,
         const senss_sensor_handle_t *reporter_handles, int reporters_count)
     {
         printf("Sensor instance name: %s\n", dev->name);

         printf("reporters : %d \n", reporters_count);

         struct hinge_angle_context *ctx = senss_sensor_get_ctx_data(dev);

         for (int i = 0; i < reporters_count; i++) {
             const struct senss_sensor_info * reporter_info =
                senss_get_sensor_info(reporter_handles[i]);

             printf("\t reporter[%d] handle: %d\n", i, reporter_handles[i]);
             printf("\t\t name: %s\n", reporter_info->name);
             printf("\t\t type: 0x%x\n", reporter_info->type);
             printf("\t\t sensor index: %d\n", reporter_info->sensor_index);

             if (reporter_info->type == SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D) {
                 /* According devicetree,  base acc's sensor index is 0 */
                 if (reporter_info->sensor_index == 0) {
                    ctx->base_acc_handle = reporter_handles[i];
                 }

                 /* According devicetree,  lid acc's sensor index is 1 */
                 if (reporter_info->sensor_index == 1) {
                    ctx->lid_acc_handle = reporter_handles[i];
                 }
             }

             if (reporter_info->type == SENSS_SENSOR_TYPE_MOTION_MOTION_DETECTOR) {
                 ctx->md_handle = reporter_handles[i];
             }
         }

         return 0;
     }

     int hinge_process(const struct device *dev, senss_sensor_handle_t reporter, void *buf, int size)
     {
         printf("Sensor instance name: %s\n", dev->name);

         struct hinge_angle_context *ctx = senss_sensor_get_ctx_data(dev);

         if (reporter == ctx->base_acc_handle ) {
             printf("Received base acc's data:\n");
             struct senss_sensor_value_3d_int32 *base_acc_v3d = buf;

             printf("\t x: %d, y: %d, z: %d\n", base_acc_v3d->readings[0].x,
                   base_acc_v3d->readings[0].y, base_acc_v3d->readings[0].z);

             /* Store to context */
             ctx->base_acc_value = *base_acc_v3d;
         }

         if (reporter == ctx->lid_acc_handle ) {
             printf("Received lid acc's data:\n");
             struct senss_sensor_value_3d_int32 *lid_acc_v3d = buf;

             printf("\t x: %d, y: %d, z: %d\n", lid_acc_v3d->readings[0].x,
                   lid_acc_v3d->readings[0].y, lid_acc_v3d->readings[0].z);

             /* Store to context */
             ctx->lid_acc_value = *lid_acc_v3d;

             /* Compute hinge angle data and report to senss runtime, here used dummy data  */
             ctx->hinge_value.header.reading_count = 1;
             ctx->hinge_value.readings[0].v = 87;

             senss_sensor_post_data(dev, &ctx->hinge_value, sizeof(ctx->hinge_value));
         }

         if (reporter == ctx->md_handle ) {
             printf("Received motion detector's data:\n");
             struct senss_sensor_value_int32 * md_data = buf;

             printf("\t value: %d\n", md_data->readings[0].v);

             /* Store to context */
             ctx->md_value = *md_data;
         }

         return 0;
     }

     static struct senss_sensor_api hinge_api = {
         .init = hinge_init,
         .get_interval = hinge_get_interval,
         .set_interval = hinge_set_interval,
         .process = hinge_process,
     };

     SENSS_SENSOR_DT_DEFINE(DT_NODELABEL(hinge_angle), &hinge_reg, &hinge_ctx, &hinge_api);


.. doxygengroup:: senss
