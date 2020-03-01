:orphan:

.. _zephyr_licensing:

Licensing of Zephyr Project components
######################################

The Zephyr kernel tree imports or reuses packages, scripts and other files that
are not covered by the `Apache 2.0 License`_. In some places
there is no LICENSE file or way to put a LICENSE file there, so we describe the
licensing in this document.

.. _Apache 2.0 License:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/LICENSE

.. _GPLv2 License:
   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/COPYING

*scripts/{checkpatch.pl,checkstack.pl,get_maintainers.pl,spelling.txt}*
  *Origin:* Linux Kernel

  *Licensing:* `GPLv2 License`_

*ext/hal/cmsis/*
  *Origin:* https://github.com/ARM-software/CMSIS_5.git

  *Licensing*: Apache 2.0 (see `ext/hal/cmsis source`_)

.. _ext/hal/cmsis source:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/ext/hal/cmsis/Include/cmsis_version.h
