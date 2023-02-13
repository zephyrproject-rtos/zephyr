.. _zephyr_release_notes:

Release Notes
#############

Zephyr project is provided as source code and build scripts for different
target architectures and configurations, and not as a binary image. Updated
versions of the Zephyr project are released approximately every three-months.

All Zephyr project source code is maintained in a `GitHub repository`_.

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
