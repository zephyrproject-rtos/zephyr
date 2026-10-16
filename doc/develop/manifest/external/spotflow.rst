.. _external_module_spotflow:

Spotflow
########

Introduction
************

`Spotflow`_ is an observability platform for embedded devices that collects logs, metrics,
and crash dumps from devices in the field. It provides dashboards, alerts, crash analysis,
vulnerability detection, and over-the-air (OTA) firmware updates.

The `Spotflow Device SDK`_ integrates with Zephyr as a module, uses the native logging and core
dump subsystems, and sends telemetry to the platform over MQTT or BLE.
OTA updates integrate with MCUboot and can also update external MCUs connected to the device.

The SDK is licensed under the Business Source License 1.1 with an additional use grant for
interaction with the Spotflow platform. See the `Spotflow Device SDK license`_ for details.

Usage with Zephyr
*****************

To pull in the SDK as a Zephyr :ref:`module <modules>`, add it as a west project in your
:file:`west.yml` manifest with the following content and run :command:`west update`:

.. code-block:: yaml

   manifest:
     projects:
       - name: spotflow
         url: https://github.com/spotflow-io/device-sdk
         revision: main
         path: modules/lib/spotflow

Create a Spotflow account and obtain an ingest key from the `Spotflow web application`_.
Ensure the device has an Internet connection, then add the following to :file:`prj.conf`,
replacing the device ID with a unique identifier and the ingest key with your own:

.. code-block:: cfg

   CONFIG_LOG=y
   CONFIG_SPOTFLOW=y
   CONFIG_SPOTFLOW_DEVICE_ID="zephyr-device-001"
   CONFIG_SPOTFLOW_INGEST_KEY="<your-ingest-key>"

The SDK sends messages from standard Zephyr logging macros, such as ``LOG_INF()``, to Spotflow.
See the `Spotflow Zephyr quickstart`_ for a complete setup using a sample application and the
`Spotflow documentation`_ for metrics, crash reports, configuration options, and OTA updates.

Reference
*********

.. _Spotflow:
   https://spotflow.io/

.. _Spotflow Device SDK:
   https://github.com/spotflow-io/device-sdk

.. _Spotflow Device SDK license:
   https://github.com/spotflow-io/device-sdk/blob/main/LICENSE.MD

.. _Spotflow web application:
   https://app.spotflow.io/

.. _Spotflow Zephyr quickstart:
   https://docs.spotflow.io/quickstart/zephyr

.. _Spotflow documentation:
   https://docs.spotflow.io/
