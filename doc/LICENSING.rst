:orphan:

.. _zephyr_licensing:

Licensing of Zephyr Project components
######################################

The Zephyr kernel tree imports or reuses packages, scripts and other files that
are not covered by the `Apache 2.0 License`_. In some places
there is no LICENSE file or way to put a LICENSE file there, so we describe the
licensing in this document.

.. _Apache 2.0 License:
   https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE

.. _GPLv2 License:
   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/COPYING

*scripts/{checkpatch.pl,checkstack.pl,spelling.txt}*
  *Origin:* Linux Kernel

  *Licensing:* `GPLv2 License`_

*scripts/{coccicheck,coccinelle/array_size.cocci,coccinelle/deref_null.cocci,coccinelle/deref_null.cocci,coccinelle/deref_null.cocci,coccinelle/mini_lock.cocci,coccinelle/mini_lock.cocci,coccinelle/mini_lock.cocci,coccinelle/noderef.cocci,coccinelle/noderef.cocci,coccinelle/returnvar.cocci,coccinelle/semicolon.cocci}*
  *Origin:* Coccinelle

  *Licensing:* `GPLv2 License`_

*subsys/testsuite/coverage/coverage.h*
  *Origin:* GCC, the GNU Compiler Collection

  *Licensing:* `GPLv2 License`_ with Runtime Library Exception

*boards/ene/kb1200_evb/support/openocd.cfg*
  *Licensing:* `GPLv2 License`_

.. _MIT License:
  https://opensource.org/licenses/MIT

*tests/benchmarks/thread_metric/{thread_metric_readme.txt,src/\*}*
  *Origin:* ThreadX

  *Licensing:* `MIT License`_

.. _BSD-3-clause:
   https://opensource.org/license/bsd-3-clause

*modules/openthread/platform/{hdlc_interface.cpp,hdlc_interface.hpp,radio_spinel.cpp}*
  *Origin:* OpenThread

  *Licensing:* `BSD-3-clause`_
