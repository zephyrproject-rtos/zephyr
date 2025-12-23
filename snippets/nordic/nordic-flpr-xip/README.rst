.. _nordic-flpr-xip:

Nordic FLPR snippet with execution in place (nordic-flpr-xip)
#############################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot Nordic FLPR
(Fast Lightweight Peripheral Processor) from application core.
FLPR code is to be executed from RRAM, so the FLPR image must be built
for the ``xip`` board variant, or with :kconfig:option:`CONFIG_XIP` enabled.
