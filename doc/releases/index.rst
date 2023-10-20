.. _zephyr_release_notes:

Releases
########

Zephyr project is provided as source code and build scripts for different target
architectures and configurations, and not as a binary image. Updated versions of
the Zephyr project are released approximately every four months.

All Zephyr project source code is maintained in a `GitHub repository`_. In order
to use a released version of the Zephyr project, it is recommended that you use
:ref:`west` to :ref:`get_the_code` of the release you are interested in.

The technical documentation for current and past releases is available at
https://docs.zephyrproject.org/ (use the version selector to select your release
of interest).

Release Life Cycle and Maintenance
**********************************

Periodic Releases
=================

The Zephyr project provides periodic releases every 4 months leading to the
long term support releases approximately every 2 years. Periodic and non-LTS
releases are maintained with updates, bug fixes and security related updates
for at least two cycles, meaning that the project supports the most recent two
releases in addition to the most recent LTS.

Long Term Support and Maintenance
=================================

A Zephyr :ref:`Long Term Support (LTS) <release_process_lts>` release is
published every 2 years and is branched and maintained independently from the
main tree for at least 2.5 years after it was released.

Support and maintenance for an LTS release stops at least half a year
after the following LTS release is published.

Security Fixes
==============

Each security issue fixed within Zephyr is backported or submitted to the
following releases:

- Currently supported Long Term Support (LTS) release.

- The most recent two releases.

For more information, see  :ref:`Security Vulnerability Reporting <reporting>`.


Supported Releases
******************

+-----------------+----------------+---------------+
| Release         | Release date   | EOL           |
+=================+================+===============+
| `Zephyr 2.7.5`_ | 2023-06-01     | 2024-08-31    |
+-----------------+----------------+---------------+
| `Zephyr 3.5.0`_ | 2023-10-20     | 2024-07-26    |
+-----------------+----------------+---------------+
| `Zephyr 3.4.0`_ | 2023-06-16     | 2024-02-29    |
+-----------------+----------------+---------------+
| `Zephyr 3.3.0`_ | 2023-02-19     | 2023-10-31    |
+-----------------+----------------+---------------+

As of 2022-01-01, LTS1 (1.14.x) is not supported and has reached end of life (EOL).

Release Notes
*************

.. toctree::
   :maxdepth: 1
   :glob:
   :reversed:

   eol_releases
   release-notes-2.7
   release-notes-3.[3-5]

Migration Guides
****************

Zephyr provides migration guides for all major releases, in order to assist
users transition from the previous release.

.. toctree::
   :maxdepth: 1
   :glob:
   :reversed:

   migration-guide-*

.. _`GitHub repository`: https://github.com/zephyrproject-rtos/zephyr
.. _`GitHub tagged releases`: https://github.com/zephyrproject-rtos/zephyr/tags
.. _`Zephyr 2.7.5`: https://docs.zephyrproject.org/2.7.5/
.. _`Zephyr 3.3.0`: https://docs.zephyrproject.org/3.3.0/
.. _`Zephyr 3.4.0`: https://docs.zephyrproject.org/3.4.0/
.. _`Zephyr 3.5.0`: https://docs.zephyrproject.org/3.5.0/
