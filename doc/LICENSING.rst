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

*kconfig* and *kbuild*
  *Origin:* Linux Kernel

  *Licensing:* `GPLv2 License`_

*scripts/{checkpatch.pl,checkstack.pl,get_maintainers.pl,spelling.txt}*
  *Origin:* Linux Kernel

  *Licensing:* `GPLv2 License`_

*ext/fs/fat/*
  *Origin:* FatFs is a file system based on the FAT file system specification.  This is
  provided by ELM Chan http://elm-chan.org/fsw/ff/00index_e.html

  *Licensing*:

    Copyright (C) 2016, ChaN, all right reserved.

    FatFs module is an open source software. Redistribution and use of FatFs in
    source and binary forms, with or without modification, are permitted provided
    that the following condition is met:

    1. Redistributions of source code must retain the above copyright notice,
       this condition and the following disclaimer.

    This software is provided by the copyright holder and contributors "AS IS"
    and any warranties related to this software are DISCLAIMED.
    The copyright owner or contributors be NOT LIABLE for any damages caused
    by use of this software.

*ext/hal/cmsis/*
  *Origin:* https://github.com/ARM-software/CMSIS.git

  *Licensing*: `CMSIS END USER LICENCE AGREEMENT`_

.. _CMSIS END USER LICENCE AGREEMENT:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/ext/hal/cmsis/CMSIS_END_USER_LICENCE_AGREEMENT.pdf

*ext/hal/nordic/*
  *Origin:*

  *Licensing*: 3-clause BSD (see `ext/hal/nordic source`_)

.. _ext/hal/nordic source:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/ext/hal/nordic/mdk/nrf51.h

*ext/hal/nxp/mcux/*
  *Origin:* http://mcux.nxp.com

  *Licensing*: 3-clause BSD (see `ext/hal/nxp/mcux source`_)

.. _ext/hal/nxp/mcux source:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/ext/hal/nxp/mcux/drivers/fsl_rtc.h

*ext/hal/qmsi/*
  *Origin:* https://github.com/quark-mcu/qmsi/releases

  *Licensing*: 3-clause BSD (see `ext/hal/qmsi source`_)

.. _ext/hal/qmsi source:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/ext/hal/qmsi/include/qm_common.h
