.. _west-built-in-cmds:

Built-in commands
#################

This page describes west's built-in commands, some of which were introduced in
:ref:`west-basics`, in more detail.

Some commands are related to Git commands with the same name, but operate
on the entire workspace. For example, ``west diff`` shows local changes in
multiple Git repositories in the workspace.

Some commands take projects as arguments. These arguments can be project
names as specified in the manifest file, or (as a fallback) paths to them
on the local file system. Omitting project arguments to commands which
accept them (such as ``west list``, ``west forall``, etc.) usually defaults
to using all projects in the manifest file plus the manifest repository
itself.

For additional help, run ``west <command> -h`` (e.g. ``west init -h``).

.. _west-init:

west init
*********

This command creates a west workspace. It can be used in two ways:

1. Cloning a new manifest repository from a remote URL
2. Creating a workspace around an existing local manifest repository

**Option 1**: to clone a new manifest repository from a remote URL, use:

.. code-block:: none

   west init [-m URL] [--mr REVISION] [--mf FILE] [directory]

The new workspace is created in the given :file:`directory`, creating a new
:file:`.west` inside this directory. You can give the manifest URL using
the ``-m`` switch, the initial revision to check out using ``--mr``, and
the location of the manifest file within the repository using ``--mf``.

For example, running:

.. code-block:: shell

   west init -m https://github.com/zephyrproject-rtos/zephyr --mr v1.14.0 zp

would clone the upstream official zephyr repository into :file:`zp/zephyr`,
and check out the ``v1.14.0`` release. This command creates
:file:`zp/.west`, and set the ``manifest.path`` :ref:`configuration option
<west-config>` to ``zephyr`` to record the location of the manifest
repository in the workspace. The default manifest file location is used.

The ``-m`` option defaults to ``https://github.com/zephyrproject-rtos/zephyr``.
The ``--mf`` option defaults to ``west.yml``. Since west v0.10.1, west will use
the default branch in the manifest repository unless the ``--mr`` option
is used to override it. (In prior versions, ``--mr`` defaulted to ``master``.)

If no ``directory`` is given, the current working directory is used.

**Option 2**: to create a workspace around an existing local manifest
repository, use:

.. code-block:: none

   west init -l [--mf FILE] directory

This creates :file:`.west` **next to** :file:`directory` in the file
system, and sets ``manifest.path`` to ``directory``.

As above, ``--mf`` defaults to ``west.yml``.

**Reconfiguring the workspace**:

If you change your mind later, you are free to change ``manifest.path`` and
``manifest.file`` using :ref:`west-config-cmd` after running ``west init``.
Just be sure to run ``west update`` afterwards to update your workspace to
match the new manifest file.

.. _west-update:

west update
***********

.. code-block:: none

   west update [-f {always,smart}] [-k] [-r]
               [--group-filter FILTER] [--stats] [PROJECT ...]

**Which projects are updated:**

By default, this command parses the manifest file, usually
:file:`west.yml`, and updates each project specified there.
If your manifest uses :ref:`project groups <west-manifest-groups>`, then
only the active projects are updated.

To operate on a subset of projects only, give ``PROJECT`` argument(s). Each
``PROJECT`` is either a project name as given in the manifest file, or a
path that points to the project within the workspace. If you specify
projects explicitly, they are updated regardless of whether they are active.

**Project update procedure:**

For each project that is updated, this command:

#. Initializes a local Git repository for the project in the workspace, if
   it does not already exist
#. Inspects the project's ``revision`` field in the manifest, and fetches
   it from the remote if it is not already available locally
#. Sets the project's :ref:`manifest-rev <west-manifest-rev>` branch to the
   commit specified by the revision in the previous step
#. Checks out ``manifest-rev`` in the local working copy as a `detached
   HEAD <https://git-scm.com/docs/git-checkout#_detached_head>`_
#. If the manifest file specifies a :ref:`submodules
   <west-manifest-submodules>` key for the project, recursively updates
   the project's submodules as described below.

To avoid unnecessary fetches, ``west update`` will not fetch project
``revision`` values which are Git SHAs or tags that are already available
locally. This is the behavior when the ``-f`` (``--fetch``) option has its
default value, ``smart``. To force this command to fetch from project remotes
even if the revisions appear to be available locally, either use ``-f always``
or set the ``update.fetch`` :ref:`configuration option <west-config>` to
``always``. SHAs may be given as unique prefixes as long as they are acceptable
to Git [#fetchall]_.

If the project ``revision`` is a Git ref that is neither a tag nor a SHA (i.e.
if the project is tracking a branch), ``west update`` always fetches,
regardless of ``-f`` and ``update.fetch``.

Some branch names might look like short SHAs, like ``deadbeef``. West treats
these like SHAs. You can disambiguate by prefixing the ``revision`` value with
``refs/heads/``, e.g. ``revision: refs/heads/deadbeef``.

For safety, ``west update`` uses ``git checkout --detach`` to check out a
detached ``HEAD`` at the manifest revision for each updated project,
leaving behind any branches which were already checked out. This is
typically a safe operation that will not modify any of your local branches.

However, if you had added some local commits onto a previously detached
``HEAD`` checked out by west, then git will warn you that you've left
behind some commits which are no longer referred to by any branch. These
may be garbage-collected and lost at some point in the future. To avoid
this if you have local commits in the project, make sure you have a local
branch checked out before running ``west update``.

If you would rather rebase any locally checked out branches instead, use
the ``-r`` (``--rebase``) option.

If you would like ``west update`` to keep local branches checked out as
long as they point to commits that are descendants of the new
``manifest-rev``, use the ``-k`` (``--keep-descendants``) option.

.. note::

   ``west update --rebase`` will fail in projects that have git conflicts
   between your branch and new commits brought in by the manifest. You
   should immediately resolve these conflicts as you usually do with
   ``git``, or you can use ``git -C <project_path> rebase --abort`` to
   ignore incoming changes for the moment.

   With a clean working tree, a plain ``west update`` never fails
   because it does not try to hold on to your commits and simply
   leaves them aside.

   ``west update --keep-descendants`` offers an intermediate option that
   never fails either but does not treat all projects the same:

   - in projects where your branch diverged from the incoming commits, it
     does not even try to rebase and leaves your branches behind just like a
     plain ``west update`` does;
   - in all other projects where no rebase or merge is needed it keeps
     your branches in place.

**One-time project group manipulation:**

The ``--group-filter`` option can be used to change which project groups
are enabled or disabled for the duration of a single ``west update`` command.
See :ref:`west-manifest-groups` for details on the project group feature.

The ``west update`` command behaves as if the ``--group-filter`` option's
value were appended to the ``manifest.group-filter``
:ref:`configuration option <west-config-index>`.

For example, running ``west update --group-filter=+foo,-bar`` would behave
the same way as if you had temporarily appended the string ``"+foo,-bar"``
to the value of ``manifest.group-filter``, run ``west update``, then restored
``manifest.group-filter`` to its original value.

Note that using the syntax ``--group-filter=VALUE`` instead of
``--group-filter VALUE`` avoids issues parsing command line options
if you just want to disable a single group, e.g. ``--group-filter=-bar``.

**Submodule update procedure:**

If a project in the manifest has a ``submodules`` key, the submodules are
updated as follows, depending on the value of the ``submodules`` key.

If the project has ``submodules: true``, west first synchronizes the project's
submodules with:

.. code-block::

   git submodule sync --recursive

West then runs one of the following in the project repository, depending on
whether you run ``west update`` with the ``--rebase`` option or without it:

.. code-block::

   # without --rebase, e.g. "west update":
   git submodule update --init --checkout --recursive

   # with --rebase, e.g. "west update --rebase":
   git submodule update --init --rebase --recursive

Otherwise, the project has ``submodules: <list-of-submodules>``. In this
case, west synchronizes the project's submodules with:

.. code-block::

   git submodule sync --recursive -- <submodule-path>

Then it updates each submodule in the list as follows, depending on whether you
run ``west update`` with the ``--rebase`` option or without it:

.. code-block::

   # without --rebase, e.g. "west update":
   git submodule update --init --checkout --recursive <submodule-path>

   # with --rebase, e.g. "west update --rebase":
   git submodule update --init --rebase --recursive <submodule-path>

The ``git submodule sync`` commands are skipped if the
``update.sync-submodules`` :ref:`west-config` option is false.

.. _west-built-in-misc:

Other project commands
**********************

West has a few more commands for managing the projects in the
workspace, which are summarized here. Run ``west <command> -h`` for
detailed help.

- ``west list``: print a line of information about each project in the
  manifest, according to a format string
- ``west manifest``: manage the manifest file. See :ref:`west-manifest-cmd`.
- ``west diff``: run ``git diff`` in local project repositories
- ``west status``: run ``git status`` in local project repositories
- ``west forall``: run an arbitrary command in local project repositories

Other built-in commands
***********************

Finally, here is a summary of other built-in commands.

- ``west config``: get or set :ref:`configuration options <west-config>`
- ``west topdir``: print the top level directory of the west workspace
- ``west help``: get help about a command, or print information about all
  commands in the workspace, including :ref:`west-extensions`

.. rubric:: Footnotes

.. [#fetchall]

   West may fetch all refs from the Git server when given a SHA as a revision.
   This is because some Git servers have historically not allowed fetching
   SHAs directly.
