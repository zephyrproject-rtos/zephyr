.. _west-multi-repo:

Basics
######

This page introduces west's basic concepts and built-in commands along with
references to further reading.

.. _west-workspace:

Introduction
************

West's built-in commands allow you to work with *projects* (Git
repositories) under a common *workspace* directory. You can create a
workspace using the :ref:`west-init` command.

If you've followed the upstream Zephyr getting started guide, your
workspace looks like this:

.. code-block:: none

   zephyrproject/                 # west topdir
   ├── .west/                     # marks the location of the topdir
   │   └── config                 # per-workspace local configuration file
   ├── zephyr/                    # .git/ repo    │ the manifest repository,
   │   ├── west.yml               # manifest file │ never modified by west
   │   └── [... other files ...]                  │ after creation
   ├── modules/
   │   └── lib/
   │       └── tinycbor/          # .git/ project
   ├── net-tools/                 # .git/ project
   └── [ ... other projects ...]

Above, :file:`zephyrproject` is the name of the workspace's top level
directory, or *topdir*. (The name :file:`zephyrproject` is just an example
-- it could be anything, like ``z``, ``my-zephyr-workspace``, etc.)

The topdir contains the :file:`.west` directory. When west needs to find
the topdir, it searches for :file:`.west`, and uses its parent directory.
The search starts from the current working directory (and starts again from
the location in the :envvar:`ZEPHYR_BASE` environment variable as a
fallback if that fails). The file :file:`.west/config` is the workspace's
:ref:`local configuration file <west-config>`.

Every west workspace contains exactly one *manifest repository*, which is a
Git repository containing a *manifest file*. The location of the manifest
repository is given by the :ref:`manifest.path configuration option
<west-config-index>` in the local configuration file.

For upstream Zephyr, :file:`zephyr` is the manifest repository, but you can
configure west to use any Git repository in the workspace as the manifest
repository. The only requirement is that it contains a valid manifest file.
See :ref:`west-topologies` for information on other options, and
:ref:`west-manifests` for details on the manifest file format.

The manifest file is a YAML file that defines *projects*, which are the
additional Git repositories in the workspace managed by west. The manifest
file is named :file:`west.yml` by default; this can be overridden using the
``manifest.file`` local configuration option.

You use the :ref:`west-update` command to update the workspace's projects
based on the contents of the manifest file. The manifest file controls
things like where the project should be placed within the workspace, what
URL to clone it from if it's missing, and what Git revision should be
checked out locally.

Projects can be located anywhere inside the workspace, but they may not
"escape" it. Project repositories need not be located in subdirectories of
the manifest repository or as immediate subdirectories of the topdir.
However, projects must have paths inside the workspace. (You may replace a
project's repository directory within the workspace with a symbolic link to
elsewhere on your computer, but west will not do this for you.)

A workspace can contain additional Git repositories or other files and
directories not managed by west. West basically ignores anything in the
workspace except :file:`.west`, the manifest repository, and the projects
specified in the manifest file.

For upstream Zephyr, ``tinycbor`` and ``net-tools`` are projects. They are
specified in the manifest file :file:`zephyr/west.yml`. This file specifies
that ``tinycbor`` is located in the :file:`modules/lib/tinycbor` directory
beneath the workspace topdir. By default, the Zephyr :ref:`build system
<build_overview>` uses west to get the locations of all the projects in the
workspace, so any code they contain can be used as :ref:`modules`.

Finally, any repository managed by a west workspace (either the manifest
repository or any project repository) can define :ref:`west-extensions`.
Extensions are extra commands not built into west that you can run when
using that workspace.

The zephyr repository uses this feature to provide Zephyr-specific commands
like :ref:`west build <west-building>`. Defining these as extensions keeps
west's core agnostic to the specifics of any workspace's Zephyr version,
etc.

.. _west-struct:

Structure
*********

West's code is distributed via PyPI in a Python package named ``west``.
This distribution includes a launcher executable, which is also named
``west`` (or ``west.exe`` on Windows).

When west is installed, the launcher is placed by :file:`pip3` somewhere in
the user's filesystem (exactly where depends on the operating system, but
should be on the ``PATH`` :ref:`environment variable <env_vars>`). This
launcher is the command-line entry point to running both built-in commmands
like ``west init``, ``west update``, along with any extensions discovered
in the workspace.

In addition to its command-line interface, you can also use west's Python
APIs directly. See :ref:`west-apis` for details.

.. _west-manifest-rev:

The ``manifest-rev`` branch
***************************

West creates and controls a Git branch named ``manifest-rev`` in each
project. This branch points to the revision that the manifest file
specified for the project at the time :ref:`west-update` was last run.
Other workspace management commands may use ``manifest-rev`` as a reference
point for the upstream revision as of this latest update. Among other
purposes, the ``manifest-rev`` branch allows the manifest file to use SHAs
as project revisions.

Although ``manifest-rev`` is a normal Git branch, west will recreate and/or
reset it on the next update. For this reason, it is **dangerous**
to check it out or otherwise modify it yourself. For instance, any commits
you manually add to this branch may be lost the next time you run ``west
update``. Instead, check out a local branch with another name, and either
rebase it on top of a new ``manifest-rev``, or merge ``manifest-rev`` into
it.

.. note::

   West does not create a ``manifest-rev`` branch in the manifest repository,
   since west does not manage the manifest repository's branches or revisions.

.. _west-built-in-cmds:

Built-in Commands
*****************

This section gives an overview of west's built-in commands.

Some commands are related to Git commands with the same name, but operate
on the entire workspace. For example, ``west diff`` shows local changes in
multiple Git repositories in the workspace.

Some commands take projects as arguments. These arguments can be project
names as specified in the manifest file, or (as a fallback) paths to them
on the local file system. Omitting project arguments to commands which
accept them (such as ``west list``, ``west forall``, etc.) usually defaults
to using all projects in the manifest file plus the manifest repository
itself.

The following documentation does not exhaustively describe all commands.
For additional help, run ``west <command> -h`` (e.g. ``west init -h``).

.. _west-init:

``west init``
=============

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

The ``-m`` option defaults to
``https://github.com/zephyrproject-rtos/zephyr``. The ``--mr`` option
defaults to ``master``. The ``--mf`` option defaults to ``west.yml``. If no
``directory`` is given, the current working directory is used.

**Option 2**: to create a workspace around an existing local manifest
repository, use:

.. code-block:: none

   west init -l [--mf FILE] directory

This creates :file:`.west` **next to** :file:`directory` in the file
system, and sets ``manifest.path`` to ``directory``.

As above, ``--mf`` defaults to ``west.yml``.

The ``west init`` command does not clone any of the projects defined in the
manifest file, regardless of whether ``-l`` is given. To do that, use
``west update``.

.. _west-update:

``west update``
---------------

This command clones and updates the projects specified in the :term:`west
manifest` file.

.. code-block:: none

   west update [-h] [--stats] [-f {always,smart}] [-k] [-r] [PROJECT ...]

By default, this command:

#. Parses the manifest file, usually :file:`west.yml`
#. Clones any project repositories that are not already present locally
#. Fetches any project revisions which are not already pulled from the
   remote
#. Sets each project's :ref:`manifest-rev <west-manifest-rev>` branch to the
   revision specified for that project in the manifest file
#. Checks out each ``manifest-rev`` in local working trees, as `detached
   HEADs <https://git-scm.com/docs/git-checkout#_detached_head>`_

To operate on a subset of projects only, specify them using the ``PROJECT``
positional arguments, which can be either project names as given in the
manifest file, or paths to the local project clones.

To force this command to fetch from project remotes even if the revisions
appear to be available locally, either use ``--fetch always`` or set the
``update.fetch`` :ref:`configuration option <west-config>` to ``"always"``.

For safety, ``west update`` uses ``git checkout --detach`` to check out a
detached ``HEAD`` at the manifest revision for each updated project,
leaving behind any branches which were already checked out. This is
typically a safe operation that will not modify any of your local branches.

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


.. _west-multi-repo-misc:

Other Repository Management Commands
====================================

West has a few more commands for managing the repositories in the
workspace, which are summarized here. Run ``west <command> -h`` for
detailed help.

- ``west list``: print a line of information about each project in the
  manifest, according to a format string
- ``west manifest``: manage the manifest file. See :ref:`west-manifest-cmd`.
- ``west diff``: run ``git diff`` in local project repositories
- ``west status``: run ``git status`` in local project repositories
- ``west forall``: run an arbitrary command in local project repositories

Additional Commands
===================

Finally, here is a summary of other built-in commands.

- ``west config``: get or set :ref:`configuration options <west-config>`
- ``west topdir``: print the top level directory of the west workspace
- ``west help``: get help about a command, or print information about all
  commands in the workspace, including :ref:`west-extensions`

.. _west-topologies:

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

   west-workspace/
   │
   ├── application/         # .git/     │
   │   ├── CMakeLists.txt               │
   │   ├── prj.conf                     │  never modified by west
   │   ├── src/                         │
   │   │   └── main.c                   │
   │   └── west.yml         # main manifest with optional import(s) and override(s)
   │                                    │
   ├── modules/
   │   └── lib/
   │       └── tinycbor/    # .git/ project from either the main manifest or some import.
   │
   └── zephyr/              # .git/ project
       └── west.yml         # This can be partially imported with lower precedence or ignored.
                            # Only the 'manifest-rev' version can be imported.


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

   west-workspace/
   ├── app1/               # .git/ project
   │   ├── CMakeLists.txt
   │   ├── prj.conf
   │   └── src/
   │       └── main.c
   ├── app2/               # .git/ project
   │   ├── CMakeLists.txt
   │   ├── prj.conf
   │   └── src/
   │       └── main.c
   ├── manifest-repo/      # .git/ never modified by west
   │   └── west.yml        # main manifest with optional import(s) and override(s)
   ├── modules/
   │   └── lib/
   │       └── tinycbor/   # .git/ project from either the main manifest or
   │                       #       frome some import
   │
   └── zephyr/             # .git/ project
       └── west.yml        # This can be partially imported with lower precedence or ignored.
                           # Only the 'manifest-rev' version can be imported.


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

Private repositories
********************

You can use west to fetch from private repositories. There is nothing
west-specific about this.

The ``west update`` command essentially runs ``git fetch YOUR_PROJECT_URL``
when a project's ``manifest-rev`` branch must be updated to a newly fetched
commit. It's up to your environment to make sure the fetch succeeds.

You can either enter the password manually or use any of the `credential
helpers built in to Git`_. Since Git has credential storage built in, there is
no need for a west-specific feature.

The following sections cover common cases for running ``west update`` without
having to enter your password, as well as how to troubleshoot issues.

.. _credential helpers built in to Git:
   https://git-scm.com/docs/gitcredentials

Fetching via HTTPS
==================

On Windows when fetching from GitHub, recent versions of Git prompt you for
your GitHub password in a graphical window once, then store it for future use
(in a default installation). Passwordless fetching from GitHub should therefore
work "out of the box" on Windows after you have done it once.

In general, you can store your credentials on disk using the "store" git
credential helper. See the `git-credential-store`_ manual page for details.

To use this helper for all the repositories in your workspace, run:

.. code-block:: shell

   west forall -c "git config credential.helper store"

To use this helper on just the projects ``foo`` and ``bar``, run:

.. code-block:: shell

   west forall -c "git config credential.helper store" foo bar

To use this helper by default on your computer, run:

.. code-block:: shell

   git config --global credential.helper store

On GitHub, you can set up a `personal access token`_ to use in place of your
account password. (This may be required if your account has two-factor
authentication enabled, and may be preferable to storing your account password
in plain text even if two-factor authentication is disabed.)

If you don't want to store any credentials on the file system, you can store
them in memory temporarily using `git-credential-cache`_ instead.

.. _git-credential-store:
   https://git-scm.com/docs/git-credential-store#_examples
.. _git-credential-cache:
   https://git-scm.com/docs/git-credential-cache
.. _personal access token:
   https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token

Fetching via SSH
================

If your SSH key has no password, fetching should just work. If it does have a
password, you can avoid entering it manually every time using `ssh-agent`_.

On GitHub, see `Connecting to GitHub with SSH`_ for details on configuration
and key creation.

.. _ssh-agent:
   https://www.ssh.com/ssh/agent
.. _Connecting to GitHub with SSH:
   https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh

Troubleshooting
===============

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
succeed. Go to ``/some/directory``, copy/paste and run the entire ``git fetch``
command, then debug from there using the documentation for your credential
storage helper.

If you can get the ``git fetch`` command to run successfully without prompting
for a password when you run it directly, you will be able to run ``west
update`` without entering your password in that same shell.

.. _PyPI:
   https://pypi.org/project/west/

.. _Zephyr issue #6770:
   https://github.com/zephyrproject-rtos/zephyr/issues/6770

.. _namespace package:
   https://www.python.org/dev/peps/pep-0420/
