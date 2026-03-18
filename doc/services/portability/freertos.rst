.. _freertos:

FreeRTOS
########

FreeRTOS is an MIT-licensed RTOS with its own APIs documented at `<https://www.freertos.org/>`_.

The interface with Zephyr is organized in a way to allow every platform to leverage common
function definitions or override individual function by providing their own.

Every function implemented in common is declared with a ``__weak`` attribute, permitting C source
or linker scripts to override the particular implementation. For instance allowing binary files
or ROM contained in bootloaders to provide some FreeRTOS functions.

Zephyr contributors are encouraged to first extend the shared implementation and fallback to a
local implementation if not feasible.
