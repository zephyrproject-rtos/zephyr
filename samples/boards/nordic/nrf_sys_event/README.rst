.. zephyr:code-sample:: nrf-sys-event
   :name: nRF System Events

   Manage global constant latency mode on Nordic SoCs using the nRF System Events API.

Overview
********

This sample demonstrates the use of the ``nrf_sys_event`` API to request and release
global constant latency mode on Nordic Semiconductor SoCs.

The sample performs the following steps:

1. Requests global constant latency mode.
2. Requests global constant latency mode a second time (reference counting incremented).
3. Releases global constant latency mode — mode remains active due to the outstanding request.
4. Releases global constant latency mode again — mode is now disabled.

If :kconfig:option:`CONFIG_NRF_SYS_EVENT_IRQ_LATENCY` is enabled, the sample additionally
measures interrupt latency for RRAMC (Resistive RAM Controller) access in three modes:

**Default RRAMC mode**
  RRAMC enters low-power state with an approximately 15 µs wake-up time.

**RRAMC woken by PPI**
  Uses the nrf_sys_event API to schedule a PPI wake-up before the expected interrupt.

**RRAMC Standby mode**
  Uses the nrf_sys_event API to configure RRAMC to 0 µs wake-up time.

Requirements
************

The sample is supported on the following Nordic Semiconductor board targets:

- nrf52dk/nrf52810
- nrf52dk/nrf52832
- nrf52833dk/nrf52820
- nrf52833dk/nrf52833
- nrf52840dk/nrf52811
- nrf52840dk/nrf52840
- nrf5340dk/nrf5340/cpuapp
- nrf5340dk/nrf5340/cpunet
- nrf54h20dk/nrf54h20/cpuapp
- nrf54h20dk/nrf54h20/cpurad
- nrf54l15dk/nrf54l15/cpuapp
- nrf7120dk/nrf7120/cpuapp

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/nrf_sys_event
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

To test with forced constant latency mode enabled from boot
(:kconfig:option:`CONFIG_SOC_NRF_FORCE_CONSTLAT`):

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/nrf_sys_event
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :gen-args: -DCONFIG_SOC_NRF_FORCE_CONSTLAT=y
   :compact:

To enable IRQ latency measurement for RRAMC (nrf54l15dk only):

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/nrf_sys_event
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: build flash
   :gen-args: -DCONFIG_NRF_SYS_EVENT_IRQ_LATENCY=y
   :compact:

Sample Output
*************

.. code-block:: console

   request global constant latency mode
   constant latency mode enabled
   request global constant latency mode again
   release global constant latency mode
   constant latency mode will remain enabled
   release global constant latency mode again
   constant latency mode will be disabled
   constant latency mode disabled
