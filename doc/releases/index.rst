.. _zephyr_release_notes:

Releases
########

Zephyr project is provided as source code and build scripts for different
target architectures and configurations, and not as a binary image. Updated
versions of the Zephyr project are released approximately every four months.

All Zephyr project source code is maintained in a `GitHub repository`_.

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
| `Zephyr 2.7.5`_ | 01/06/2023     | 31/08/2024    |
+-----------------+----------------+---------------+
| `Zephyr 3.3.0`_ | 19/02/2023     | 31/10/2023    |
+-----------------+----------------+---------------+
| `Zephyr 3.2.0`_ | 30/09/2022     | 31/06/2023    |
+-----------------+----------------+---------------+


As of 01/01/2022, LTS1 (1.14.x) is not supported and has reached end of life (EOL).

Release Notes
*************

For Zephyr versions up to 1.13, you can either download source as a tar.gz file
(see the bottom of the `GitHub tagged releases`_ page corresponding to each
release), or clone the GitHub repository.

With the introduction of the :ref:`west` tool after the release of Zephyr 1.13,
it is no longer recommended to download or clone the source code manually.
Instead we recommend you follow the instructions in :ref:`get_the_code` to do
so with the help of west.

The project's technical documentation is also tagged to correspond with a
specific release and can be found at https://docs.zephyrproject.org/.

.. comment We need to split the globbing of release notes to get the
   single-digit and double-digit subversions sorted correctly.  Specify
   names in normal order and use the :reversed: option to reverse it.
   This will get us through 10 subversions (0-9) before we need to
   update this list for two-digit subversions again.

.. toctree::
   :maxdepth: 1
   :glob:
   :reversed:

   release-notes-1.?
   release-notes-1.*
   release-notes-*

.. _`GitHub repository`: https://github.com/zephyrproject-rtos/zephyr
.. _`GitHub tagged releases`: https://github.com/zephyrproject-rtos/zephyr/tags
.. _`Zephyr 2.7.5`: https://docs.zephyrproject.org/2.7.5/
.. _`Zephyr 3.2.0`: https://docs.zephyrproject.org/3.2.0/
.. _`Zephyr 3.3.0`: https://docs.zephyrproject.org/3.3.0/
