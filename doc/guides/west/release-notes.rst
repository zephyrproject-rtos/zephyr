West Release Notes
##################

v0.6.x
******

- No separate bootstrapper

  In west v0.5.x, the program was split into two components, a bootstrapper and
  a per-installation clone. See `Multiple Repository Management in the v1.14
  documentation`_ for more details.

  This is similar to how Google's Repo tool works, and lets west iterate quickly
  at first. It caused confusion, however, and west is now stable enough to be
  distributed entirely as one piece via PyPI.

  From v0.6.x onwards, all of the core west commands and helper classes are
  part of the west package distributed via PyPI. This eliminates complexity
  and makes it possible to import west modules from anywhere in the system,
  not just extension commands.
- The ``selfupdate`` command still exists for backwards compatibility, but
  now simply exits after printing an error message.
- Manifest syntax changes

  - A west manifest file's ``projects`` elements can now specify their fetch
    URLs directly, like so:

    .. code-block:: yaml

       manifest:
         projects:
           - name: example-project-name
             url: https://github.com/example/example-project

    Project elements with ``url`` attributes set in this way may not also have
    ``remote`` attributes.
  - Project names must be unique: this restriction is needed to support future
    work, but was not possible in west v0.5.x because distinct projects may
    have URLs with the same final pathname component, like so:

    .. code-block:: yaml

       manifest:
         remotes:
           - name: remote-1
             url-base: https://github.com/remote-1
           - name: remote-2
             url-base: https://github.com/remote-2
         projects:
           - name: project
             remote: remote-1
             path: remote-1-project
           - name: project
             remote: remote-2
             path: remote-2-project

    These manifests can now be written with projects that use ``url``
    instead of ``remote``, like so:

    .. code-block:: yaml

       manifest:
         projects:
           - name: remote-1-project
             url: https://github.com/remote-1/project
           - name: remote-2-project
             url: https://github.com/remote-2/project

- The ``west list`` command now supports a ``{sha}`` format string key

- The default format string for ``west list`` was changed to ``"{name:12}
  {path:28} {revision:40} {url}"``.

- The command ``west manifest --validate`` can now be run to load and validate
  the current manifest file, among other error-handling fixes related to
  manifest parsing.

- Incompatible API changes were made to west's APIs. Further changes are
  expected until API stability is declared in west v1.0.

  - The ``west.manifest.Project`` constructor's ``remote`` and ``defaults``
    positional arguments are now kwargs. A new ``url`` kwarg was also added; if
    given, the ``Project`` URL is set to that value, and the ``remote`` kwarg
    is ignored.

  - ``west.manifest.MANIFEST_SECTIONS`` was removed. There is only one section
    now, namely ``manifest``. The *sections* kwargs in the
    ``west.manifest.Manifest`` factory methods and constructor were also
    removed.

  - The ``west.manifest.SpecialProject`` class was removed. Use
    ``west.manifest.ManifestProject`` instead.


v0.5.x
******

West v0.5.x is the first version used widely by the Zephyr Project as part of
its v1.14 Long-Term Support (LTS) release. The `west v0.5.x documentation`_ is
available as part of the Zephyr's v1.14 documentation.

West's main features in v0.5.x are:

- Multiple repository management using Git repositories, including self-update
  of west itself
- Hierarchical configuration files
- Extension commands

Versions Before v0.5.x
**********************

Tags in the west repository before v0.5.x are prototypes which are of
historical interest only.

.. _Multiple Repository Management in the v1.14 documentation:
   https://docs.zephyrproject.org/1.14.0/guides/west/repo-tool.html

.. _west v0.5.x documentation:
   https://docs.zephyrproject.org/1.14.0/guides/west/index.html

