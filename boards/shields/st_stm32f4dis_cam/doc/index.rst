.. _st_stm32f4dis_cam:

ST STM32F4DIS-CAM
#################

Overview
********

The STM32F4DIS-CAM camera board embeds a 1.3MPix OV9655 sensor and a
30 pins FFC connector in order to be used for some of the STM32 Discovery
kits such as STM32L4R9I-disco board offering a DVP camera interface.

Requirements
************

The camera module bundle is compatible with STM32 Discovery kits and
Evaluation boards featuring a 30 pins FFC connector, such as the STM32F4
Discovery kit or STM32L4R9I Discovery kit.

Usage
*****

The shield can be used in any application by setting ``SHIELD`` to
``st_stm32f4dis_cam`` for boards with the necessary device tree node labels.

Set ``--shield "st_stm32f4dis_cam"`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: stm32l4r9i_disco
   :shield: st_stm32f4dis_cam
   :goals: build
