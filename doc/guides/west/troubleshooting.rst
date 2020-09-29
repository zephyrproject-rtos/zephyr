.. _west-troubleshooting:

Troubleshooting West
####################

This page covers common issues with west and how to solve them.

"'west' is not recognized as an internal or external command, operable program or batch file.'
**********************************************************************************************

On Windows, this means that either west is not installed, or your :envvar:`PATH`
environment variable does not contain the directory where pip installed
:file:`west.exe`.

First, make sure you've installed west; see :ref:`west-install`. Then try
running ``west`` from a new ``cmd.exe`` window. If that still doesn't work,
keep reading.

You need to find the directory containing :file:`west.exe`, then add it to your
:envvar:`PATH`. (This :envvar:`PATH` change should have been done for you when
you installed Python and pip, so ordinarily you should not need to follow these
steps.)

Run this command in ``cmd.exe``::

  pip3 show west

Then:

#. Look for a line in the output that looks like ``Location:
   C:\foo\python\python38\lib\site-packages``. The exact location
   will be different on your computer.
#. Look for a file named ``west.exe`` in the ``scripts`` directory
   ``C:\foo\python\python38\scripts``.

   .. important::

      Notice how ``lib\site-packages`` in the ``pip3 show`` output was changed
      to ``scripts``!
#. If you see ``west.exe`` in the ``scripts`` directory, add the full path to
   ``scripts`` to your :envvar:`PATH` using a command like this::

     setx PATH "%PATH%;C:\foo\python\python38\scripts"

   **Do not just copy/paste this command**. The ``scripts`` directory location
   will be different on your system.
#. Close your ``cmd.exe`` window and open a new one. You should be able to run
   ``west``.

"Error: unexpected keyword argument 'requires_workspace'"
*********************************************************

This error occurs on some Linux distributions after upgrading to west 0.7.0 or
later from 0.6.x. For example:

.. code-block:: none

   $ west update
   [... stack trace ...]
   TypeError: __init__() got an unexpected keyword argument 'requires_workspace'

This appears to be a problem with the distribution's pip; see `this comment in
west issue 373`_ for details. Some versions of **Ubuntu** and **Linux Mint** are known to
have this problem. Some users report issues on Fedora as well.

Neither macOS nor Windows users have reported this issue. There have been no
reports of this issue on other Linux distributions, like Arch Linux, either.

.. _this comment in west issue 373:
   https://github.com/zephyrproject-rtos/west/issues/373#issuecomment-583489272

**Workaround 1**: remove the old version, then upgrade:

.. code-block:: none

   $ pip3 show west | grep Location: | cut -f 2 -d ' '
   /home/foo/.local/lib/python3.6/site-packages
   $ rm -r /home/foo/.local/lib/python3.6/site-packages/west
   $ pip3 install --user west==0.7.0

**Workaround 2**: install west in a Python virtual environment

One option is to use the `venv module`_ that's part of the Python 3 standard
library. Some distributions remove this module from their base Python 3
packages, so you may need to do some additional work to get it installed on
your system.

.. _venv module:
   https://docs.python.org/3/library/venv.html

"invalid choice: 'build'" (or 'flash', etc.)
********************************************

If you see an unexpected error like this when trying to run a Zephyr extension
command (like :ref:`west flash <west-flashing>`, :ref:`west build
<west-building>`, etc.):

.. code-block:: none

   $ west build [...]
   west: error: argument <command>: invalid choice: 'build' (choose from 'init', [...])

   $ west flash [...]
   west: error: argument <command>: invalid choice: 'flash' (choose from 'init', [...])

The most likely cause is that you're running the command outside of a
:ref:`west workspace <west-workspace>`. West needs to know where your workspace
is to find :ref:`west-extensions`.

To fix this, you have two choices:

#. Run the command from inside a workspace (e.g. the :file:`zephyrproject`
   directory you created when you :ref:`got started <getting_started>`).

   For example, create your build directory inside the workspace, or run ``west
   flash --build-dir YOUR_BUILD_DIR`` from inside the workspace.

#. Set the :envvar:`ZEPHYR_BASE` :ref:`environment variable <env_vars>` and re-run
   the west extension command. If set, west will use :envvar:`ZEPHYR_BASE` to
   find your workspace.

If you're unsure whether a command is built-in or an extension, run ``west
help`` from inside your workspace. The output prints extension commands
separately, and looks like this for mainline Zephyr:

.. code-block:: none

   $ west help

   built-in commands for managing git repositories:
     init:                 create a west workspace
     [...]

   other built-in commands:
     help:                 get help for west or a command
     [...]

   extension commands from project manifest (path: zephyr):
     build:                compile a Zephyr application
     flash:                flash and run a binary on a board
     [...]

"invalid choice: 'post-init'"
*****************************

If you see this error when running ``west init``:

.. code-block:: none

   west: error: argument <command>: invalid choice: 'post-init'
   (choose from 'init', 'update', 'list', 'manifest', 'diff',
   'status', 'forall', 'config', 'selfupdate', 'help')

Then you have an old version of west installed, and are trying to use it in a
workspace that requires a more recent version.

The easiest way to resolve this issue is to upgrade west and retry as follows:

#. Install the latest west with the ``-U`` option for ``pip3 install`` as shown
   in :ref:`west-install`.

#. Back up any contents of :file:`zephyrproject/.west/config` that you want to
   save. (If you don't have any configuration options set, it's safe to skip
   this step.)

#. Completely remove the :file:`zephyrproject/.west` directory (if you don't,
   you will get the "already in a workspace" error message discussed next).

#. Run ``west init`` again.

"already in an installation"
****************************

You may see this error when running ``west init`` with west 0.6:

.. code-block:: none

   FATAL ERROR: already in an installation (<some directory>), aborting

If this is unexpected and you're really trying to create a new west workspace,
then it's likely that west is using the :envvar:`ZEPHYR_BASE` :ref:`environment
variable <env_vars>` to locate a workspace elsewhere on your system.

This is intentional; it allows you to put your Zephyr applications in
any directory and still use west to build, flash, and debug them, for example.

To resolve this issue, unset :envvar:`ZEPHYR_BASE` and try again.
