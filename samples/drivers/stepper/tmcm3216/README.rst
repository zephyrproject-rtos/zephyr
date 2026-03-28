.. zephyr:code-sample:: tmcm3216
   :name: TMCM-3216 stepper
   :relevant-api: stepper_interface

   Control a stepper motor using the ADI TMCM-3216 standalone stepper motor
   controller and driver board over RS485.

Description
***********

This sample demonstrates how to use the TMCM-3216 3-axis standalone stepper motor
controller and driver board with Zephyr's stepper driver API. It configures the
ramp parameters (velocity and acceleration) using the generic
:c:func:`stepper_ctrl_configure_ramp` API, then performs a ping-pong movement
depending on the :kconfig:option:`CONFIG_PING_PONG_N_REV` configuration, reversing
direction each time the target position is reached.

The sample also showcases TMCM-3216-specific extensions:

- ``tmcm3216_get_max_velocity`` -- read back configured velocity
- ``tmcm3216_get_actual_velocity`` -- read real-time motor velocity
- ``tmcm3216_get_status`` -- read extended status (position, velocity, endstops,
  position reached flag)

Wiring
******

The TMCM-3216 communicates via RS485. Connect:

- STM32 USART2 TX (PD5) to RS485 transceiver DI
- STM32 USART2 RX (PD6) to RS485 transceiver RO
- STM32 GPIO A0 (PA3) to RS485 transceiver DE/RE
- RS485 transceiver A/B to TMCM-3216 RS485 A/B

Building and Running
********************

This project controls the stepper and outputs status to the console. It requires
an ADI TMCM-3216 module connected via an RS485 transceiver.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/tmcm3216
   :board: nucleo_u575zi_q
   :goals: build flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   [00:00:00.059,000] <inf> stepper_tmcm3216: Starting TMCM-3216 stepper sample
   [00:00:00.117,000] <inf> stepper_tmcm3216: Ramp configured: speed=50000 usteps/s accel=5000 decel=5000 usteps/s^2
   [00:00:01.234,000] <inf> stepper_tmcm3216: Status: pos=3200 vel=0 moving=no pos_reached=yes left_end=no right_end=no
   [00:00:01.234,000] <inf> stepper_tmcm3216: Moving by -3200 steps
   [00:00:02.345,000] <inf> stepper_tmcm3216: Status: pos=0 vel=0 moving=no pos_reached=yes left_end=no right_end=no
   [00:00:02.345,000] <inf> stepper_tmcm3216: Moving by 3200 steps

   <repeats endlessly>
