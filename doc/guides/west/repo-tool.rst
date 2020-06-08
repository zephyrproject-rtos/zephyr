.. _west-multi-repo:

Multiple Repository Management
##############################

This page introduces basic concepts related to west and its multiple repository
management features, and gives an overview of the associated commands. See
:ref:`west-history` and `Zephyr issue #6770`_ for additional discussion,
rationale, and motivation.

.. note::

   West's multi-repo commands are meant to augment Git in minor ways for
   multi-repo work, not to replace it. For tasks that only operate on one
   repository, just use plain Git commands.

.. _west-workspace:

Introduction
************

West's built-in commands allow you to work with projects composed of multiple
Git repositories installed under a common parent directory, which we call a
*west workspace* (before west 0.7, this was called a *west installation*). This
works similarly to `Git Submodules
<https://git-scm.com/book/en/v2/Git-Tools-Submodules>`_ and Google's `repo
<https://gerrit.googlesource.com/git-repo/>`_.

A west workspace is the result of running the ``west init`` command, which
is described in more detail below. For upstream Zephyr, the workspace looks
like this:

.. code-block:: none

   zephyrproject
   ├── .west
   │   └── config
   ├── zephyr
   │   ├── west.yml
   │   └── [... other files ...]
   ├── modules
   │   └── lib
   │       └── tinycbor
   ├── net-tools
   └── [ ... other projects ...]

Above, :file:`zephyrproject` is the name of the west workspace's root
directory. This name is just an example -- it could be anything, like ``z``,
``my-zephyr-workspace``, etc.  The file :file:`.west/config` is the
workspace's :ref:`local configuration file <west-config>`.

Every west workspace contains exactly one *manifest repository*, which is a
Git repository containing a file named :file:`west.yml`, which is the *west
manifest*. The location of the manifest repository is given by the
:ref:`manifest.path configuration option <west-config-index>` in the local
configuration file. The manifest file, along with west's configuration files,
controls the workspace's behavior. For upstream Zephyr, :file:`zephyr` is
the manifest repository, but you can configure west to use any Git repository
in the workspace as the manifest repository. The only requirement is that it
contains a valid manifest file. See :ref:`west-manifests` for more details on
what this means.

Both of the :file:`tinycbor` and :file:`net-tools` directories are *projects*
managed by west, and configured in the manifest file. A west workspace can
contain arbitrarily many projects. As shown above, projects can be located
anywhere in the workspace. They don't have to be subdirectories of the
manifest directory, and they can be inside of arbitrary subdirectories inside
the workspace's root directory. By default, the Zephyr build system uses
west to get the locations of all the projects in the workspace, so any code
they contain can be used by applications. This behavior can be overridden using
the ``ZEPHYR_MODULES`` CMake variable; see :ref:`modules` for details.

Finally, any repository managed by a west workspace can contain
:ref:`extension commands <west-extensions>`, which are extra west commands
provided by that project. This includes the manifest repository and any project
repository.

Topologies supported
********************

The following are example source code topologies supported by west.

- T1: star topology, zephyr is the manifest repository
- T2: star topology, a Zephyr application is the manifest repository
- T3: forest topology, freestanding manifest repository

T1: Star topology, zephyr is the manifest repository
====================================================

- The zephyr repository acts as the central repository and specifies
  its :ref:`modules` in its :file:`west.yml`
- Analogy with existing mechanisms: Git submodules with zephyr as the
  super-project

This is the default. See :ref:`west-workspace` for how mainline Zephyr is an
example of this topology.

.. _west-t2:

T2: Star topology, application is the manifest repository
=========================================================

- Useful for those focused on a single application
- A repository containing a Zephyr application acts as the central repository
  and names other projects required to build it in its :file:`west.yml`. This
  includes the zephyr repository and any modules.
- Analogy with existing mechanisms: Git submodules with the application as
  the super-project, zephyr and other projects as submodules

A workspace using this topology looks like this:

.. code-block:: none

   west-workspace
   ├── application
   │   ├── CMakeLists.txt
   │   ├── prj.conf
   │   ├── src
   │   │   └── main.c
   │   └── west.yml
   ├── modules
   │   └── lib
   │       └── tinycbor
   └── zephyr

Here is an example :file:`application/west.yml` which uses
:ref:`west-manifest-import`, available since west 0.7, to import Zephyr v2.2.0
and its modules into the application manifest file:

.. code-block:: yaml

   # Example T2 west.yml, using manifest imports.
   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     projects:
       - name: zephyr
         remote: zephyrproject-rtos
         revision: v2.2.0
         import: true
     self:
       path: application

You can still selectively "override" individual Zephyr modules if you use
``import:`` in this way; see :ref:`west-manifest-ex1.3` for an example.

Another way to do the same thing is to copy/paste :file:`zephyr/west.yml`
to :file:`application/west.yml`, adding an entry for the zephyr
project itself, like this:

.. code-block:: yaml

   # Equivalent to the above, but with manually maintained Zephyr modules.
   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     defaults:
       remote: zephyrproject-rtos
     projects:
       - name: zephyr
         revision: v2.2.0
         west-commands: scripts/west-commands.yml
       - name: net-tools
         revision: some-sha-goes-here
         path: tools/net-tools
       # ... other Zephyr modules go here ...
     self:
       path: application

(The ``west-commands`` is there for :ref:`west-build-flash-debug` and other
Zephyr-specific :ref:`west-extensions`. It's not necessary when using
``import``.)

The main advantage to using ``import`` is not having to track the revisions of
imported projects separately. In the above example, using ``import`` means
Zephyr's :ref:`module <modules>` versions are automatically determined from the
:file:`zephyr/west.yml` revision, instead of having to be copy/pasted (and
maintained) on their own.

T3: Forest topology
===================

- Useful for those supporting multiple independent applications or downstream
  distributions with no "central" repository
- A dedicated manifest repository which contains no Zephyr source code,
  and specifies a list of projects all at the same "level"
- Analogy with existing mechanisms: Google repo-based source distribution

A workspace using this topology looks like this:

.. code-block:: none

   west-workspace
   ├── app1
   │   ├── CMakeLists.txt
   │   ├── prj.conf
   │   └── src
   │       └── main.c
   ├── app2
   │   ├── CMakeLists.txt
   │   ├── prj.conf
   │   └── src
   │       └── main.c
   ├── manifest-repo
   │   └── west.yml
   ├── modules
   │   └── lib
   │       └── tinycbor
   └── zephyr

Here is an example T3 :file:`manifest-repo/west.yml` which uses
:ref:`west-manifest-import`, available since west 0.7, to import Zephyr
v2.2.0 and its modules, then add the ``app1`` and ``app2`` projects:

.. code-block:: yaml

   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
       - name: your-git-server
         url-base: https://git.example.com/your-company
     defaults:
       remote: your-git-server
     projects:
       - name: zephyr
         remote: zephyrproject-rtos
         revision: v2.2.0
         import: true
       - name: app1
         revision: SOME_SHA_OR_BRANCH_OR_TAG
       - name: app2
         revision: ANOTHER_SHA_OR_BRANCH_OR_TAG
     self:
       path: manifest-repo

You can also do this "by hand" by copy/pasting :file:`zephyr/west.yml`
as shown :ref:`above <west-t2>` for the T2 topology, with the same caveats.

.. _west-struct:

West Structure
**************

West's code is distributed via PyPI in a Python package named ``west``. See
:ref:`west-apis` for API documentation.

This distribution also includes a launcher executable, also named ``west``.
When west is installed, the launcher is placed by :file:`pip3` somewhere in the
user's filesystem (exactly where depends on the operating system, but should be
on the ``PATH`` :ref:`environment variable <env_vars>`). This launcher is the
command-line entry point.

.. _west-manifest-rev:

The ``manifest-rev`` branch
***************************

West creates a branch named ``manifest-rev`` in each project, pointing to the
commit the project's revision resolves to. The branch is updated whenever
project data is fetched by ``west update``. Other multi-repo commands also use
``manifest-rev`` as a reference for the upstream revision as of the most recent
update. See :ref:`west-multi-repo-cmds`, below, for more information.

``manifest-rev`` is a normal Git branch, but if you delete or otherwise modify
it, west will recreate and/or reset it as if with ``git reset --hard`` on the
next update (though ``git update-ref`` is used internally). For this reason, it
is normally a **bad idea to modify it yourself**. ``manifest-rev`` was added to
allow SHAs as project revisions in the manifest, and to give a consistent
reference for the current upstream revision regardless of how the manifest
changes over time.

.. note::

   West does not create a ``manifest-rev`` branch in the manifest repository,
   since west does not manage the manifest repository's branches or revisions.

.. _west-multi-repo-cmds:

Multi-Repo Commands
*******************

This section gives a quick overview of the multi-repo commands, split up by
functionality. Some commands loosely mimic the corresponding Git command, but
in a multi-repo context (e.g. ``west diff`` shows local changes on all
repositories).

Project arguments can be the names of projects in the manifest, or (as
fallback) paths to them. Omitting project arguments to commands which accept a
list of projects (such as ``west list``, ``west forall``, etc.) usually
defaults to using all projects in the manifest file plus the manifest
repository itself.

For help on individual commands, run ``west <command> -h`` (e.g. ``west diff
-h``).

Main Commands
=============

The ``west init`` and ``west update`` multi-repo commands are the most
important to understand.

- ``west init [-l] [-m URL] [--mr REVISION] [PATH]``: create a west
  workspace in directory :file:`PATH` (i.e. :file:`.west` etc. will be
  created there). If the ``PATH`` argument is not given, the current working
  directory is used. This command does not clone any of the projects in the
  manifest; that is done the next time ``west update`` is run.

  This command can be invoked in two ways:

  1. If you already have a local clone of the zephyr repository and want to
     create a west workspace around it, you can use the ``-l`` switch to
     pass its path to west, as in: ``west init -l path/to/zephyr``. This is
     the only reason to use ``-l``.

  2. Otherwise, omit ``-l`` to create a new workspace from a remote manifest
     repository. You can give the manifest URL using the ``-m`` switch, and its
     revision using ``--mr``. For example, invoking west with: ``west init -m
     https://github.com/zephyrproject-rtos/zephyr --mr v1.15.0`` would clone
     the upstream official zephyr repository at the tagged release v1.15.0
     (``-m`` defaults to https://github.com/zephyrproject-rtos/zephyr, and
     the ``-mr`` default is overridden to ``v1.15.0``).

- ``west update [--fetch {always,smart}] [--rebase] [--keep-descendants]
  [PROJECT ...]``: clone and update the specified projects based
  on the current :term:`west manifest`.

  By default, this command:

  #. Parses the manifest file, :file:`west.yml`
  #. Clones any project repositories that are not already present locally
  #. Fetches any project revisions in the manifest file which are not already
     pulled from the remote
  #. Sets each project's :ref:`manifest-rev <west-manifest-rev>` branch to the
     current manifest revision
  #. Checks out those revisions in local working trees

  To operate on a subset of projects only, specify them using the ``PROJECT``
  positional arguments, which can be either project names as given in the
  manifest file, or paths to the local project clones.

  To force this command to fetch from project remotes even if the revisions
  appear to be available locally, either use ``--fetch always`` or set the
  ``update.fetch`` :ref:`configuration option <west-config>` to ``"always"``.

  For safety, ``west update`` uses ``git checkout --detach`` to check out a
  detached ``HEAD`` at the manifest revision for each updated project, leaving
  behind any branches which were already checked out. This is typically a safe
  operation that will not modify any of your local branches. See the help for
  the ``--rebase`` / ``-r`` and ``--keep-descendants`` / ``-k`` options for
  ways to influence this.

.. _west-multi-repo-misc:

Miscellaneous Commands
======================

West has a few more commands for managing the multi-repo, which are briefly
discussed here. Run ``west <command> -h`` for detailed help.

- ``west forall -c COMMAND [PROJECT ...]``: Runs the shell command ``COMMAND``
  within the top-level repository directory of each of the specified projects
  (default: all cloned projects). If ``COMMAND`` consists of more than one
  word, it must be quoted to prevent it from being split up by the shell.

  To run an arbitrary Git command in each project, use something like ``west
  forall -c 'git <command> --options'``. Note that ``west forall`` can be used
  to run any command, though, not just Git commands.

- ``west help <command>``: this is equivalent to ``west <command> -h``.

- ``west list [-f FORMAT] [PROJECT ...]``: Lists project information from the
  manifest file, such as URL, revision, path, etc. The printed information can
  be controlled using the ``-f`` option.

- ``west manifest``: Manipulates manifest files. See :ref:`west-manifest-cmd`.

- ``west manifest --validate``: Ensure the current manifest file is
  well-formed. Print information about what's wrong and fail the process in
  case of error.

- ``west diff [PROJECT ...]``: Runs a multi-repo ``git diff``
  for the specified projects.

- ``west status [PROJECT ...]``: Like ``west diff``, for
  running ``git status``.

- ``west topdir``: Prints the top directory of the west workspace.

.. _PyPI:
   https://pypi.org/project/west/

.. _Zephyr issue #6770:
   https://github.com/zephyrproject-rtos/zephyr/issues/6770

.. _namespace package:
   https://www.python.org/dev/peps/pep-0420/
