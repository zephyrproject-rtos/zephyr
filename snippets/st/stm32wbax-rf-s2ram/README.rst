.. _st-s2ram:

STM32WBAX suspend to ram configuration when rf enabled (stm32wbax-rf-s2ram)
###########################################################################

Overview
********

This snippet allows users to build Zephyr applications for STMicroelectronics stm32wbax
devices using a suspend to ram configuration (mapped on stdby hw feature) suitable for
rf applications.
Tested on nucleo_wba55cg and nucleo_wba65ri boards.

.. code-block:: console

   west build -S stm32wbax-rf-s2ram [...]

Requirements
************

Actually this configuration has been tested only nucleo_wba55cg and nucleo_wba65ri
