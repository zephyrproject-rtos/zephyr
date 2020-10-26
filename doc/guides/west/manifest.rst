.. _west-manifests:

West Manifests
##############

This page contains detailed information about west's multiple repository model,
manifest files, and the ``west manifest`` command. For API documentation on the
``west.manifest`` module, see :ref:`west-apis-manifest`. For a more general
introduction and command overview, see :ref:`west-multi-repo`.

.. only:: html

   .. contents::
      :depth: 3

.. _west-mr-model:

Multiple Repository Model
*************************

West's view of the repositories in a :term:`west workspace`, and their
history, looks like the following figure (though some parts of this example are
specific to upstream Zephyr's use of west):

.. figure:: west-mr-model.png
   :align: center
   :alt: West multi-repo history
   :figclass: align-center

   West multi-repo history

The history of the manifest repository is the line of Git commits which is
"floating" on top of the gray plane. Parent commits point to child commits
using solid arrows. The plane below contains the Git commit history of the
repositories in the workspace, with each project repository boxed in by a
rectangle. Parent/child commit relationships in each repository are also shown
with solid arrows.

The commits in the manifest repository (again, for upstream Zephyr this is the
zephyr repository itself) each have a manifest file. The manifest file in each
commit specifies the corresponding commits which it expects in each of the
project repositories. This relationship is shown using dotted line arrows in the
diagram. Each dotted line arrow points from a commit in the manifest repository
to a corresponding commit in a project repository.

Notice the following important details:

- Projects can be added (like ``P1`` between manifest repository
  commits ``D`` and ``E``) and removed (``P2`` between the same
  manifest repository commits)

- Project and manifest repository histories don't have to move
  forwards or backwards together:

  - ``P2`` stays the same from ``A → B``, as do ``P1`` and ``P3`` from ``F →
    G``.
  - ``P3`` moves forward from ``A → B``.
  - ``P3`` moves backward from ``C → D``.

  One use for moving backward in project history is to "revert" a regression by
  going back to a revision before it was introduced.

- Project repository commits can be "skipped": ``P3`` moves forward
  multiple commits in its history from ``B → C``.

- In the above diagram, no project repository has two revisions "at
  the same time": every manifest file refers to exactly one commit in
  the projects it cares about. This can be relaxed by using a branch
  name as a manifest revision, at the cost of being able to bisect
  manifest repository history.

.. _west-manifest-files:

Manifest Files
**************

A west manifest is a YAML file named :file:`west.yml`. Manifests have a
top-level ``manifest`` section with some subsections, like this:

.. code-block:: yaml

   manifest:
     defaults:
       # default project attributes (optional)
     remotes:
       # short names for project URLs (optional)
     projects:
       # a list of projects managed by west (mandatory)
     self:
       # configuration related to the manifest repository itself,
       # i.e. the repository containing west.yml (optional)
     version: <schema-version> # optional

In YAML terms, the manifest file contains a mapping, with a ``manifest``
key. Any other keys and their contents are ignored (west v0.5 also required a
``west`` key, but this is ignored starting with v0.6).

There are four subsections: ``defaults``, ``remotes``, ``projects``, and
``self``. In YAML terms, the value of the ``manifest`` key is also a mapping,
with these "subsections" as keys. Only ``projects`` is mandatory: this is the
list of repositories managed by west and their metadata.

We'll cover the ``remotes`` and ``projects`` subsections in detail first.

The ``remotes`` subsection contains a sequence which specifies the base URLs
where projects can be fetched from. Each sequence element has a name and a "URL
base". These are used to form the complete fetch URL for each project. For
example:

.. code-block:: yaml

   manifest:
     # ...
     remotes:
       - name: remote1
         url-base: https://git.example.com/base1
       - name: remote2
         url-base: https://git.example.com/base2

Above, two remotes are given, with names ``remote1`` and ``remote2``. Their URL
bases are respectively ``https://git.example.com/base1`` and
``https://git.example.com/base2``. You can use SSH URL bases as well; for
example, you might use ``git@example.com:base1`` if ``remote1`` supported Git
over SSH as well. Anything acceptable to Git will work.

The ``projects`` subsection contains a sequence describing the project
repositories in the west workspace. Every project has a unique name. You can
specify what Git remote URLs to use when cloning and fetching the projects,
what revisions to track, and where the project should be stored on the local
file system.

Here is an example. We'll assume the ``remotes`` given above.

.. Note: if you change this example, keep the equivalent manifest below in
   sync.

.. code-block:: yaml

   manifest:
     # [... same remotes as above...]
     projects:
       - name: proj1
         remote: remote1
         path: extra/project-1
       - name: proj2
         repo-path: my-path
         remote: remote2
         revision: v1.3
       - name: proj3
         url: https://github.com/user/project-three
         revision: abcde413a111

In this manifest:

- ``proj1`` has remote ``remote1``, so its Git fetch URL is
  ``https://git.example.com/base1/proj1``. The remote ``url-base`` is appended
  with a ``/`` and the project ``name`` to form the URL.

  Locally, this project will be cloned at path ``extra/project-1`` relative to
  the west workspace's root directory, since it has an explicit ``path``
  attribute with this value.

  Since the project has no ``revision`` specified, ``master`` is used by
  default. The current tip of this branch will be fetched and checked out as a
  detached ``HEAD`` when west next updates this project.

- ``proj2`` has a ``remote`` and a ``repo-path``, so its fetch URL is
  ``https://git.example.com/base2/my-path``. The ``repo-path`` attribute, if
  present, overrides the default ``name`` when forming the fetch URL.

  Since the project has no ``path`` attribute, its ``name`` is used by
  default. It will be cloned into a directory named ``proj2``. The commit
  pointed to by the ``v1.3`` tag will be checked out when west updates the
  project.

- ``proj3`` has an explicit ``url``, so it will be fetched from
  ``https://github.com/user/project-three``.

  Its local path defaults to its name, ``proj3``. Commit ``abcde413a111`` will
  be checked out when it is next updated.

The list of project keys and their usage follows. Sometimes we'll refer to the
``defaults`` subsection; it will be described next.

- ``name``: Mandatory. the name of the project. The name cannot be one of the
  reserved values "west" or "manifest". The name must be unique in the manifest
  file.
- ``remote`` or ``url``: Mandatory (one of the two, but not both).

  If the project has a ``remote``, that remote's ``url-base`` will be combined
  with the project's ``name`` (or ``repo-path``, if it has one) to form the
  fetch URL instead.

  If the project has a ``url``, that's the complete fetch URL for the
  remote Git repository.

  If the project has neither, the ``defaults`` section must specify a
  ``remote``, which will be used as the the project's remote. Otherwise, the
  manifest is invalid.
- ``repo-path``: Optional. If given, this is concatenated on to the remote's
  ``url-base`` instead of the project's ``name`` to form its fetch URL.
  Projects may not have both ``url`` and ``repo-path`` attributes.
- ``revision``: Optional. The Git revision that ``west update`` should check
  out. This will be checked out as a detached HEAD by default, to avoid
  conflicting with local branch names.  If not given, the ``revision`` value
  from the ``defaults`` subsection will be used if present.

  A project revision can be a branch, tag, or SHA. The default ``revision`` is
  ``master`` if not otherwise specified.
- ``path``: Optional. Relative path specifying where to clone the repository
  locally, relative to the top directory in the west workspace. If missing,
  the project's ``name`` is used as a directory name.
- ``clone-depth``: Optional. If given, a positive integer which creates a
  shallow history in the cloned repository limited to the given number of
  commits. This can only be used if the ``revision`` is a branch or tag.
- ``west-commands``: Optional. If given, a relative path to a YAML file within
  the project which describes additional west commands provided by that
  project. This file is named :file:`west-commands.yml` by convention. See
  :ref:`west-extensions` for details.
- ``import``: Optional. If ``true``, imports projects from manifest files in
  the given repository into the current manifest. See
  :ref:`west-manifest-import` for more details.

The ``defaults`` subsection can provide default values for project
attributes. In particular, the default remote name and revision can be
specified here. Another way to write the same manifest we have been describing
so far using ``defaults`` is:

.. code-block:: yaml

   manifest:
     defaults:
       remote: remote1
       revision: v1.3

     remotes:
       - name: remote1
         url-base: https://git.example.com/base1
       - name: remote2
         url-base: https://git.example.com/base2

     projects:
       - name: proj1
         path: extra/project-1
         revision: master
       - name: proj2
         repo-path: my-path
         remote: remote2
       - name: proj3
         url: https://github.com/user/project-three
         revision: abcde413a111

The ``self`` subsection can be used to control the behavior of the
manifest repository itself. Its value is a map with the following keys:

- ``path``: Optional. The path to clone the manifest repository into, relative
  to the west workspace's root directory. If not given, the basename of the
  path component in the manifest repository URL will be used by default.  For
  example, if the URL is ``https://git.example.com/project-repo``, the manifest
  repository would be cloned to the directory :file:`project-repo`.

- ``west-commands``: Optional. This is analogous to the same key in a
  project sequence element.

- ``import``: Optional. This is also analogous to the ``projects`` key, but
  allows importing projects from other files in the manifest repository. See
  :ref:`west-manifest-import`.

As an example, let's consider this snippet from the zephyr repository's
:file:`west.yml`:

.. code-block:: yaml

   manifest:
     # ...
     self:
       path: zephyr
       west-commands: scripts/west-commands.yml

This ensures that the zephyr repository is cloned into path ``zephyr``, though
as explained above that would have happened anyway if cloning from the default
manifest URL, ``https://github.com/zephyrproject-rtos/zephyr``. Since the
zephyr repository does contain extension commands, its ``self`` entry declares
the location of the corresponding :file:`west-commands.yml` relative to the
repository root.

.. _west-manifest-schema-version:

The ``version`` subsection can be used to mark the lowest version of the
manifest file schema that can parse this file's data:

.. code-block:: yaml

   manifest:
     version: 0.7
     # marks that this manifest uses features available in west 0.7 and
     # up, like manifest imports

The pykwalify schema :file:`manifest-schema.yml` in the west source code
repository is used to validate the manifest section. The current manifest
``version`` is 0.7, which corresponds to west version 0.7. This is the only
value this field can currently take.

If a later version of west, say version ``21.0``, includes changes to the
manifest schema that cannot be parsed by west 0.7, then setting ``version:
21.0`` will cause west to print an error when attempting to parse the manifest
data.

.. _west-manifest-import:

Manifest Imports
****************

You can use the ``import`` key briefly described above to include projects from
other manifest files in your :file:`west.yml`. This key can be either a
``project`` or ``self`` section attribute:

.. code-block:: yaml

   manifest:
     projects:
       - name: some-project
         import: ...
     self:
       import: ...

You can use a "self: import:" to load additional files from the repository
containing your :file:`west.yml`. You can use a "project: ... import:" to load
additional files defined in that project's Git history.

West resolves the final manifest from individual manifest files in this order:

#. imported files in ``self``
#. your :file:`west.yml` file
#. imported files in ``projects``

During resolution, west ignores projects which have already been defined in
other files. For example, a project named ``foo`` in your :file:`west.yml`
makes west ignore other projects named ``foo`` imported from your ``projects``
list.

The ``import`` key can be a boolean, path, mapping, or sequence. We'll describe
these in order, using examples:

- :ref:`Boolean <west-manifest-import-bool>`
   - :ref:`west-manifest-ex1.1`
   - :ref:`west-manifest-ex1.2`
   - :ref:`west-manifest-ex1.3`
- :ref:`Relative path <west-manifest-import-path>`
   - :ref:`west-manifest-ex2.1`
   - :ref:`west-manifest-ex2.2`
   - :ref:`west-manifest-ex2.3`
- :ref:`Mapping with additional configuration <west-manifest-import-map>`
   - :ref:`west-manifest-ex3.1`
   - :ref:`west-manifest-ex3.2`
   - :ref:`west-manifest-ex3.3`
   - :ref:`west-manifest-ex3.4`
- :ref:`Sequence of paths and mappings <west-manifest-import-seq>`
   - :ref:`west-manifest-ex4.1`
   - :ref:`west-manifest-ex4.2`

A more :ref:`formal description <west-manifest-formal>` of how this works is
last, after the examples.

Troubleshooting Note
====================

If you're using this feature and find west's behavior confusing, try
:ref:`resolving your manifest <west-manifest-resolve>` to see the final results
after imports are done.

.. _west-manifest-import-bool:

Option 1: Boolean
=================

This is the easiest way to use ``import``.

If ``import`` is ``true`` as a ``projects`` attribute, west imports projects
from the :file:`west.yml` file in that project's root directory. If it's
``false`` or missing, it has no effect. For example, this manifest would import
:file:`west.yml` from the ``p1`` git repository at revision ``v1.0``:

.. code-block:: yaml

   manifest:
     # ...
     projects:
       - name: p1
         revision: v1.0
         import: true    # Import west.yml from p1's v1.0 git tag
       - name: p2
         import: false   # Nothing is imported from p2.
       - name: p3        # Nothing is imported from p3 either.

It's an error to set ``import`` to either ``true`` or ``false`` inside
``self``, like this:

.. code-block:: yaml

   manifest:
     # ...
     self:
       import: true  # Error

.. _west-manifest-ex1.1:

Example 1.1: Downstream of a Zephyr release
-------------------------------------------

You have a source code repository you want to use with Zephyr v1.14.1 LTS.  You
want to maintain the whole thing using west. You don't want to modify any of
the mainline repositories.

In other words, the west workspace you want looks like this:

.. code-block:: none

   my-downstream/
   ├── .west/                     # west directory
   ├── zephyr/                    # mainline zephyr repository
   │   └── west.yml               # the v1.14.1 version of this file is imported
   ├── modules/                   # modules from mainline zephyr
   │   ├── hal/
   │   └── [...other directories..]
   ├── [ ... other projects ...]  # other mainline repositories
   └── my-repo/                   # your downstream repository
       ├── west.yml               # main manifest importing zephyr/west.yml v1.14.1
       └── [...other files..]

You can do this with the following :file:`my-repo/west.yml`:

.. code-block:: yaml

   # my-repo/west.yml:
   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     projects:
       - name: zephyr
         remote: zephyrproject-rtos
         revision: v1.14.1
         import: true

You can then create the workspace on your computer like this, assuming
``my-repo`` is hosted at ``https://git.example.com/my-repo``:

.. code-block:: console

   west init -m https://git.example.com/my-repo my-downstream
   cd my-downstream
   west update

After ``west init``, :file:`my-downstream/my-repo` will be cloned.

After ``west update``, all of the projects defined in the ``zephyr``
repository's :file:`west.yml` at revision ``v1.14.1`` will be cloned into
:file:`my-downstream` as well.

You can add and commit any code to :file:`my-repo` you please at this point,
including your own Zephyr applications, drivers, etc. See :ref:`application`.

.. _west-manifest-ex1.2:

Example 1.2: "Rolling release" Zephyr downstream
------------------------------------------------

This is similar to :ref:`west-manifest-ex1.1`, except we'll use ``revision:
master`` for the zephyr repository:

.. code-block:: yaml

   # my-repo/west.yml:
   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     projects:
       - name: zephyr
         remote: zephyrproject-rtos
         revision: master
         import: true

You can create the workspace in the same way:

.. code-block:: console

   west init -m https://git.example.com/my-repo my-downstream
   cd my-downstream
   west update

This time, whenever you run ``west update``, the special :ref:`manifest-rev
<west-manifest-rev>` branch in the ``zephyr`` repository will be updated to
point at a newly fetched ``master`` branch tip from the URL
https://github.com/zephyrproject-rtos/zephyr.

The contents of :file:`zephyr/west.yml` at the new ``manifest-rev`` will then
be used to import projects from Zephyr. This lets you stay up to date with the
latest changes in the Zephyr project. The cost is that running ``west update``
will not produce reproducible results, since the remote ``master`` branch can
change every time you run it.

It's also important to understand that west **ignores your working tree's**
:file:`zephyr/west.yml` entirely when resolving imports. West always uses the
contents of imported manifests as they were committed to the latest
``manifest-rev`` when importing from a project.

You can only import manifest from the file system if they are in your manifest
repository's working tree. See :ref:`west-manifest-ex2.2` for an example.

.. _west-manifest-ex1.3:

Example 1.3: Downstream of a Zephyr release, with module fork
-------------------------------------------------------------

This manifest is similar to the one in :ref:`west-manifest-ex1.1`, except it:

- is a downstream of Zephyr 2.0
- includes a downstream fork of the :file:`modules/hal/nordic`
  :ref:`module <modules>` which was included in that release

.. code-block:: yaml

   # my-repo/west.yml:
   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
       - name: my-remote
         url-base: https://git.example.com
     projects:
       - name: hal_nordic         # higher precedence
         remote: my-remote
         revision: my-sha
         path: modules/hal/nordic
       - name: zephyr
         remote: zephyrproject-rtos
         revision: v2.0.0
         import: true             # imported projects have lower precedence

   # subset of zephyr/west.yml contents at v2.0.0:
   manifest:
     defaults:
       remote: zephyrproject-rtos
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     projects:
     # ...
     - name: hal_nordic           # lower precedence, values ignored
       path: modules/hal/nordic
       revision: another-sha

With this manifest file, the project named ``hal_nordic``:

- is cloned from ``https://git.example.com/hal_nordic`` instead of
  ``https://github.com/zephyrproject-rtos/hal_nordic``.
- is updated to commit ``my-sha`` by ``west update``, instead of
  the mainline commit ``another-sha``

In other words, when your top-level manifest defines a project, like
``hal_nordic``, west will ignore any other definition it finds later on while
resolving imports.

This does mean you have to copy the ``path: modules/hal/nordic`` value into
:file:`my-repo/west.yml` when defining ``hal_nordic`` there. The value from
:file:`zephyr/west.yml` is ignored entirely. See :ref:`west-manifest-resolve`
for troubleshooting advice if this gets confusing in practice.

When you run ``west update``, west will:

- update zephyr's ``manifest-rev`` to point at the ``v2.0.0`` tag
- import :file:`zephyr/west.yml` at that ``manifest-rev``
- locally check out the ``v2.0.0`` revisions for all zephyr projects except
  ``hal_nordic``
- update ``hal_nordic`` to ``my-sha`` instead of ``another-sha``

.. _west-manifest-import-path:

Option 2: Relative path
=======================

The ``import`` value can also be a relative path to a manifest file or a
directory containing manifest files. The path is relative to the root directory
of the ``projects`` or ``self`` repository the ``import`` key appears in.

Here is an example:

.. code-block:: yaml

   manifest:
     projects:
       - name: project-1
         revision: v1.0
         import: west.yml
       - name: project-2
         revision: master
         import: p2-manifests
     self:
       import: submanifests

This will import the following:

- the contents of :file:`project-1/west.yml` at ``manifest-rev``, which points
  at tag ``v1.0`` after running ``west update``
- any YAML files in the directory tree :file:`project-2/p2-manifests`
  at the latest ``master``, as fetched by ``west update``, sorted by file name
- YAML files in :file:`submanifests` in your manifest repository,
  as they appear on your file system, sorted by file name

Notice how ``projects`` imports get data from Git using ``manifest-rev``, while
``self`` imports get data from your file system. This is because as usual, west
leaves version control for your manifest repository up to you.

.. _west-manifest-ex2.1:

Example 2.1: Downstream of a Zephyr release with explicit path
--------------------------------------------------------------

This is an explicit way to write an equivalent manifest to the one in
:ref:`west-manifest-ex1.1`.

.. code-block:: yaml

   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     projects:
       - name: zephyr
         remote: zephyrproject-rtos
         revision: v1.14.1
         import: west.yml

The setting ``import: west.yml`` means to use the file :file:`west.yml` inside
the ``zephyr`` project. This example is contrived, but shows the idea.

This can be useful in practice when the name of the manifest file you want to
import is not :file:`west.yml`.

.. _west-manifest-ex2.2:

Example 2.2: Downstream with directory of manifest files
--------------------------------------------------------

Your Zephyr downstream has a lot of additional repositories. So many, in fact,
that you want to split them up into multiple manifest files, but keep track of
them all in a single manifest repository, like this:

.. code-block:: none

   my-repo/
   ├── submanifests
   │   ├── 01-libraries.yml
   │   ├── 02-vendor-hals.yml
   │   └── 03-applications.yml
   └── west.yml

You want to add all the files in :file:`my-repo/submanifests` to the main
manifest file, :file:`my-repo/west.yml`, in addition to projects in
:file:`zephyr/west.yml`. You want to track the latest mainline master
instead of using a fixed revision.

Here's how:

.. code-block:: yaml

   # my-repo/west.yml:
   manifest:
     remotes:
       - name: zephyrproject-rtos
         url-base: https://github.com/zephyrproject-rtos
     projects:
       - name: zephyr
         remote: zephyrproject-rtos
         import: true
     self:
       import: submanifests

Manifest files are imported in this order during resolution:

#. :file:`my-repo/submanifests/01-libraries.yml`
#. :file:`my-repo/submanifests/02-vendor-hals.yml`
#. :file:`my-repo/submanifests/03-applications.yml`
#. :file:`my-repo/west.yml`
#. :file:`zephyr/west.yml`

.. note::

   The :file:`.yml` file names are prefixed with numbers in this example to
   make sure they are imported in the specified order.

   You can pick arbitrary names. West sorts files in a directory by name before
   importing.

Notice how the manifests in :file:`submanifests` are imported *before*
:file:`my-repo/west.yml` and :file:`zephyr/west.yml`. In general, an ``import``
in the ``self`` section is processed before the manifest files in ``projects``
and the main manifest file.

This means projects defined in :file:`my-repo/submanifests` take highest
precedence. For example, if :file:`01-libraries.yml` defines ``hal_nordic``,
the project by the same name in :file:`zephyr/west.yml` is simply ignored. As
usual, see :ref:`west-manifest-resolve` for troubleshooting advice.

This may seem strange, but it allows you to redefine projects "after the fact",
as we'll see in the next example.

.. _west-manifest-ex2.3:

Example 2.3: Continuous Integration overrides
---------------------------------------------

Your continuous integration system needs to fetch and test multiple
repositories in your west workspace from a developer's forks instead of your
mainline development trees, to see if the changes all work well together.

Starting with :ref:`west-manifest-ex2.2`, the CI scripts add a
file :file:`00-ci.yml` in :file:`my-repo/submanifests`, with these contents:

.. code-block:: yaml

   # my-repo/submanifests/00-ci.yml:
   manifest:
     projects:
       - name: a-vendor-hal
         url: https://github.com/a-developer/hal
         revision: a-pull-request-branch
       - name: an-application
         url: https://github.com/a-developer/application
         revision: another-pull-request-branch

The CI scripts run ``west update`` after generating this file in
:file:`my-repo/submanifests`. The projects defined in :file:`00-ci.yml` have
higher precedence than other definitions in :file:`my-repo/submanifests`,
because the name :file:`00-ci.yml` comes before the other file names.

Thus, ``west update`` always checks out the developer's branches in the
projects named ``a-vendor-hal`` and ``an-application``, even if those same
projects are also defined elsewhere.

.. _west-manifest-import-map:

Option 3: Mapping
=================

The ``import`` key can also contain a mapping with the following keys:

- ``file``: Optional. The name of the manifest file or directory to import.
  This defaults to :file:`west.yml` if not present.
- ``name-whitelist``: Optional. If present, a name or sequence of project names
  to include.
- ``path-whitelist``: Optional. If present, a path or sequence of project paths
  to match against. This is a shell-style globbing pattern, currently
  implemented with `pathlib`_. Note that this means case sensitivity is
  platform specific.
- ``name-blacklist``: Optional. Like ``name-whitelist``, but contains project
  names to exclude rather than include.
- ``path-blacklist``: Optional. Like ``path-whitelist``, but contains project
  paths to exclude rather than include.
- ``path-prefix``: Optional (new in v0.8.0). If given, this will be prepended
  to the project's path in the workspace, as well as the paths of any imported
  projects. This can be used to place these projects in a subdirectory of the
  workspace.

.. _re: https://docs.python.org/3/library/re.html
.. _pathlib:
   https://docs.python.org/3/library/pathlib.html#pathlib.PurePath.match

Whitelists override blacklists if both are given. For example, if a project is
blacklisted by path, then whitelisted by name, it will still be imported.

.. _west-manifest-ex3.1:

Example 3.1: Downstream with name whitelist
-------------------------------------------

Here is a pair of manifest files, representing a mainline and a
downstream. The downstream doesn't want to use all the mainline
projects, however. We'll assume the mainline :file:`west.yml` is
hosted at ``https://git.example.com/mainline/manifest``.

.. code-block:: yaml

   # mainline west.yml:
   manifest:
     projects:
       - name: mainline-app                # included
         path: examples/app
         url: https://git.example.com/mainline/app
       - name: lib
         path: libraries/lib
         url: https://git.example.com/mainline/lib
       - name: lib2                        # included
         path: libraries/lib2
         url: https://git.example.com/mainline/lib2

   # downstream west.yml:
   manifest:
     projects:
       - name: mainline
         url: https://git.example.com/mainline/manifest
         import:
           name-whitelist:
             - mainline-app
             - lib2
       - name: downstream-app
         url: https://git.example.com/downstream/app
       - name: lib3
         path: libraries/lib3
         url: https://git.example.com/downstream/lib3

An equivalent manifest in a single file would be:

.. code-block:: yaml

   manifest:
     projects:
       - name: mainline
         url: https://git.example.com/mainline/manifest
       - name: downstream-app
         url: https://git.example.com/downstream/app
       - name: lib3
         path: libraries/lib3
         url: https://git.example.com/downstream/lib3
       - name: mainline-app                   # imported
         path: examples/app
         url: https://git.example.com/mainline/app
       - name: lib2                           # imported
         path: libraries/lib2
         url: https://git.example.com/mainline/lib2

If a whitelist had not been used, the ``lib`` project from the mainline
manifest would have been imported.

.. _west-manifest-ex3.2:

Example 3.2: Downstream with path whitelist
-------------------------------------------

Here is an example showing how to whitelist mainline's libraries only,
using ``path-whitelist``.

.. code-block:: yaml

   # mainline west.yml:
   manifest:
     projects:
       - name: app
         path: examples/app
         url: https://git.example.com/mainline/app
       - name: lib
         path: libraries/lib                  # included
         url: https://git.example.com/mainline/lib
       - name: lib2
         path: libraries/lib2                 # included
         url: https://git.example.com/mainline/lib2

   # downstream west.yml:
   manifest:
     projects:
       - name: mainline
         url: https://git.example.com/mainline/manifest
         import:
           path-whitelist: libraries/*
       - name: app
         url: https://git.example.com/downstream/app
       - name: lib3
         path: libraries/lib3
         url: https://git.example.com/downstream/lib3

An equivalent manifest in a single file would be:

.. code-block:: yaml

   manifest:
     projects:
       - name: lib                          # imported
         path: libraries/lib
         url: https://git.example.com/mainline/lib
       - name: lib2                         # imported
         path: libraries/lib2
         url: https://git.example.com/mainline/lib2
       - name: mainline
         url: https://git.example.com/mainline/manifest
       - name: app
         url: https://git.example.com/downstream/app
       - name: lib3
         path: libraries/lib3
         url: https://git.example.com/downstream/lib3

.. _west-manifest-ex3.3:

Example 3.3: Downstream with path blacklist
-------------------------------------------

Here's an example showing how to blacklist all vendor HALs from mainline by
common path prefix in the workspace, add your own version for the chip
you're targeting, and keep everything else.

.. code-block:: yaml

   # mainline west.yml:
   manifest:
     defaults:
       remote: mainline
     remotes:
       - name: mainline
         url-base: https://git.example.com/mainline
     projects:
       - name: app
       - name: lib
         path: libraries/lib
       - name: lib2
         path: libraries/lib2
       - name: hal_foo
         path: modules/hals/foo     # excluded
       - name: hal_bar
         path: modules/hals/bar     # excluded
       - name: hal_baz
         path: modules/hals/baz     # excluded

   # downstream west.yml:
   manifest:
     projects:
       - name: mainline
         url: https://git.example.com/mainline/manifest
         import:
           path-blacklist: modules/hals/*
       - name: hal_foo
         path: modules/hals/foo
         url: https://git.example.com/downstream/hal_foo

An equivalent manifest in a single file would be:

.. code-block:: yaml

   manifest:
     defaults:
       remote: mainline
     remotes:
       - name: mainline
         url-base: https://git.example.com/mainline
     projects:
       - name: app                  # imported
       - name: lib                  # imported
         path: libraries/lib
       - name: lib2                 # imported
         path: libraries/lib2
       - name: mainline
         repo-path: https://git.example.com/mainline/manifest
       - name: hal_foo
         path: modules/hals/foo
         url: https://git.example.com/downstream/hal_foo

.. _west-manifest-ex3.4:

Example 3.4: Import into a subdirectory
---------------------------------------

You want to import a manifest and its projects, placing everything into a
subdirectory of your :term:`west workspace`.

For example, suppose you want to import this manifest from project ``foo``,
adding this project and its projects ``bar`` and ``baz`` to your workspace:

.. code-block:: yaml

   # foo/west.yml:
   manifest:
     defaults:
       remote: example
     remotes:
       - name: example
         url-base: https://git.example.com
     projects:
       - name: bar
       - name: baz

Instead of importing these into the top level workspace, you want to place all
three project repositories in an :file:`external-code` subdirectory, like this:

.. code-block:: none

   workspace/
   └── external-code/
       ├── foo/
       ├── bar/
       └── baz/

You can do this using this manifest:

.. code-block:: yaml

   manifest:
     projects:
       - name: foo
         url: https://git.example.com/foo
         import:
           path-prefix: external-code

An equivalent manifest in a single file would be:

.. code-block:: yaml

   # foo/west.yml:
   manifest:
     defaults:
       remote: example
     remotes:
       - name: example
         url-base: https://git.example.com
     projects:
       - name: foo
         path: external-code/foo
       - name: bar
         path: external-code/bar
       - name: baz
         path: external-code/baz

.. _west-manifest-import-seq:

Option 4: Sequence
==================

The ``import`` key can also contain a sequence of files, directories,
and mappings.

.. _west-manifest-ex4.1:

Example 4.1: Downstream with sequence of manifest files
-------------------------------------------------------

This example manifest is equivalent to the manifest in
:ref:`west-manifest-ex2.2`, with a sequence of explicitly named files.

.. code-block:: yaml

   # my-repo/west.yml:
   manifest:
     projects:
       - name: zephyr
         url: https://github.com/zephyrproject-rtos/zephyr
         import: west.yml
     self:
       import:
         - submanifests/01-libraries.yml
         - submanifests/02-vendor-hals.yml
         - submanifests/03-applications.yml

.. _west-manifest-ex4.2:

Example 4.2: Import order illustration
--------------------------------------

This more complicated example shows the order that west imports manifest files:

.. code-block:: yaml

   # my-repo/west.yml
   manifest:
     # ...
     projects:
       - name: my-library
       - name: my-app
       - name: zephyr
         import: true
       - name: another-manifest-repo
         import: submanifests
     self:
       import:
         - submanifests/libraries.yml
         - submanifests/vendor-hals.yml
         - submanifests/applications.yml
     defaults:
       remote: my-remote

For this example, west resolves imports in this order:

#. the listed files in :file:`my-repo/submanifests` are first, in the order
   they occur (e.g. :file:`libraries.yml` comes before
   :file:`applications.yml`, since this is a sequence of files), since the
   ``self: import:`` is always imported first
#. :file:`my-repo/west.yml` is next (with projects ``my-library`` etc. as long
   as they weren't already defined somewhere in :file:`submanifests`)
#. :file:`zephyr/west.yml` is after that, since that's the first ``import`` key
   in the ``projects`` list in :file:`my-repo/west.yml`
#. files in :file:`another-manifest-repo/submanifests` are last (sorted by file
   name), since that's the final project ``import``

.. _west-manifest-formal:

Manifest Import Details
=======================

This section describes how west imports a manifest file a bit more formally.

Overview
--------

A west manifest's ``projects`` and ``self`` sections can have ``import`` keys,
like so:

.. code-block:: yaml

   # Top-level west.yml.
   manifest:
     # ...
     projects:
       - name: foo
         revision: rev-1
         import: import-1
       - name: bar
         revision: rev-2
         import: import-2
       # ...
       - name: baz
         revision: rev-N
         import: import-N
     self:
       import: self-import

Import keys are optional. If any of ``import-1, ..., import-N`` are missing,
west will not import additional manifest data from that project. If
``self-import`` is missing, no additional files in the manifest repository
(beyond the top-level west.yml) are imported.

The ultimate outcome of resolving manifest imports is a final list of projects,
which is produced by combining the ``projects`` defined in the top-level file
with those defined in imported files. Importing is done in this order:

#. Manifests from ``self-import`` are imported first.
#. The top-level manifest file's ``projects`` are added in next.
#. Manifests from ``import-1``, ..., ``import-N``, are imported in that order.

This process recurses if necessary.

Projects are identified by name. If the same name occurs in multiple manifests,
the first definition is used, and subsequent definitions are ignored. For
example, if ``import-1`` contains a project named ``bar``, that is ignored,
because the top-level :file:`west.yml` has already defined a project by that
name.

The contents of files named by ``import-1`` through ``import-N`` are imported
from Git at the latest ``manifest-rev`` revisions in their projects. These
revisions can be updated to the values ``rev-1`` through ``rev-N`` by running
``west update``. If any ``manifest-rev`` reference is missing or out of date,
``west update`` also fetches project data from the remote fetch URL and updates
the reference.

Also note that all imported manifests, from the root manifest to the repository
which defines a project ``P``, must be up to date in order for west to update
``P`` itself. For example, this means ``west update P`` would update
``manifest-rev`` in the ``baz`` project if :file:`baz/west.yml` defines ``P``,
as well as updating the ``manifest-rev`` branch in the local git clone of
``P``. Confusingly, the update of ``baz`` may result in the removal of ``P``
from :file:`baz/west.yml`, which would cause ``west update P`` to fail with an
unrecognized project!

For this reason, it's usually best to run plain ``west update`` to avoid errors
if you use manifest imports. By default, west won't fetch any project data over
the network if a project's revision is a SHA or tag which is already available
locally, so updating the extra projects shouldn't take too much time unless
it's really needed. See the documentation for the :ref:`update.fetch
<west-config-index>` configuration option for more information.

If an imported manifest file has a ``west-commands:`` definition in its
``self:`` section, the extension commands defined there are added to the set of
available extensions at the time the manifest is imported. They will thus take
precedence over any extension commands with the same names added later on.

When an individual ``import`` key refers to multiple manifest files, they are
processed in this order:

- If the value is a relative path naming a directory (or a map whose ``file``
  is a directory), the manifest files it contains are processed in
  lexicographic order -- i.e., sorted by file name.
- If the value is a sequence, its elements are recursively imported in the
  order they appear.

.. _west-manifest-cmd:

Manifest Command
****************

The ``west manifest`` command can be used to manipulate manifest files.
It takes an action, and action-specific arguments.

The following sections describe each action and provides a basic signature for
simple uses. Run ``west manifest --help`` for full details on all options.

.. _west-manifest-resolve:

Resolving Manifests
===================

The ``--resolve`` action outputs a single manifest file equivalent to your
current manifest and all its :ref:`imported manifests <west-manifest-import>`:

.. code-block:: none

   west manifest --resolve [-o outfile]

The main use for this action is to see the "final" manifest contents after
performing any ``import``\ s.

To print detailed information about each imported manifest file and how
projects are handled during manifest resolution, set the maximum verbosity
level using ``-v``:

.. code-block:: console

   west -v manifest --resolve

Freezing Manifests
==================

The ``--freeze`` action outputs a frozen manifest:

.. code-block:: none

   west manifest --freeze [-o outfile]

A "frozen" manifest is a manifest file where every project's revision is a SHA.
You can use ``--freeze`` to produce a frozen manifest that's equivalent to your
current manifest file. The ``-o`` option specifies an output file; if not
given, standard output is used.

Validating Manifests
====================

The ``--validate`` action either succeeds if the current manifest file is valid,
or fails with an error:

.. code-block:: none

   west manifest --validate

The error message can help diagnose errors.

.. _west-manifest-path:

Get the manifest path
=====================

The ``--path`` action prints the path to the top level manifest file:

.. code-block:: none

   west manifest --path

The output is something like ``/path/to/workspace/west.yml``. The path format
depends on your operating system.
