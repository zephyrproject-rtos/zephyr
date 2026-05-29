.. _nordic-flpr:

Nordic FLPR snippet with execution from SRAM (nordic-flpr)
##########################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot Nordic FLPR
(Fast Lightweight Peripheral Processor) from application core.
FLPR code is to be executed from SRAM, so the FLPR image must be built
without the ``xip`` board variant, or with :kconfig:option:`CONFIG_XIP` disabled.
