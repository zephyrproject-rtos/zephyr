:orphan:

.. _zephyr_licensing:

Licensing of Zephyr Project components
######################################

The Zephyr kernel tree imports or reuses packages, scripts and other files that
are not covered by the `Apache 2.0 License`_. In some places
there is no LICENSE file or way to put a LICENSE file there, so we describe the
licensing in this document.

Continuous Integration Scripts
------------------------------

* *Origin:* Linux Kernel
* *Licensing:* `GPLv2 License`_
* *Impact:* These files are used in Continuous Integration (CI) and never linked into the firmware.
* *Files:*

  * :zephyr_file:`scripts/checkpatch.pl`
  * :zephyr_file:`scripts/checkstack.pl`
  * :zephyr_file:`scripts/spelling.txt`

Coccinelle Scripts
------------------

  * *Origin:* Coccinelle
  * *Licensing:* `GPLv2 License`_
  * *Impact:* These files are used by `Coccinelle`_, a tool for transforming C-code, and never linked
    into the firmware.
  * *Files:*

    * :zephyr_file:`scripts/coccicheck`
    * :zephyr_file:`scripts/coccinelle/array_size.cocci`
    * :zephyr_file:`scripts/coccinelle/deref_null.cocci`
    * :zephyr_file:`scripts/coccinelle/deref_null.cocci`
    * :zephyr_file:`scripts/coccinelle/deref_null.cocci`
    * :zephyr_file:`scripts/coccinelle/mini_lock.cocci`
    * :zephyr_file:`scripts/coccinelle/mini_lock.cocci`
    * :zephyr_file:`scripts/coccinelle/mini_lock.cocci`
    * :zephyr_file:`scripts/coccinelle/noderef.cocci`
    * :zephyr_file:`scripts/coccinelle/noderef.cocci`
    * :zephyr_file:`scripts/coccinelle/returnvar.cocci`
    * :zephyr_file:`scripts/coccinelle/semicolon.cocci`

GCOV Coverage Header File
-------------------------

* *Origin:* GCC, the GNU Compiler Collection
* *Licensing:* `GPLv2 License`_ with Runtime Library Exception
* *Impact:* This file is only linked into the firmware if :kconfig:option:`CONFIG_COVERAGE_GCOV` is
  enabled.
* *Files:*

  * :zephyr_file:`subsys/testsuite/coverage/coverage.h`

ENE KB1200_EVB Board OpenOCD Configuration
------------------------------------------

* *Licensing:* `GPLv2 License`_
* *Impact:* This file is used by `OpenOCD`_ when programming and debugging the
  :zephyr:board:`kb1200_evb` board. It is never linked into the firmware.
* *Files:*

  * :zephyr_file:`boards/ene/kb1200_evb/support/openocd.cfg`

Thread-Metric RTOS Test Suite Source Files
------------------------------------------

* *Origin:* ThreadX
* *Licensing:* `MIT License`_
* *Impact:* These files are only linked into the Thread-Metric RTOS Test Suite test firmware.
* *Files:*

  * :zephyr_file:`tests/benchmarks/thread_metric/thread_metric_readme.txt`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_api.h`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_basic_processing_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_cooperative_scheduling_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_interrupt_preemption_processing_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_interrupt_processing_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_memory_allocation_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_message_processing_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_porting_layer.h`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_porting_layer_zephyr.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_preemptive_scheduling_test.c`
  * :zephyr_file:`tests/benchmarks/thread_metric/src/tm_synchronization_processing_test.c`

OpenThread Spinel HDLC RCP Host Interface Files
-----------------------------------------------

* *Origin:* OpenThread
* *Licensing:* `BSD-3-clause`_
* *Impact:* These files are only linked into the firmware if :kconfig:option:`CONFIG_HDLC_RCP_IF` is
  enabled.
* *Files*:

  * :zephyr_file:`modules/openthread/platform/hdlc_interface.hpp`
  * :zephyr_file:`modules/openthread/platform/radio_spinel.cpp`
  * :zephyr_file:`modules/openthread/platform/hdlc_interface.cpp`

Python Devicetree library test files
------------------------------------

* *Licensing:* `BSD-3-clause`_
* *Impact:* These are only used for testing and never linked with the firmware.
* *Files*:

  * Various yaml files under ``scripts/dts/python-devicetree/tests``

.. _Apache 2.0 License:
   https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE

.. _GPLv2 License:
   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/COPYING

.. _MIT License:
  https://opensource.org/licenses/MIT

.. _BSD-3-clause:
   https://opensource.org/license/bsd-3-clause

.. _Coccinelle:
   https://coccinelle.gitlabpages.inria.fr/website/

.. _OpenOCD:
   https://openocd.org
