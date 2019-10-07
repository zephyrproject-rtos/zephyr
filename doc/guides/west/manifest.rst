.. _west-manifests:

West Manifests
##############

This page contains detailed information about west's multiple repository model
and manifest files. For API documentation on the ``west.manifest`` module, see
:ref:`west-apis-manifest`. For a more general introduction and command
overview, see :ref:`west-multi-repo`.

.. _west-mr-model:

Multiple Repository Model
*************************

West's view of the repositories in a :term:`west installation`, and their
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
repositories in the installation, with each project repository boxed in by a
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
     # [...]
     remotes:
       - name: remote1
         url-base: https://example.com/base1
       - name: remote2
         url-base: https://example.com/base2

Above, two remotes are given, with names ``remote1`` and ``remote2``. Their URL
bases are respectively ``https://example.com/base1`` and
``https://example.com/base2``. You can use SSH URL bases as well; for example,
you might use ``git@example.com:base1`` if ``remote1`` supported Git over SSH
as well. Anything acceptable to Git will work.

The ``projects`` subsection contains a sequence describing the project
repositories in the west installation. Every project has a unique name. You can
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
  ``https://example.com/base1/proj1``. The remote ``url-base`` is appended with
  a ``/`` and the project ``name`` to form the URL.

  Locally, this project will be cloned at path ``extra/project-1`` relative to
  the west installation's root directory, since it has an explicit ``path``
  attribute with this value.

  Since the project has no ``revision`` specified, ``master`` is used by
  default. The current tip of this branch will be fetched and checked out as a
  detached ``HEAD`` when west next updates this project.

- ``proj2`` has a ``remote`` and a ``repo-path``, so its fetch URL is
  ``https://example.com/base2/my-path``. The ``repo-path`` attribute, if
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
  locally, relative to the top directory in the west installation. If missing,
  the project's ``name`` is used as a directory name.
- ``clone-depth``: Optional. If given, a positive integer which creates a
  shallow history in the cloned repository limited to the given number of
  commits. This can only be used if the ``revision`` is a branch or tag.
- ``west-commands``: Optional. If given, a relative path to a YAML file within
  the project which describes additional west commands provided by that
  project. This file is named :file:`west-commands.yml` by convention. See
  :ref:`west-extensions` for details.

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
         url-base: https://example.com/base1
       - name: remote2
         url-base: https://example.com/base2

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

Finally, the ``self`` subsection can be used to control the behavior of the
manifest repository itself. Its value is a map with the following keys:

- ``path``: Optional. The path to clone the manifest repository into, relative
  to the west installation's root directory. If not given, the basename of the
  path component in the manifest repository URL will be used by default.  For
  example, if the URL is ``https://example.com/project-repo``, the manifest
  repository would be cloned to the directory :file:`project-repo`.

- ``west-commands``: Optional. This is analogous to the same key in a
  project sequence element.

As an example, let's consider this snippet from the zephyr repository's
:file:`west.yml`:

.. code-block:: yaml

   manifest:
     # [...]
     self:
       path: zephyr
       west-commands: scripts/west-commands.yml

This ensures that the zephyr repository is cloned into path ``zephyr``, though
as explained above that would have happened anyway if cloning from the default
manifest URL, ``https://github.com/zephyrproject-rtos/zephyr``. Since the
zephyr repository does contain extension commands, its ``self`` entry declares
the location of the corresponding :file:`west-commands.yml` relative to the
repository root.

The pykwalify schema :file:`manifest-schema.yml` in the west source code
repository is used to validate the manifest section.
