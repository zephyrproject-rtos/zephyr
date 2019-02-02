.. _west-multi-repo:

Multiple Repository Management
##############################

Introduction
************

West includes a set of commands for working with projects composed of multiple
Git repositories installed under a common parent directory (a *west
installation*), similar to how `Git Submodules
<https://git-scm.com/book/en/v2/Git-Tools-Submodules>`_ and Google's `repo
<https://gerrit.googlesource.com/git-repo/>`_ work.

The rest of this page introduces these multi-repo concepts and gives an
overview of the associated west commands, along with an example workflow. See
`Zephyr issue #6770`_ for additional background and discussion.

.. note::

   The multi-repo commands are meant to augment Git in minor ways for
   multi-repo work, not replace it. For tasks that aren't multi-repo-related,
   use plain Git commands.
   This page explains what the west multi-repo commands do behind the scenes.

A west installation is the result of running the ``west init`` command to
either create a new installation or convert a standalone zephyr repository into
a multi-repo installation. When using upstream Zephyr, it looks like this:

.. code-block:: console

   └── zephyrproject/
       ├── .west/
       │   ├── config
       │   └── west/
       ├── zephyr/
       │   └── west.yml
       ├── a-project
       └── ...

Above, :file:`zephyrproject` is the name of the west installation's common
parent directory, and :file:`a-project` is another project managed by west in
the installation. The file :file:`.west/config` is the installation's west
configuration file. The directory :file:`.west/west` is a clone of the west
repository itself; more details on why that is needed are given below.

Every west installation contains a *manifest repository*, which is a Git
repository containing a file named :file:`west.yml`. This file, along with
what's in :file:`.west`, controls the installation's behavior. In the above
example, zephyr is the manifest repository. However, as you'll see below, any
repository in an installation can be the manifest repository; it doesn't have
to be zephyr. However, every installation has exactly one manifest repository;
its location is specified in :file:`.west/config`. Alongside the manifest
repository are *projects*, which are Git repositories specified by
:file:`west.yml`.

Requirements
************

Although the motivation behind splitting the Zephyr codebase into multiple
repositories is outside of the scope of this page, the fundamental requirements
along with a clear justification of the choice not to use existing tools and
instead develop a new one, do belong here.
At the most fundamental level, the requirements for a transition to multiple
repositories in the Zephyr Project are:

* **R1**: Keep externally maintained code outside of the main repository
* **R2**: Provide a tool that both Zephyr users and distributors can make use of
  to benefit from and extend the functionality above
* **R3**: Allow overriding or removing repositories without having to make changes
  to the zephyr repository
* **R4**: Support both continuous tracking and commit-based (bisectable) project
  updating

Topologies supported
********************

The requirements above lead us to the three main source code organization
topologies that we intend to address with west:

* **T1**: Star topology with zephyr as the manifest repository:

  - The zephyr repository acts as the central repository and includes a
    complete list of projects in itself
  - Default (upstream) configuration
  - Analogy with existing mechanisms: Git submodules with zephyr as the
    superproject

* **T2**: Star topology with an application repository as the manifest repository

  - The application repository acts as the central repository and includes a
    complete list of projects in itself
  - Useful for downstream distributions focused in a single application
    repository
  - Analogy with existing mechanisms: Git submodules with the application as
    the superproject, zephyr as a submodule

* **T3**: Forest topology with a set of trees all at the same level

  - A dedicated manifest repository contains only a list of repositories
  - Useful for downstream distributions that track the latest ``HEAD`` on all
    repositories
  - Analogy with existing mechanisms: Google repo-based distribution

Rationale for a custom tool
***************************

During the different stages of design and development for west, using already
established and proven multi-repo technologies was considered multiple times.
After careful analysis, no existing tool or mechanism was found suitable for
Zephyr’s use case set and requirements. The following two tools were examined
in detail:

* Google repo

  - The tool is Python 2 only
  - Does not play well with Windows
  - It is poorly documented and maintained
  - It does not fully support a set of bisectable repositories without
    additional intervention (**R4**)

* Git submodules

  - Does not fully support **R1**, since the externally maintained repositories
    would still need to be inside the main zephyr Git tree
  - Does not support **R3**. Downstream copies would need to either delete or
    replace submodule definitions
  - Does not support continuous tracking of the latest ``HEAD`` in external
    repositories (**R4**)
  - Requires hardcoding of the paths/locations of the external repositories

Finally, please see :ref:`west-history` for the motivations behind using a
single tool for both multi-repository management as well as building, debugging
and flashing.


.. _west-struct:

Structure
*********

West structure
==============

West is currently downloaded and installed on a system in the following stages:

* Bootstrapper: Installed using ``pip``, implements ``west init``
* Installation: Installed using ``west init``, implements built-in multi-repo
  commands
* Extension commands: Installed using ``west update``, parses the manifest
  in the west installation for additional commands

.. note::

   The separation between the "bootstrapper" and "installation" pieces is
   inspired by the Google Repo tool, but it's a temporary structure for
   convenience. In the future, the two will be merged. This will lessen the
   complexity of the design and make it easier to write and debug extension
   commands.

Repository structure
====================

Beyond west itself, the actual code repositories that west works with
are the following:

* Manifest repository: Cloned by ``west init``, and managed afterward with Git
  by the user. Contains the list of projects in the manifest file
  :file:`west.yml`. In the case of upstream Zephyr, the zephyr repository is
  the manifest repository.
* Projects: Cloned and managed by ``west update``. Listed in the manifest file.

Bootstrapper
============

The bootstrapper module is distributed using `PyPI`_ and installed using
:file:`pip`. A launcher named ``west`` is placed by :file:`pip` in the user's
``PATH``. This the only entry point to west.  It implements a single command:
``west init``. This command needs to be run first to use the rest of
functionality included in ``west``, by creating a west installation. The
command ``west init`` does the following:

* Clone west itself in a :file:`.west/west` folder in the installation
* Clone the manifest repository in the folder specified by the manifest file's
  (:file:`west.yml`) ``self.path`` section. Additional information
  on the manifest can be found in the :ref:`west-multi-repo` section)

Once ``west init`` has been run, the bootstrapper will delegate the handling of
any west commands other than ``init`` to the cloned west installation.

This means that there is a single bootstrapper instance installed at any time
(unless you use virtual environments), which can then be used to initialize as
many installations as needed.

.. _west-struct-installation:

Installation
============

A west installation, as describe above, contains a clone of the west repository
in :file:`.west/west`. The clone of west in the installation is where the
built-in multi-repo command implementations are currently provided.

When running a west command, the bootstrapper delegates handling for all
commands except ``init`` to the :file:`.west/west` repository in the current
installation.

Extension Commands
==================

The west manifest file (more about which below) allows any repository in the
installation to provide additional west commands (the flash and debug commands
use this mechanism; they are defined in the west repository). Every repository
which includes extension commands must provide a YAML file which configures the
names of the commands, the Python files that implement them, and other
metadata.

The locations of these YAML files throughout the installation are specified in
the manifest, which you'll read about next.

.. _west-mr-model:

Model
*****

Manifest
========

A **manifest** is a `YAML <http://yaml.org/>`_ file that, at a minimum, gives the
URL and revision for each project repository in the multi-repo (each **project**).
The manifest is stored in the **manifest repository** and is named :file:`west.yml`.

The format of the west manifest is described by a pykwalify schema, `manifest-schema.yml
<https://github.com/zephyrproject-rtos/west/blob/master/src/west/manifest-schema.yml>`_.

A west-based Zephyr installation has a special manifest repository. This
repository contains the west manifest file named :file:`west.yml`. The west
manifest contains:

* A list of the projects (Git repositories) in the installation and
  metadata about them (where to clone them from, what revision to check out,
  etc.). Externally maintained projects (vendor HALs, crypto libraries, etc)
  will be moved into their own repositories as part of the transition to
  multi-repository, and will be managed using this list.

* Metadata about the manifest repository, such as what path to clone it into in
  the Zephyr installation.

* Optionally, a description of how to clone west itself (this allows users to
  fork west itself, should that be necessary).

* Additionally both the projects and the metadata about the manifest repository
  can optionally include information about additional west commands (extensions)
  that are contained inside the Git repositories.

Repositories
============

There are therefore three types of repositories that exist in a west-based Zephyr
installation:

* West repository: This is cloned by ``west init`` and placed in
  :file:`.west/west`

* Manifest repository: This is the repository that contains the :file:`west.yml`
  manifest file described above which lists all the projects. The manifest
  repository can either contain only the manifest itself or also have actual
  code in it. In the case of the upstream Zephyr Project, the manifest
  repository is the `zephyr repository <https://github.com/zephyrproject-rtos/zephyr>`_,
  which contains all zephyr source code except for externally maintained
  projects, which are listed in the :file:`west.yml` manifest file.
  It is the user's responsibility to update this repository using Git.

* Project repositories: In the context of west, projects are source code
  repositories that are listed in the manifest file, :file:`west.yml` contained
  inside the manifest repository. West manages projects, updating them according
  to the revisions present in the manifest file.

West's view of multirepo history looks like this example (though some parts of
the example are specific to upstream Zephyr’s use of west):

.. figure:: west-mr-model.png
    :align: center
    :alt: West multi-repo history
    :figclass: align-center

    West multi-repo history

The history of the manifest repository is the line of Git commits which is
"floating" on top of a plane (parent commits point to child commits using
solid arrows.) The plane contains the Git commit history of the projects, with
each project's history boxed in by a rectangle.

The commits in the manifest repository (again, for upstream Zephyr this is the
zephyr repository itself) each have a manifest file. The manifest file in each
zephyr commit gives the corresponding commits which it expects in the other
projects. These are shown using dotted line arrows in the diagram.

Notice a few important details about the above picture:

- Other projects can stay at the same versions between two zephyr commits
  (like ``P2`` does between zephyr commits ``A → B``, and both ``P1`` and
  ``P3`` do in ``F → G``).
- Other projects can move forward in history between two zephyr commits (``P3``
  from ``A → B``).
- A project can also move backwards in its history as zephyr moves forward
  (like ``P3`` from zephyr commits ``C → D``). One use for this is to "revert"
  a regression by moving the other project to a version before it was
  introduced.
- Not all zephyr manifests have the same other projects (``P2`` is not a part
  of the installation at zephyr commits ``F`` and ``G``).
- Two zephyr commits can have the same external commits (like ``F`` and ``G``).
- Not all commits in some projects are associated with a zephyr commit (``P3``
  "jumps" multiple commits in its history between zephyr commits ``B → C``).
- Every zephyr commit’s manifest refers to exactly one version in each of the
  other projects it cares about.

The ``manifest-rev`` branch
***************************

West creates a branch named ``manifest-rev`` in each project, pointing to the
project's manifest revision (or, more specifically, to the commit the revision
resolves to). The ``manifest-rev`` branch is updated whenever project data is
fetched (the `command overview`_ below explains which commands fetch project
data).

All work branches created using west track the ``manifest-rev`` branch. Several
multi-repo commands also use ``manifest-rev`` as a reference for the upstream
revision (as of the most recent fetch).

.. note::

   ``manifest-rev`` is a normal Git branch, and is only treated specially by
   name. If you delete or otherwise modify it, it will be recreated/reset when
   upstream data is next fetched by ``west``, as if through ``git reset
   --hard`` (though ``git update-ref`` is used internally).

   Since ``manifest-rev`` represents the upstream revision as of the most
   recent fetch, it is normally a bad idea to modify it.

   ``manifest-rev`` was added to allow branches to track SHA revisions, and to
   give a consistent reference for the upstream revision regardless of how the
   manifest changes over time.

.. note::
   West does not create a ``manifest-rev`` branch in the manifest repository,
   since west does not manage the manifest repository's branches or revisions.

Command overview
================

This section gives a quick overview of the multi-repo commands, split up by
functionality. Some commands loosely mimic the corresponding Git command, but in
a multi-repo context (``west diff`` shows local changes on all repositories).

Passing no projects to commands that accept a list of projects usually means to
run the command for all projects listed in the manifest.

.. note::

   For the most up-to-date documentation, see the command help texts (e.g.
   ``west diff --help``). Only the most important flags are mentioned here.

Cloning and updating projects
*****************************

After running ``west init`` to initialize west (e.g., with the default Zephyr
manifest), the following commands will clone/update projects.

.. note::

   To implement self-updates, ``west init`` also clones a repository with the
   west source code, which is updated whenever the projects are. The ``west``
   command is implemented as a thin wrapper that calls through to the code in
   the cloned repository. The wrapper itself only implements the ``west init``
   command.

   This is the same model used by Google's ``repo`` tool.

- ``west init [-l] [-m URL] [--mr REVISION] [PATH]``: Initializes a west
  installation.

  This command can be invoked in two distinct ways.
  If you already have a local copy or clone of the manifest repository, you can
  use the ``-l`` switch to instruct west to initialize an installation around
  the existing clone, without modifying it. For example,
  ``west init -l path/to/zephyr`` is useful if you already have cloned the
  zephyr repository in the past using Git and now want to initialize a west
  installation around it.
  If you however want to initialize an installation directly from the remote
  repository, you have the option to specify its URL using the ``-m`` switch
  and/or its revision with the ``--mr`` one. For example, invoking west with:
  ``west init -m https://github.com/zephyrproject-rtos/zephyr --mr v1.15.0``
  would clone the upstream official zephyr repository at the tagged release
  v1.15.0.

- ``west update [PROJECT ...]``: Clones or updates the specified
  projects (default: all projects).

  This command will parse the manifest file (:file:`west.yml`) in the manifest
  repository, clone all project repositories that are not already present
  locally and finally update all projects to the revision specified in the
  manifest file.
  An initial branch named after the project's manifest revision is created in
  each cloned project repository. The names of branch and tag revisions are
  used as-is.  For qualified refs like ``refs/heads/foo``, the last component
  (``foo``) is used. For SHA revisions, a detached ``HEAD`` is checked out.

.. note::
  West uses ``git checkout`` to switch each project to the revision specified
  in the manifest repository. This is typically a safe operation that will not
  modify any branches or staged work you might have.

Miscellaneous commands
**********************

These commands perform miscellaneous functions.

- ``west list``: Lists project information from the manifest (URL, revision,
  path, etc.), along with other manifest-related information.

- ``west diff [PROJECT ...] [ARGUMENT ...]``: Runs a multi-repo ``git diff``
  for the specified projects (default: all cloned projects).

  Extra arguments are passed as-is to ``git diff``.

- ``west status [PROJECT ...] [ARGUMENT ...]``: Like ``west diff``, for
  running ``git status``.

- ``west forall -c COMMAND [PROJECT ...]``: Runs the shell command ``COMMAND``
  within the top-level repository directory of each of the specified projects
  (default: all cloned projects).

  If ``COMMAND`` consists of more than one word, it must be quoted to prevent
  it from being split up by the shell.

  Note that ``west forall`` can be used to run any command, not just Git
  commands. To run a Git command, do ``west forall -c 'git ...'``.

- ``west selfupdate``: Updates the west repository.

  Normally, the west repository is updated automatically whenever a command
  that fetches upstream data is run (this behavior can be
  suppressed for the duration of a single command by passing ``--no-update``).

.. _PyPI:
   https://pypi.org/project/west/

.. _Zephyr issue #6770:
   https://github.com/zephyrproject-rtos/zephyr/issues/6770
