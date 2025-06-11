.. zephyr:code-sample:: nrf_ironside_update
   :name: Nordic IronSide SE firmware update

   Update the Nordic IronSide SE firmware.

Overview
********

The Nordic IronSide SE Update sample updates the IronSide SE firmware on a SoC that already has IronSide SE installed.
It can update both the main image and the recovery image of IronSide SE using the IronSide SE firmware release ZIP file.

Update procedure
****************

The update procedure works as follows:

1. The application invokes the IronSide SE update service and passes the parameters that correspond to the location of the HEX file of the IronSide SE firmware update.

#. The application prints the return value of the service call and outputs information from the update HEX file.

#. After the service call completes, the IronSide SE firmware updates the internal state of the device.

#. The firmware installs the update during the next device boot.
   This operation can take several seconds.

Once the operation has completed, you can read the boot report to verify that the update has taken place.

Building and running the application for nrf54h20dk/nrf54h20/cpuapp/iron
************************************************************************

.. note::
   You can use this application only when there is already a version of IronSide SE installed on the device.

1. Unzip the IronSide SE release ZIP to get the update hex file:

   .. code-block:: console

      unzip nrf54h20_soc_binaries_v.*.zip

#. Program the appropriate update hex file from the release ZIP using one (not both) of the following commands:

   a) To update IronSide SE firmware:

      .. code-block:: console

         nrfutil device program --traits jlink --firmware update/ironside_se_update.hex

   b) To update IronSide SE recovery firmware:

      .. code-block:: console

         nrfutil device program --traits jlink --firmware update/ironside_se_recovery_update.hex

#. Build and program the application:

   .. zephyr-app-commands::
      :zephyr-app: samples/boards/nordic/nrf_ironside/update
      :board: nrf54h20dk/nrf54h20/cpuapp/iron
      :goals: flash

#. Trigger a reset:

.. code-block:: console

   nrfutil device reset --reset-kind RESET_PIN --traits jlink

#. Check that the new version is installed:

   .. code-block:: console

      # Read the version fields from the boot report
      nrfutil x-read --direct --address 0x2f88fd04 --bytes 16 --traits jlink
      # Read the update status in the boot report
      nrfutil x-read --direct --address 0x2f88fd24 --bytes 4 --traits jlink
