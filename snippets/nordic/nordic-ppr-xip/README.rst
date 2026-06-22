.. _nordic-ppr-xip:

Nordic boot PPR snippet with execution in place (nordic-ppr-xip)
################################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot Nordic PPR
(Peripheral Processor) from another core. PPR code is to be executed from MRAM,
so the PPR image must be built for the ``xip`` board variant, or with
:kconfig:option:`CONFIG_XIP` enabled.
