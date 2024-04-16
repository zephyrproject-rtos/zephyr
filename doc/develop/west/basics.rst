.. _west-basics:

Basics
######

This page introduces west's basic concepts and provides references to further
reading.

West's built-in commands allow you to work with *projects* (Git
repositories) under a common *workspace* directory.

Example workspace
*****************

If you've followed the upstream Zephyr getting started guide, your
workspace looks like this:

.. code-block:: none

   zephyrproject/                 # west topdir
   ├── .west/                     # marks the location of the topdir
   │   └── config                 # per-workspace local configuration file
   │
   │   # The manifest repository, never modified by west after creation:
   ├── zephyr/                    # .git/ repo
   │   ├── west.yml               # manifest file
   │   └── [... other files ...]
   │
   │   # Projects managed by west:
   ├── modules/
   │   └── lib/
   │       └── zcbor/             # .git/ project
   ├── net-tools/                 # .git/ project
   └── [ ... other projects ...]

.. _west-workspace:

Workspace concepts
******************

Here are the basic concepts you should understand about this structure.
Additional details are in :ref:`west-workspaces`.

topdir
  Above, :file:`zephyrproject` is the name of the workspace's top level
  directory, or *topdir*. (The name :file:`zephyrproject` is just an example
  -- it could be anything, like ``z``, ``my-zephyr-workspace``, etc.)

  You'll typically create the topdir and a few other files and directories
  using :ref:`west init <west-init-basics>`.

.west directory
  The topdir contains the :file:`.west` directory. When west needs to find
  the topdir, it searches for :file:`.west`, and uses its parent directory.
  The search starts from the current working directory (and starts again from
  the location in the :envvar:`ZEPHYR_BASE` environment variable as a
  fallback if that fails).

configuration file
  The file :file:`.west/config` is the workspace's :ref:`local configuration
  file <west-config>`.

manifest repository
  Every west workspace contains exactly one *manifest repository*, which is a
  Git repository containing a *manifest file*. The location of the manifest
  repository is given by the :ref:`manifest.path configuration option
  <west-config-index>` in the local configuration file.

  For upstream Zephyr, :file:`zephyr` is the manifest repository, but you can
  configure west to use any Git repository in the workspace as the manifest
  repository. The only requirement is that it contains a valid manifest file.
  See :ref:`west-topologies` for information on other options, and
  :ref:`west-manifests` for details on the manifest file format.

manifest file
  The manifest file is a YAML file that defines *projects*, which are the
  additional Git repositories in the workspace managed by west. The manifest
  file is named :file:`west.yml` by default; this can be overridden using the
  ``manifest.file`` local configuration option.

  You use the :ref:`west update <west-update-basics>` command to update the
  workspace's projects based on the contents of the manifest file.

projects
  Projects are Git repositories managed by west. Projects are defined in the
  manifest file and can be located anywhere inside the workspace. In the above
  example workspace, ``zcbor`` and ``net-tools`` are projects.

  By default, the Zephyr :ref:`build system <build_overview>` uses west to get
  the locations of all the projects in the workspace, so any code they contain
  can be used as :ref:`modules`. Note however that modules and projects
  :ref:`are conceptually different <modules-vs-projects>`.

extensions
  Any repository known to west (either the manifest repository or any project
  repository) can define :ref:`west-extensions`. Extensions are extra west
  commands you can run when using that workspace.

  The zephyr repository uses this feature to provide Zephyr-specific commands
  like :ref:`west build <west-building>`. Defining these as extensions keeps
  west's core agnostic to the specifics of any workspace's Zephyr version,
  etc.

ignored files
  A workspace can contain additional Git repositories or other files and
  directories not managed by west. West basically ignores anything in the
  workspace except :file:`.west`, the manifest repository, and the projects
  specified in the manifest file.

west init and west update
*************************

The two most important workspace-related commands are ``west init`` and ``west
update``.

.. _west-init-basics:

``west init`` basics
--------------------

This command creates a west workspace.

.. important::

   West doesn't change your manifest repository contents after ``west init`` is
   run. Use ordinary Git commands to pull new versions, etc.

You will typically run it once, like this:

.. code-block:: shell

   west init -m https://github.com/zephyrproject-rtos/zephyr --mr v2.5.0 zephyrproject

This will:

#. Create the topdir, :file:`zephyrproject`, along with
   :file:`.west` and :file:`.west/config` inside it
#. Clone the manifest repository from
   https://github.com/zephyrproject-rtos/zephyr, placing it into
   :file:`zephyrproject/zephyr`
#. Check out the ``v2.5.0`` git tag in your local zephyr clone
#. Set ``manifest.path`` to ``zephyr`` in :file:`.west/config`
#. Set ``manifest.file`` to ``west.yml``

Your workspace is now almost ready to use; you just need to run ``west update``
to clone the rest of the projects into the workspace to finish.

For more details, see :ref:`west-init`.

.. _west-update-basics:

``west update`` basics
----------------------

This command makes sure your workspace contains Git repositories matching the
projects in the manifest file.

.. important::

   Whenever you check out a different revision in your manifest repository, you
   should run ``west update`` to make sure your workspace contains the
   project repositories the new revision expects.

The ``west update`` command reads the manifest file's contents by:

#. Finding the topdir. In the ``west init`` example above, that
   means finding :file:`zephyrproject`.
#. Loading :file:`.west/config` in the topdir to read the ``manifest.path``
   (e.g. ``zephyr``) and ``manifest.file`` (e.g. ``west.yml``) options.
#. Loading the manifest file given by these options (e.g.
   :file:`zephyrproject/zephyr/west.yml`).

It then uses the manifest file to decide where missing projects should be
placed within the workspace, what URLs to clone them from, and what Git
revisions should be checked out locally. Project repositories which already
exist are updated in place by fetching and checking out their respective Git
revisions in the manifest file.

For more details, see :ref:`west-update`.

Other built-in commands
***********************

See :ref:`west-built-in-cmds`.

.. _west-zephyr-extensions:

Zephyr Extensions
*****************

See the following pages for information on Zephyr's extension commands:

- :ref:`west-build-flash-debug`
- :ref:`west-sign`
- :ref:`west-zephyr-ext-cmds`
- :ref:`west-shell-completion`

Troubleshooting
***************

See :ref:`west-troubleshooting`.
