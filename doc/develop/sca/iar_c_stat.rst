.. _icstat:

IAR C-STAT support
##################

`IAR C-STAT <https://iar.com/cstat>`__ is a comprehensive static analysis tool for
C/C++ source code. It can find errors and vulnerabilities supporting a number of
coding standards such as MISRA C, MISRA C++, CERT C/C++ and CWE.

Installing IAR C-STAT
*********************

IAR C-STAT comes pre-installed with the IAR Build Tools and with the IAR Embedded
Workbench. Refer to your respective product's documentation for details.

Building with IAR C-STAT
************************

To run IAR C-STAT, you will need CMake 4.1.0 or later. When building with
:ref:`west build <west-building>` append the additional parameter to select
IAR C-STAT ``-DZEPHYR_SCA_VARIANT=iar_c_stat``, e.g.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f429ii_aca
   :gen-args: -DZEPHYR_SCA_VARIANT=iar_c_stat
   :goals: build
   :compact:

Configuring IAR C-STAT
***********************

The IAR C-STAT accepts parameters for customizing the analysis.
The following table lists the supported options.

.. list-table::
   :header-rows: 1

   * - Parameter
     - Description
   * - ``CSTAT_RULESET``
     - The pre-defined ruleset to be used. (default: ``stdchecks``, accepted values: ``all,cert,misrac2004,misrac2012,misrac++2008,stdchecks,security``)
   * - ``CSTAT_ANALYZE_THREADS``
     - The number of threads to use in analysis. (default: <CPU count>)
   * - ``CSTAT_ANALYZE_OPTS``
     - Arguments passed to the ``analyze`` command directly. (e.g. ``--timeout=900;--deterministic;--fpe``)
   * - ``CSTAT_DB``
     - Override the default location of the C-STAT SQLite database. (e.g. ``/home/user/cstat.db``)
   * - ``CSTAT_CLEANUP``
     - Perform a cleanup of the C-STAT SQLite database. (e.g. ``true``)

These parameters can be passed on the command line, or be set as environment
variables. Below you will find an example of how to enable and combine
non-standard rulesets at will:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f429ii_aca
   :gen-args: -DZEPHYR_SCA_VARIANT=iar_c_stat -DCSTAT_RULESET=misrac2012,cert
   :goals: build
