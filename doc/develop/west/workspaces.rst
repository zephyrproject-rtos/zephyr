.. _west-workspaces:

Workspaces
##########

This page describes the *west workspace* concept introduced in
:ref:`west-basics` in more detail.

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

The ``refs/west/*`` Git refs
****************************

West also reserves all Git refs that begin with ``refs/west/`` (such as
``refs/west/foo``) for itself in local project repositories. Unlike
``manifest-rev``, these refs are not regular branches. West's behavior here is
an implementation detail; users should not rely on these refs' existence or
behavior.

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
in plain text even if two-factor authentication is disabled.)

You can use the Git credential store to authenticate with a GitHub PAT
(Personal Access Token) like so:

.. code-block:: shell

   echo "https://x-access-token:$GH_TOKEN@github.com" >> ~/.git-credentials

If you don't want to store any credentials on the file system, you can store
them in memory temporarily using `git-credential-cache`_ instead.

If you setup fetching via SSH, you can use Git URL rewrite feature. The following
command instructs Git to use SSH URLs for GitHub instead of HTTPS ones:

.. code-block:: shell

   git config --global url."git@github.com:".insteadOf "https://github.com/"

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

Project locations
*****************

Projects can be located anywhere inside the workspace, but they may not
"escape" it.

In other words, project repositories need not be located in subdirectories of
the manifest repository or as immediate subdirectories of the topdir. However,
projects must have paths inside the workspace.

You may replace a project's repository directory within the workspace with a
symbolic link to elsewhere on your computer, but west will not do this for you.

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
   │       └── zcbor/       # .git/ project from either the main manifest or some import.
   │
   └── zephyr/              # .git/ project
       └── west.yml         # This can be partially imported with lower precedence or ignored.
                            # Only the 'manifest-rev' version can be imported.


Here is an example :file:`application/west.yml` which uses
:ref:`west-manifest-import`, available since west 0.7, to import Zephyr v2.5.0
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
         revision: v2.5.0
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
         revision: v2.5.0
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
   │       └── zcbor/      # .git/ project from either the main manifest or
   │                       #       from some import
   │
   └── zephyr/             # .git/ project
       └── west.yml        # This can be partially imported with lower precedence or ignored.
                           # Only the 'manifest-rev' version can be imported.

Here is an example T3 :file:`manifest-repo/west.yml` which uses
:ref:`west-manifest-import`, available since west 0.7, to import Zephyr
v2.5.0 and its modules, then add the ``app1`` and ``app2`` projects:

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
         revision: v2.5.0
         import: true
       - name: app1
         revision: SOME_SHA_OR_BRANCH_OR_TAG
       - name: app2
         revision: ANOTHER_SHA_OR_BRANCH_OR_TAG
     self:
       path: manifest-repo

You can also do this "by hand" by copy/pasting :file:`zephyr/west.yml`
as shown :ref:`above <west-t2>` for the T2 topology, with the same caveats.

.. _workspace-as-git-repo:

Not supported: workspace topdir as .git repository
**************************************************

Some users have asked for support making the workspace :ref:`topdir
<west-workspace>` a git repository, like this example:

.. code-block:: none

   my-workspace/                  # workspace topdir
   ├── .git/                      # puts the entire workspace in a git repository
   ├── .west/                     # marks the location of the topdir
   └── [ ... other projects ...]

This is **not** an officially supported topology. As a design decision, west
assumes that the workspace topdir itself is not a git repository.

You may be able to make something like this "work" for yourself and your own
goals. However, future versions of west might contain changes which can "break"
your setup.
