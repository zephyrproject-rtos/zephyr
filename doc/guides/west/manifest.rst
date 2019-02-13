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

A west manifest is a YAML file named :file:`west.yml`. Manifests have two
top-level "sections", ``west`` and ``manifest``, like this:

.. code-block:: yaml

   west:
     # contents of west section
   manifest:
     # contents of manifest section

In YAML terms, the manifest file contains a mapping, with two keys relevant to
west at top level. These keys are the scalar strings ``west`` and
``manifest``. Their contents are described next.

West Section
============

.. note::

   Support for this feature will be removed in a future version of west, when
   the west repository is no longer cloned into the installation.

The ``west`` section specifies the URL and revision of the west repository
which is cloned into the installation. For example:

.. code-block:: yaml

   west:
     url: https://example.com/west
     revision: v0.5.6

This specifies cloning the west repository from URL
``https://example.com/west`` (any URL accepted by ``git clone`` will work), at
revision ``v0.5.6``. The revision can be a Git branch, tag, or SHA.

That is, the west section also contains a mapping, with permitted keys ``url``
and ``revision``. These specify the fetch URL and Git revision for the west
repository to clone into the installation, as described in
:ref:`west-struct`. If not given, the default URL is
https://github.com/zephyrproject-rtos/west, and the default revision is
``master``.

The file :file:`west-schema.yml` in the west source code repository contains a
pykwalify schema for this section's contents.

Manifest Section
================

This is the main section in the manifest file. There are four subsections:
``defaults``, ``remotes``, ``projects``, and ``self``. In YAML terms, the value
of the ``manifest`` key is also a mapping, with these "subsections" as keys.
For example:

.. code-block:: yaml

   manifest:
     defaults:
       # contents of defaults subsection
     remotes:
       # contents of remotes subsection
     projects:
       # contents of projects subsection
     self:
       # contents of self subsection

The ``remotes`` and ``projects`` subsections are the only mandatory ones, so
we'll cover them first.

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

The ``projects`` subsection contains a sequence describing the
project repositories in the west installation. Each project has a
name and a remote; the project's name is appended to the remote URL
base to form the Git fetch URL west uses to clone the project and keep
it up to date. Here is a simple example; we'll assume the ``remotes``
given above.

.. code-block:: yaml

   manifest:
     # [...]
     projects:
       - name: proj1
         remote: remote1
         path: extra/project-1
       - name: proj2
         remote: remote1
         revision: v1.3
       - name: proj3
         remote: remote2
         revision: abcde413a111

This example has three projects:

- ``proj1`` has remote ``remote1``, so its Git fetch URL is
  ``https://example.com/base1/proj1`` (note that west adds the ``/`` between
  the URL base and project name). This project will be cloned at path
  ``extra/project-1`` relative to the west installation's root directory.
  Since the project has no ``revision``, the current tip of the ``master``
  branch will be checked out as a detached ``HEAD``.

- ``proj2`` has the same remote, so its fetch URL is
  ``https://example.com/base1/proj2``. Since the project has no ``path``
  specified, it will be cloned at ``proj2`` (i.e. a project's ``name`` is used
  as its default ``path``). The commit pointed to by the ``v1.3`` tag will be
  checked out.

- ``proj3`` has fetch URL ``https://example.com/base2/proj3`` and will be
  cloned at path ``proj3``. Commit ``abcde413a111`` will be checked out.

Each element in the ``projects`` sequence can contain the following keys. Some
of the description refers to the ``defaults`` subsection, which will be
described next.

- ``name``: Mandatory, the name of the project. The fetch URL is formed as
  remote url-base + '/' + ``name``. The name cannot be one of the reserved
  values "west" and "manifest".
- ``remote``: The name of the project's remote. If not given, the ``remote``
  value in the ``defaults`` subsection is tried next. If both are missing, the
  manifest is invalid.
- ``revision``: Optional. The current project revision used by ``west update``.
  If not given, the value from the ``defaults`` subsection will be used if
  present.  If both are missing, ``master`` is used. A project revision can be
  a branch, tag, or SHA. The names of unqualified branch and tag revisions are
  fetched as-is.  For qualified refs, like ``refs/heads/foo``, the last
  component (``foo``) is used.
- ``path``: Optional. Where to clone the repository locally. If missing, it's
  cloned in the west installation's root subdirectory given by the project's
  name.
- ``clone-depth``: Optional. If given, a positive integer which creates a
  shallow history in the cloned repository limited to the given number of
  commits.
- ``west-commands``: Optional. If given, a relative path to a YAML file within
  the project which describes additional west commands provided by that
  project. This file is named :file:`west-commands.yml` by convention. See
  :ref:`west-extensions` for details.

The ``defaults`` subsection can provide default values for project-related
values. In particular, the default remote name and revision can be specified
here. Another way to write the same manifest we have been describing so far
using ``defaults`` is:

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
       - name: proj3
         remote: remote2
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
