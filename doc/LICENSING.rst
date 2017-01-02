.. _zephyr_licensing:

Licensing of Zephyr Project components
######################################

The Zephyr kernel tree imports or reuses packages, scripts and other files that
are not covered by the :download:`Apache License <../LICENSE>`. In some places
there is no LICENSE file or way to put a LICENSE file there, so we describe the
licensing in this document.


- *kconfig* and *kbuild*

  *Origin:* Linux Kernel
  *Licensing:* *GPLv2*

- *scripts/{checkpatch.pl,checkstack.pl,get_maintainers.pl,spelling.txt}*

  *Origin:* Linux Kernel
  *Licensing:* *GPLv2*

- *ext/fs/fat/*

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

- *ext/hal/cmsis/*

  *Origin:* https://github.com/ARM-software/CMSIS.git

  *Licensing*: :download:`CMSIS_END_USER_LICENCE_AGREEMENT <../ext/hal/cmsis/CMSIS_END_USER_LICENCE_AGREEMENT.pdf>`

- *ext/hal/nordic/*

  *Origin:*

  *Licensing*: 3-clause BSD (see :download:`source <../ext/hal/nordic/mdk/nrf51.h>`)

- *ext/hal/nxp/mcux/*

  *Origin:* http://mcux.nxp.com

  *Licensing*: 3-clause BSD (see :download:`source <../ext/hal/nxp/mcux/drivers/fsl_rtc.h>`)

- *ext/hal/qmsi/*

  *Origin:* https://github.com/quark-mcu/qmsi/releases

  *Licensing*: 3-clause BSD (see :download:`source <../ext/hal/qmsi/include/qm_common.h>`)
