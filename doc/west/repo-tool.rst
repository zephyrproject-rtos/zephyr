.. _west-multi-repo:

Multi-repository management
###########################

.. note::

   Zephyr is currently not managed as a multi-repo. This page describes an
   upcoming tool and potential workflow.

West includes a set of commands for working with projects composed of multiple
Git repositories (a *multi-repo*), similar to Google's `repo
<https://gerrit.googlesource.com/git-repo/>`_ tool.

The rest of this page introduces multi-repo concepts and gives an overview of
the multi-repo commands, along with an example workflow.

.. note::

   The multi-repo commands are meant to augment Git in minor ways for
   multi-repo work, not replace it. For tasks that aren't multi-repo-related,
   use plain Git commands.

   This page explains what the West multi-repo commands do behind the scenes.

Manifests
=========

A *manifest* is a `YAML <http://yaml.org/>`_ file that gives the URL and
revision for each repository in the multi-repo (each *project*), possibly along
with other information. Manifests are normally stored in a separate Git
repository.

Running ``west init`` to initialize a West installation will clone the selected
manifest repository. Running ``west init`` without specifying a manifest
repository will use `a default manifest repository
<https://github.com/zephyrproject-rtos/manifest>`_ for Zephyr. A different
manifest repository can be selected with ``west init -m URL``.

The format of the West manifest is described by a `pykwalify
<https://pypi.org/project/pykwalify/>`_ schema, found `here
<https://github.com/zephyrproject-rtos/west/blob/master/src/west/manifest-schema.yml>`_.

The ``manifest-rev`` branch
***************************

West creates a branch named ``manifest-rev`` in each project, pointing to the
project's manifest revision (or, more specifically, to the commit the revision
resolves to). The ``manifest-rev`` branch is updated whenever project data is
fetched (the `command overview`_ below explains which commands fetch project
data).

All work branches created using West track the ``manifest-rev`` branch. Several
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

Command overview
================

This section gives a quick overview of the multi-repo commands, split up by
functionality. All commands loosely mimic the corresponding Git command, but in
a multi-repo context (``west fetch`` fetches upstream data without updating
local work branches, etc.)

Passing no projects to commands that accept a list of projects usually means to
run the command for all projects listed in the manifest.

.. note::

   For the most up-to-date documentation, see the command help texts (e.g.
   ``west fetch --help``). Only the most important flags are mentioned here.

Cloning and updating projects
*****************************

After running ``west init`` to initialize West (e.g., with the default Zephyr
manifest), the following commands will clone/update projects.

Except for ``west rebase``, all commands in this section implicitly update the
manifest repository, fetch upstream data, and update the ``manifest-rev``
branch.

.. note::

   To implement self-updates, ``west init`` also clones a repository with the
   West source code, which is updated whenever the manifest is. The ``west``
   command is implemented as a thin wrapper that calls through to the code in
   the cloned repository. The wrapper itself only implements the ``west init``
   command.

   This is the same model used by Google's ``repo`` tool.

- ``west clone [-b BRANCH_NAME] [PROJECT ...]``: Clones the specified
  projects (default: all projects).

  An initial branch named after the project's manifest revision is created in
  each cloned repository. The names of branch and tag revisions are used as-is.
  For qualified refs like ``refs/heads/foo``, the last component (``foo``) is
  used. For SHA revisions, the generic name ``work`` is used.

  A different branch name can be specified with ``-b``.

  Like all branches created using West, the initial branch tracks
  ``manifest-rev``.

- ``west fetch [PROJECT ...]``: Fetches new upstream changes in each of the
  specified projects (default: all projects), cloning them first if necessary.
  Local branches are not updated (except for the special ``manifest-rev``
  branch).

  If a repository is cloned using ``west fetch``, the initial state is a
  detached HEAD at ``manifest-rev``.

  .. note::

     ``west clone`` is an alias for ``west fetch`` + ``west branch <name>``
     (see below).

- ``west pull [PROJECT ...]``: Fetches new upstream changes in each of the
  specified projects (default: all projects) and rebases them on top of
  ``manifest-rev``.

  This corresponds to ``git pull --rebase``, with the tracked branch taken as
  ``manifest-rev``.

- ``west rebase [PROJECT ...]``: Rebases each of the specified projects
  (default: all cloned projects) on top of ``manifest-rev``.

  .. note::

     ``west pull`` is an alias for ``west fetch`` + ``west rebase``.

Working with branches
*********************

The following commands are used to create, check out, and list branches that
span multiple projects (in reality, Git branches with identical names in
multiple repositories).

- ``west branch [BRANCH_NAME [PROJECT ...]]``: Creates a branch
  ``BRANCH_NAME`` in each of the specified projects (default: all cloned
  projects).

  Like all branches created using West, the newly created branches track
  ``manifest-rev``.

  If no arguments are passed to ``west branch``, local branches from all
  projects are listed, along with the projects they appear in.

- ``west checkout [-b] BRANCH_NAME [PROJECT ...]]``: Checks out the branch
  ``BRANCH_NAME`` in each of the specified projects (default: all cloned
  projects). This command is a no-op for projects that do not have the
  specified branch.

  With ``-b``, creates and checks out the branch if it does not exist in a
  project (like ``git checkout -b``), instead of ignoring the project.

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

- ``west update [--update-manifest] [--update-west]``: Updates the manifest
  and/or west repositories. With no arguments, both are updated.

  Normally, the manifest and west repositories are updated automatically
  whenever a command that fetches upstream data is run (this behavior can be
  suppressed for the duration of a single command by passing ``--no-update``).
  If the manifest or west repository has local modifications and cannot be
  fast-forwarded to the new version, the update of the repository is skipped
  (with a warning).

Example workflow
================

This section gives an example workflow that clones the Zephyr repositories,
creates and switches between two work branches, updates (rebases) a work branch
to the latest manifest revision, and inspects upstream changes.

1. First, we initialize West with the default Zephyr manifest, and clone all
   projects:

   .. code-block:: console

      west init
      west clone

   .. note::

      Repositories can also be cloned with ``west fetch``. The only difference
      between ``west fetch`` and ``west clone`` is that no initial work branch
      is created by ``west fetch``. Instead, the initial state with ``west
      fetch`` is a detached ``HEAD`` at ``manifest-rev``.

      The initial work branch created by ``west clone`` is named after the
      project's manifest revision. We don't do any work on the initial work
      branch in this example, and create our own work branches instead.

2. Next, we create and check out a branch ``xy-feature`` for implementing a
   feature that spans the ``proj-x`` and ``proj-y`` projects:

   .. code-block:: console

      west checkout -b xy-feature proj-x proj-y

   This creates a Git branch named ``xy-feature`` in ``proj-x`` and ``proj-y``.
   The new branches are automatically set up to track the ``manifest-rev``
   branch in their respective repositories.

   We work on the repositories as usual.

3. While working on the feature, an urgent bug comes up that spans all
   projects. We use ordinary Git commands to commit/stash the partially
   implemented feature, and then check out a new branch for fixing the bug:

   .. code-block:: console

      west checkout -b global-fix

4. After fixing the bug, we want to switch back to working on the feature, but
   we forgot its branch name. We list branches with ``west branch`` to remind
   ourselves, and then switch back:

   .. code-block:: console

      west checkout xy-feature

   This command only affects the ``proj-x`` and ``proj-y`` repositories, since
   only those have the ``xy-feature`` branch.

5. We now update (rebase) the ``proj-x`` and ``proj-y`` repositories, to get
   get any new upstream changes into the ``xy-feature`` branch:

   .. code-block:: console

      west pull proj-x proj-y

6. After doing more work, we want to see if more changes have been added
   upstream, but we don't want to rebase yet. Instead, we just fetch changes in
   all projects:

   .. code-block:: console

      west fetch

   This command fetches the upstream revision specified in the manifest and
   updates the ``manifest-rev`` branch to point to it, for all projects. Local
   work branches are not touched.

7. Running ``west status`` or ``git status`` now shows that ``manifest-rev``
   is ahead of the ``xy-feature`` branch in ``proj-x``, meaning there are new
   upstream changes.

   We want to update ``proj-x``. This could be done with ``west pull proj-x``,
   but since the ``xy-feature`` branch tracks the ``manifest-rev`` branch, we
   decide to mix it up a bit with a plain Git command inside the ``proj-x``
   repository:

   .. code-block:: console

      git pull --rebase

   .. note::

      In general, do not assume that multi-repo commands are "better" where a
      plain Git command will do. In particular, the multi-repo commands are not
      meant to replicate every feature of the corresponding Git commands.
