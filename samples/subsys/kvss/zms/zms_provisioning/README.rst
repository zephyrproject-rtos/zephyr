.. zephyr:code-sample:: zms-provisioning
   :name: ZMS Provisioning Data
   :relevant-api: zms_high_level_api

   Generate ZMS provisioning data at build time and flash it with the application image.

Overview
********

This sample demonstrates how to pre-populate a ZMS partition with data generated at build time.
It uses ``gen_zms_provision_data.py`` to create provisioning records from an input file
(``example_data.txt`` by default).

The provisioning script generates a standalone provisioning-only image, ``provisioned_raw.hex``
(no application code), which is then merged with ``zephyr.hex`` into ``zephyr_with_provision.hex``.
``west flash`` is configured to program that merged image, so a single ``west flash`` call
programs both the application and the provisioning data.
``provisioned_raw.hex`` remains available as a separate artifact, e.g. to flash or inspect the
provisioning data independently of the application.

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

The provisioning data and layout can be customized with these CMake cache variables,
set with ``-D<variable>=<value>`` on the ``west build`` command line:

* ``ZMS_PROVISION_DATA_FILE`` (default: ``example_data.txt``): input file with ID/data pairs
* ``ZMS_PROVISION_DATA`` (default: empty): inline ID/data pairs, semicolon-separated
  (e.g. ``-DZMS_PROVISION_DATA="1:6162;2:63646566"``)
* ``ZMS_SECTOR_SIZE`` (default: ``0x1000``): sector size in bytes, in hex format
* ``ZMS_OUTPUT_NAME`` (default: ``provisioned_raw.hex``): output file name for the
  standalone provisioning-only image

Generated Artifacts
*******************

The build always produces these files in ``build/zephyr``:

* ``zephyr.hex``: application image
* ``provisioned_raw.hex``: standalone provisioning-only image (no application code)
* ``zephyr_with_provision.hex``: merged image, used by ``west flash``
