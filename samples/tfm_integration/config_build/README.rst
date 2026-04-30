.. zephyr:code-sample:: tfm_config_build
   :name: TF-M build configuration

   Verify TF-M builds across different bootloader and image configurations.

Overview
********

This sample exists to validate that TF-M integrates correctly with Zephyr
under different build configurations. The application code is intentionally
minimal — it prints ``Hello World!`` with the board target to confirm the
build succeeded and the non-secure image boots properly.

Two configuration variants are tested:

* **No BL2**: Builds without the MCUboot-based BL2 bootloader
  (``CONFIG_TFM_BL2=n``).
* **Single-image MCUboot**: Packages TF-M and Zephyr as a single MCUboot
  image (``CONFIG_TFM_MCUBOOT_IMAGE_NUMBER=1``).

Requirements
************

A board with TF-M support. Currently tested on:

- ``v2m_musca_s1/musca_s1/ns``
- ``nrf5340dk/nrf5340/cpuapp/ns``
- ``nrf9160dk/nrf9160/ns``
- ``nrf54l15dk/nrf54l15/cpuapp/ns``
- ``nrf54l15dk/nrf54l10/cpuapp/ns``
- ``nrf54lm20dk/nrf54lm20a/cpuapp/ns``
- ``bl5340_dvk/nrf5340/cpuapp/ns``

Building and Running
********************

Default build:

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/config_build
   :board: nrf5340dk/nrf5340/cpuapp/ns
   :goals: build

Without BL2:

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/config_build
   :board: nrf5340dk/nrf5340/cpuapp/ns
   :goals: build
   :gen-args: -DCONFIG_TFM_BL2=n

Single-image MCUboot (STM32 platforms):

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/config_build
   :board: stm32h573i_dk/stm32h573xx/ns
   :goals: build
   :gen-args: -DCONFIG_TFM_MCUBOOT_IMAGE_NUMBER=1

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0 ***
   Hello World! nrf5340dk/nrf5340/cpuapp/ns
