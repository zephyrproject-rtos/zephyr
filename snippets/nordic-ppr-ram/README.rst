.. _nordic-ppr-ram:

Nordic boot PPR snippet with execution from RAM (nordic-ppr-ram)
################################################################

Overview
********

This snippet allows users to build Zephyr with the capability to boot Nordic PPR
(Peripheral Processor) from another core. PPR code is executed from RAM. Note
that PPR image must be built with :kconfig:option:`CONFIG_XIP` disabled.
