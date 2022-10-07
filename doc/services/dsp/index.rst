.. _zdsp_api:

Digital Signal Processing (DSP)
###############################

.. contents::
    :local:
    :depth: 2

The DSP API provides an architecture agnostic way for signal processing.
Currently, the API will work on any architecture but will likely not be
optimized. The status of the various architectures can be found below:

+--------------+-------------+
| Architecture | Status      |
+--------------+-------------+
| ARC          | Unoptimized |
| ARM          | Optimized   |
| ARM64        | Optimized   |
| MIPS         | Unoptimized |
| NIOS2        | Unoptimized |
| POSIX        | Unoptimized |
| RISCV        | Unoptimized |
| RISCV64      | Unoptimized |
| SPARC        | Unoptimized |
| X86          | Unoptimized |
| XTENSA       | Unoptimized |
+--------------+-------------+

Using zDSP
**********

zDSP provides various backend options which are selected automatically for the
application. By default, including the CMSIS module will enable all
architectures to use the zDSP APIs. This can be done by setting::

	CONFIG_CMSIS_DSP=y

If your application requires some additional customization, it's possible to
enable :kconfig:option:`CONFIG_DSP_BACKEND_CUSTOM` which means that the
application is responsible for providing the implementation of the zDSP
library.

Optimizing for your architecture
********************************

If your architecture is showing as ``Unoptimized``, it's possible to add a new
zDSP backend to better support it. To do that, a new Kconfig option should be
added to `subsys/dsp/Kconfig`_ along with the required dependencies and the
``default`` set for ``DSP_BACKEND`` Kconfig choice.

Next, the implementation should be added at ``subsys/dsp/<backend>/`` and
linked in at `subsys/dsp/CMakeLists.txt`_.

API Reference
*************

.. doxygengroup:: math_dsp

.. _subsys/dsp/Kconfig: https://github.com/zephyrproject-rtos/zephyr/blob/main/subsys/dsp/Kconfig
.. _subsys/dsp/CMakeLists.txt: https://github.com/zephyrproject-rtos/zephyr/blob/main/subsys/dsp/CMakeLists.txt
