.. _posix_overview:

Overview
########

The Portable Operating System Interface (POSIX) is a family of standards specified by the
`IEEE Computer Society`_ for maintaining compatibility between operating systems. Zephyr
implements a subset of the standard POSIX API specified by `IEEE 1003.1-2017`_ (also known as
POSIX-1.2017).

..  figure:: posix.svg
    :align: center
    :alt: POSIX Support in Zephyr

    POSIX support in Zephyr

.. note::
    This page does not document Zephyr's :ref:`POSIX architecture<Posix arch>`, which is used to
    run Zephyr as a native application under the host operating system for prototyping,
    test, and diagnostic purposes.

With the POSIX support available in Zephyr, an existing POSIX conformant
application can be ported to run on the Zephyr kernel, and therefore leverage
Zephyr features and functionality. Additionally, a library designed to be
POSIX conformant can be ported to Zephyr kernel based applications with no changes.

The POSIX API is an increasingly popular OSAL (operating system abstraction layer) for IoT and
embedded applications, as can be seen in Zephyr, AWS:FreeRTOS, TI-RTOS, and NuttX.

Benefits of POSIX support in Zephyr include:

- Offering a familiar API to non-embedded programmers, especially from Linux
- Enabling reuse (portability) of existing libraries based on POSIX APIs
- Providing an efficient API subset appropriate for small (MCU) embedded systems

.. _posix_subprofiles:

POSIX Subprofiles
=================

While Zephyr supports running multiple :ref:`threads <threads_v2>` (possibly in an
:ref:`SMP <smp_arch>` configuration), as well as
:ref:`Virtual Memory and MMUs <memory_management_api>`, Zephyr code and data normally share a
common address space that is partitioned into separate :ref:`Memory Domains <memory_domain>`. The
Zephyr kernel executable code and the application executable code are typically compiled into the
same binary artifact. From that perspective, Zephyr apps can be seen as running in the context of
a single process.

While multi-purpose operating systems (OS) offer full POSIX conformance, Real-Time Operating
Systems (RTOS) such as Zephyr typically serve a fixed-purpose, have limited hardware resources,
and experience limited user interaction. In such systems, full POSIX conformance can be
impractical and unnecessary.

For that reason, POSIX defined the following :ref:`Application Environment Profiles (AEP)<posix_aep>`
as part of `IEEE 1003.13-2003`_ (also known as POSIX.13-2003). Each AEP adds incrementally more
features over the required :ref:`POSIX System Interfaces <posix_system_interfaces>`.

..  figure:: aep.svg
    :align: center
    :scale: 150%
    :alt: POSIX Application Environment Profiles (AEP)

    POSIX Application Environment Profiles (AEP)

* Minimal Realtime System Profile (:ref:`PSE51 <posix_aep_pse51>`)
* Realtime Controller System Profile (:ref:`PSE52 <posix_aep_pse52>`)
* Dedicated Realtime System Profile (:ref:`PSE53 <posix_aep_pse53>`)
* Multi-Purpose Realtime System (PSE54)

POSIX.13-2003 AEP were formalized in 2003 via "Units of Functionality" but the specification is now
inactive (for reference only). Nevertheless, the intent is still captured as part of POSIX-1.2017
via :ref:`Options<posix_options>` and :ref:`Option Groups<posix_option_groups>`.

For more information, please see `IEEE 1003.1-2017, Section E, Subprofiling Considerations`_.

.. _posix_apps:

POSIX Applications in Zephyr
============================

A POSIX app in Zephyr is :ref:`built like any other app<application>` and therefore requires the
usual :file:`prj.conf`, :file:`CMakeLists.txt`, and source code. For example, the app below
leverages the ``nanosleep()`` and ``perror()`` POSIX functions.

.. code-block:: cfg
   :caption: `prj.conf` for a simple POSIX app in Zephyr

    CONFIG_POSIX_API=y

.. code-block:: c
   :caption: A simple app that uses Zephyr's POSIX API

    #include <stddef.h>
    #include <stdio.h>
    #include <time.h>

    void megasleep(size_t megaseconds)
    {
        struct timespec ts = {
            .tv_sec = megaseconds * 1000000,
            .tv_nsec = 0,
        };

        printf("See you in a while!\n");
        if (nanosleep(&ts, NULL) == -1) {
            perror("nanosleep");
        }
    }

    int main()
    {
        megasleep(42);
        return 0;
    }

For more examples of POSIX applications, please see the :ref:`POSIX sample applications<posix-samples>`.

.. _posix_config:

Configuration
=============

Like most features in Zephyr, POSIX features are
:ref:`highly configurable<zephyr_intro_configurability>` but disabled by default. Users must
explicitly choose to enable POSIX options via :ref:`Kconfig<kconfig>` selection.

Subprofiles
+++++++++++

Enable one of the Kconfig options below to quickly configure a pre-defined
:ref:`POSIX subprofile <posix_subprofiles>`.

* :kconfig:option:`CONFIG_POSIX_AEP_CHOICE_BASE` (:ref:`Base <posix_system_interfaces_required>`)
* :kconfig:option:`CONFIG_POSIX_AEP_CHOICE_PSE51` (:ref:`PSE51 <posix_aep_pse51>`)
* :kconfig:option:`CONFIG_POSIX_AEP_CHOICE_PSE52` (:ref:`PSE52 <posix_aep_pse52>`)
* :kconfig:option:`CONFIG_POSIX_AEP_CHOICE_PSE53` (:ref:`PSE53 <posix_aep_pse53>`)

Additional POSIX :ref:`Options and Option Groups <posix_option_groups>` may be enabled as needed
via Kconfig (e.g. ``CONFIG_POSIX_C_LIB_EXT=y``). Further fine-tuning may be accomplished via
:ref:`additional POSIX-related Kconfig options <posix_kconfig_options>`.

Subprofiles, Options, and Option Groups should be considered the preferred way to configure POSIX
in Zephyr going forward.

Legacy
++++++

Historically, Zephyr used :kconfig:option:`CONFIG_POSIX_API` to configure a set of POSIX features
that was overloaded and always increasing in size.

* :kconfig:option:`CONFIG_POSIX_API`

The option is now frozen, and can be considered equivalent to the following:

* :kconfig:option:`CONFIG_POSIX_AEP_CHOICE_PSE51`
* :kconfig:option:`CONFIG_POSIX_FD_MGMT`
* :kconfig:option:`CONFIG_POSIX_MESSAGE_PASSING`
* :kconfig:option:`CONFIG_POSIX_NETWORKING`

However, :kconfig:option:`CONFIG_POSIX_API` should be considered legacy and should not be used for
new Zephyr applications.

.. _IEEE: https://www.ieee.org/
.. _IEEE Computer Society: https://www.computer.org/
.. _IEEE 1003.1-2017: https://standards.ieee.org/ieee/1003.1/7101/
.. _IEEE 1003.13-2003: https://standards.ieee.org/ieee/1003.13/3322/
.. _IEEE 1003.1-2017, Section E, Subprofiling Considerations:
    https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html
