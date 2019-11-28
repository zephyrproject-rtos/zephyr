.. _west-troubleshooting:

Troubleshooting West
####################

This page covers common issues with west and how to solve them.

"invalid choice: 'post-init'"
*****************************

If you see this error when running ``west init``:

.. code-block:: none

   west: error: argument <command>: invalid choice: 'post-init'
   (choose from 'init', 'update', 'list', 'manifest', 'diff',
   'status', 'forall', 'config', 'selfupdate', 'help')

Then you have an old version of west installed, and are trying to use it in an
installation that requires a more recent version.

The easiest way to resolve this issue is to upgrade west and retry as follows:

#. Install the latest west with the ``-U`` option for ``pip3 install`` as shown
   in :ref:`west-install`.

#. Back up any contents of :file:`zephyrproject/.west/config` that you want to
   save. (If you don't have any configuration options set, it's safe to skip
   this step.)

#. Completely remove the :file:`zephyrproject/.west` directory (if you don't,
   you will get the "already in an installation" error message discussed next).

#. Run ``west init`` again.

"already in an installation"
****************************

You may see this error when running ``west init``:

.. code-block:: none

   FATAL ERROR: already in an installation (<some directory>), aborting

If this is unexpected and you're really trying to create a new installation,
then it's likely that west is using the :envvar:`ZEPHYR_BASE` :ref:`environment
variable <env_vars>` to locate a west installation elsewhere on your system.
This is intentional behavior; it allows you to put your Zephyr applications in
any directory and still use west.

To resolve this issue, unset :envvar:`ZEPHYR_BASE` and try again.
