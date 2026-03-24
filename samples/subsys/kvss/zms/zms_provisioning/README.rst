.. zephyr:code-sample:: zms-provisioning
   :name: ZMS Provisioning Data
   :relevant-api: zms_high_level_api

   Generate ZMS provisioning data at build time and flash it with the application image.

Overview
********

This sample demonstrates how to pre-populate a ZMS partition with data generated at build time.
It uses ``gen_zms_provision_data.py`` to create provisioning records from an input file
(``example_data.txt`` by default).

The provisioning flow supports two modes controlled by
``CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX``:

* ``y``: generate a single merged image (``zephyr_with_provision.hex``)
* ``n``: generate a raw provisioning image (``provisioned_raw.hex``) and a merged image for flashing

In both modes, ``west flash`` is configured to program ``zephyr_with_provision.hex``.

Requirements
************

* ``nrf54h20dk/nrf54h20/cpuapp``

Building and Running
********************

Build the sample for ``nrf54h20dk/nrf54h20/cpuapp``:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/kvss/zms/zms_provisioning
   :board: nrf54h20dk/nrf54h20/cpuapp
   :goals: build
   :compact:

Flash the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/kvss/zms/zms_provisioning
   :board: nrf54h20dk/nrf54h20/cpuapp
   :goals: flash
   :compact:

Configuration
*************

The sample provides this configuration option:

* ``CONFIG_SAMPLE_ZMS_PROVISIONING_MERGE_WITH_APP_HEX``

Set it in ``prj.conf`` or as an extra configuration for your build.

Generated Artifacts
*******************

Depending on configuration, the build produces these files in ``build/zephyr``:

* ``zephyr.hex``: application image
* ``provisioned_raw.hex``: raw provisioning image (when merge option is disabled)
* ``zephyr_with_provision.hex``: image used by ``west flash``
