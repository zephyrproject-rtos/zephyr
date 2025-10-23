.. _west-troubleshooting:

Troubleshooting West
####################

This page covers common issues with west and how to solve them.

``west update`` fetching failures
*********************************

One good way to troubleshoot fetching issues is to run ``west update`` in
verbose mode, like this:

.. code-block:: shell

   west -v update

The output includes Git commands run by west and their outputs. Look for
something like this:

.. code-block:: none

   === updating your_project (path/to/your/project):
   west.manifest: your_project: checking if cloned
   [...other west.manifest logs...]
   --- your_project: fetching, need revision SOME_SHA
   west.manifest: running 'git fetch ... https://github.com/your-username/your_project ...' in /some/directory

The ``git fetch`` command example in the last line above is what needs to
succeed.

One strategy is to go to ``/path/to/your/project``, copy/paste and run the entire
``git fetch`` command, then debug from there using the documentation for your
credential storage helper.

If you're behind a corporate firewall and may have proxy or other issues,
``curl -v FETCH_URL`` (for HTTPS URLs) or ``ssh -v FETCH_URL`` (for SSH URLs)
may be helpful.

If you can get the ``git fetch`` command to run successfully without prompting
for a password when you run it directly, you will be able to run ``west
update`` without entering your password in that same shell.

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
