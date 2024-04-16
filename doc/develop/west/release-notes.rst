.. _west-release-notes:

West Release Notes
##################

v1.1.0
******

Major changes:

- ``west compare``: new command that compares the state of the
  workspace against the manifest.

- Support for a new ``manifest.project-filter`` configuration option.
  See :ref:`west-config-index` for details. The ``west manifest --freeze``
  and ``west manifest --resolve`` commands currently cannot be used when
  this option is set. This restriction can be removed in a later release.

- Project names which contain comma (``,``) or whitespace now generate
  warnings. These warnings are errors if the new ``manifest.project-filter``
  configuration option is set. The warnings may be promoted to errors in a
  future major version of west.

Other changes:

- ``west forall`` now takese a ``--group`` argument that can be used
  to restrict the command to only run in one or more groups. Run
  ``west help forall`` for details.

- All west commands will now output log messages from west API modules at
  warning level or higher. In addition, the ``--verbose`` argument to west
  can be used once to include informational messages, or twice to include
  debug messages, from all commands.

Bug fixes:

- Various improvements to error messages, debug logging, and error handling.

API changes:

- ``west.manifest.Manifest.is_active()`` now respects the
  ``manifest.project-filter`` configuration option's value.

v1.0.1
******

Major changes:

- Manifest schema version "1.0" is now available for use in this release. This
  is identical to the "0.13" schema version in terms of features, but can be
  used by applications that do not wish to use a "0.x" manifest "version:"
  field. See :ref:`west-manifest-schema-version` for details on this feature.

Bug fixes:

- West no longer exits with a successful error code when sent an
  interrupt signal. Instead, it exits with a platform-specific
  error code and signals to the calling environment that the
  process was interrupted.

v1.0.0
******

Major changes in this release:

- The :ref:`west-apis` are now declared stable. Any breaking changes will be
  communicated by a major version bump from v1.x.y to v2.x.y.

- West v1.0 no longer works with the Zephyr v1.14 LTS releases. This LTS has
  long been obsoleted by Zephyr v2.7 LTS. If you need to use Zephyr v1.14, you
  must use west v0.14 or earlier.

- Like the rest of Zephyr, west now requires Python v3.8 or later

- West commands no longer accept abbreviated command line arguments. For
  example, you must now specify ``west update --keep-descendants`` instead of
  using an abbreviation like ``west update --keep-d``. This is part of a change
  applied to all of Zephyr's Python scripts' command-line interfaces. The
  abbreviations were causing problems in practice when commands were updated to
  add new options with similar names but different behavior to existing ones.

Other changes:

- All built-in west functions have stopped using ``west.log``

- ``west update``: new ``--submodule-init-config`` option.
  See commit `9ba92b05`_ for details.

Bug fixes:

- West extension commands that failed to load properly sometimes dumped stack.
  This has been fixed and west now prints a sensible error message in this case.

- ``west config`` now fails on malformed configuration option arguments
  which lack a ``.`` in the option name

API changes:

- The west package now contains the metadata files necessary for some static
  analyzers (such as `mypy`_) to auto-detect its type annotations.
  See commit `d9f00e24`_ for details.

- the deprecated ``west.build`` module used for Zephyr v1.14 LTS compatibility was
  removed

- the deprecated ``west.cmake`` module used for Zephyr v1.14 LTS compatibility was
  removed

- the ``west.log`` module is now deprecated. This module uses global state,
  which can make it awkward to use it as an API which multiple different python
  modules may rely on.

- The :ref:`west-apis-commands` module got some new APIs which lay groundwork
  for a future change to add a global verbosity control to a command's output,
  and work to remove global state from the ``west`` package's API:

  - New ``west.commands.WestCommand.__init__()`` keyword argument: ``verbosity``
  - New ``west.commands.WestCommand`` property: ``color_ui``
  - New ``west.commands.WestCommand`` methods, which should be used to print output
    from extension commands instead of writing directly to sys.stdout or
    sys.stderr: ``inf()``, ``wrn()``, ``err()``, ``die()``, ``banner()``,
    ``small_banner()``
  - New ``west.commands.VERBOSITY`` enum

.. _9ba92b05: https://github.com/zephyrproject-rtos/west/commit/9ba92b054500d75518ff4c4646590bfe134db523
.. _d9f00e24: https://github.com/zephyrproject-rtos/west/commit/d9f00e242b8cb297b56e941982adf231281c6bae
.. _mypy: https://www.mypy-lang.org/

v0.14.0
*******

Bug fixes:

- West commands that were run with a bad local configuration file dumped stack
  in a confusing way. This has been fixed and west now prints a sensible error
  message in this case.

- A bug in the way west looks for the zephyr repository was fixed. The bug
  itself usually appeared when running an extension command like ``west build``
  in a new workspace for the first time; this used to fail (just for the first
  time, not on subsequent command invocations) unless you ran the command in
  the workspace's top level directory.

- West now prints sensible error messages when the user lacks permission to
  open the manifest file instead of dumping stack traces.

API changes:

- The ``west.manifest.MalformedConfig`` exception type has been moved to the
  ``west.configuration`` module

- The ``west.manifest.MalformedConfig`` exception type has been moved to the
  :ref:`west.configuration <west-apis-configuration>` module

- The ``west.configuration.Configuration`` class now raises ``MalformedConfig``
  instead of ``RuntimeError`` in some cases

v0.13.1
*******

Bug fix:

- When calling west.manifest.Manifest.from_file() when outside of a
  workspace, west again falls back on the ZEPHYR_BASE environment
  variable to locate the workspace.

v0.13.0
*******

New features:

- You can now associate arbitrary user data with the manifest repository
  itself in the ``manifest: self: userdata:`` value, like so:

  .. code-block:: YAML

     manifest:
       self:
         userdata: <any YAML value can go here>

Bug fixes:

- The path to the manifest repository reported by west could be incorrect in
  certain circumstances detailed in [issue
  #572](https://github.com/zephyrproject-rtos/west/issues/572). This has been
  fixed as part of a larger overhaul of path handling support in the
  ``west.manifest`` API module.

- The ``west.Manifest.ManifestProject.__repr__`` return value was fixed

:ref:`API <west-apis>` changes:

- ``west.configuration.Configuration``: new object-oriented interface to the
  current configuration. This reflects the system, global, and workspace-local
  configuration values, and allows you to read, write, and delete configuration
  options from any or all of these locations.

- ``west.commands.WestCommand``:

  - ``config``: new attribute, returns a ``Configuration`` object or aborts the
    program if none is set. This is always usable from within extension command
    ``do_run()`` implementations.
  - ``has_config``: new boolean attribute, which is ``True`` if and only if
    reading ``self.config`` will abort the program.

- The path handling in the ``west.manifest`` package has been overhauled in a
  backwards-incompatible way. For more details, see commit
  [56cfe8d1d1](https://github.com/zephyrproject-rtos/west/commit/56cfe8d1d1f3c9b45de3e793c738acd62db52aca).

- ``west.manifest.Manifest.validate()``: this now returns the validated data as
  a Python dict. This can be useful if the value passed to this function was a
  str, and the dict is desired.

- ``west.manifest.Manifest``: new:

  - path attributes ``abspath``, ``posixpath``, ``relative_path``,
    ``yaml_path``, ``repo_path``, ``repo_posixpath``
  - ``userdata`` attribute, which contains the parsed value
    from ``manifest: self: userdata:``, or is None
  - ``from_topdir()`` factory method

- ``west.manifest.ManifestProject``: new ``userdata`` attribute, which also
  contains the parsed value from ``manifest: self: userdata:``, or is None

- ``west.manifest.ManifestImportFailed``: the constructor can now take any
  value; this can be used to reflect failed imports from a :ref:`map
  <west-manifest-import-map>` or other compound value.

- Deprecated configuration APIs:

  The following APIs are now deprecated in favor of using a ``Configuration``
  object. Usually this will be done via ``self.config`` from a ``WestCommand``
  instance, but this can be done directly by instantiating a ``Configuration``
  object for other usages.

  - ``west.configuration.config``
  - ``west.configuration.read_config``
  - ``west.configuration.update_config``
  - ``west.configuration.delete_config``

v0.12.0
*******

New features:

- West now works on the `MSYS2 <https://www.msys2.org/>`_ platform.

- West manifest files can now contain arbitrary user data associated with each
  project. See :ref:`west-project-userdata` for details.

Bug fixes:

- The ``west list`` command's ``{sha}`` format key has been fixed for
  the manifest repository; it now prints ``N/A`` ("not applicable")
  as expected.

:ref:`API <west-apis>` changes:

- The ``west.manifest.Project.userdata`` attribute was added to support
  project user data.

v0.11.1
*******

New features:

- ``west status`` now only prints output for projects which have a nonempty
  status.

Bug fixes:

- The manifest file parser was incorrectly allowing project names which contain
  the path separator characters ``/`` and ``\``. These invalid characters are
  now rejected.

  Note: if you need to place a project within a subdirectory of the workspace
  topdir, use the ``path:`` key. If you need to customize a project's fetch URL
  relative to its remote ``url-base:``, use ``repo-path:``. See
  :ref:`west-manifests-projects` for examples.

- The changes made in west v0.10.1 to the ``west init --manifest-rev`` option
  which selected the default branch name were leaving the manifest repository
  in a detached HEAD state. This has been fixed by using ``git clone`` internally
  instead of ``git init`` and ``git fetch``. See `issue #522`_ for details.

- The ``WEST_CONFIG_LOCAL`` environment variable now correctly
  overrides the default location, :file:`<workspace topdir>/.west/config`.

- ``west update --fetch=smart`` (``smart`` is the default) now correctly skips
  fetches for project revisions which are `lightweight tags`_ (it already
  worked correctly for annotated tags; only lightweight tags were unnecessarily
  fetched).

Other changes:

- The fix for issue #522 mentioned above introduces a new restriction. The
  ``west init --manifest-rev`` option value, if given, must now be either a
  branch or a tag. In particular, "pseudo-branches" like GitHub's
  ``pull/1234/head`` references which could previously be used to fetch a pull
  request can no longer be passed to ``--manifest-rev``. Users must now fetch
  and check out such revisions manually after running ``west init``.

:ref:`API <west-apis>` changes:

- ``west.manifest.Manifest.get_projects()`` avoids incorrect results in
  some edge cases described in `issue #523`_.

- ``west.manifest.Project.sha()`` now works correctly for tag revisions.
  (This applies to both lightweight and annotated tags.)

.. _lightweight tags: https://git-scm.com/book/en/v2/Git-Basics-Tagging
.. _issue #522: https://github.com/zephyrproject-rtos/west/issues/522
.. _issue #523: https://github.com/zephyrproject-rtos/west/issues/523

v0.11.0
*******

New features:

- ``west update`` now supports ``--narrow``, ``--name-cache``, and
  ``--path-cache`` options. These can be influenced by the ``update.narrow``,
  ``update.name-cache``, and ``update.path-cache`` :ref:`west-config` options.
  These can be used to optimize the speed of the update.
- ``west update`` now supports a ``--fetch-opt`` option that will be passed to
  the ``git fetch`` command used to fetch remote revisions when updating each
  project.

Bug fixes:

- ``west update`` now synchronizes Git submodules in projects by default. This
  avoids issues if the URL changes in the manifest file from when the submodule
  was first initialized. This behavior can be disabled by setting the
  ``update.sync-submodules`` configuration option to ``false``.

Other changes:

- the :ref:`west-apis-manifest` module has fixed docstrings for the Project
  class

v0.10.1
*******

New features:

- The :ref:`west-init` command's ``--manifest-rev`` (``--mr``) option no longer
  defaults to ``master``. Instead, the command will query the repository for
  its default branch name and use that instead. This allows users to move from
  ``master`` to ``main`` without breaking scripts that do not provide this
  option.

.. _west_0_10_0:

v0.10.0
*******

New features:

- The ``name`` key in a project's :ref:`submodules list
  <west-manifest-submodules>` is now optional.

Bug fixes:

- West now checks that the manifest schema version is one of the explicitly
  allowed values documented in :ref:`west-manifest-schema-version`. The old
  behavior was just to check that the schema version was newer than the west
  version where the ``manifest: version:`` key was introduced. This incorrectly
  allowed invalid schema versions, like ``0.8.2``.

Other changes:

- A manifest file's ``group-filter`` is now propagated through an ``import``.
  This is a change from how west v0.9.x handled this. In west v0.9.x, only the
  top level manifest file's ``group-filter`` had any effect; the group filter
  lists from any imported manifests were ignored.

  Starting with west v0.10.0, the group filter lists from imported manifests
  are also imported. For details, see :ref:`west-group-filter-imports`.

  The new behavior will take effect if ``manifest: version:`` is not given or
  is at least ``0.10``. The old behavior is still available in the top level
  manifest file only with an explicit ``manifest: version: 0.9``. See
  :ref:`west-manifest-schema-version` for more information on schema versions.

  See `west pull request #482
  <https://github.com/zephyrproject-rtos/west/pull/482>`_ for the motivation
  for this change and additional context.

v0.9.1
******

Bug fixes:

- Commands like ``west manifest --resolve`` now correctly include group and
  group filter information.

Other changes:

- West now warns if you combine ``import`` with ``group-filter``. Semantics for
  this combination have changed starting with v0.10.x. See the v0.10.0 release
  notes above for more information.

.. _west_0_9_0:

v0.9.0
******

.. warning::

   The ``west config`` fix described below comes at a cost: any comments or
   other manual edits in configuration files will be removed when setting a
   configuration option via that command or the ``west.configuration`` API.

.. warning::

   Combining the ``group-filter`` feature introduced in this release with
   manifest imports is discouraged. The resulting behavior has changed in west
   v0.10.

New features:

- West manifests now support :ref:`west-manifest-submodules`. This allows you
  to clone `Git submodules
  <https://git-scm.com/book/en/v2/Git-Tools-Submodules>`_ into a west project
  repository in addition to the project repository itself.

- West manifests now support :ref:`west-manifest-groups`. Project groups can be
  enabled and disabled to determine what projects are "active", and therefore
  will be acted upon by the following commands: ``west update``, ``west list``,
  ``west diff``, ``west status``, ``west forall``.

- ``west update`` no longer updates inactive projects by default. It now
  supports a ``--group-filter`` option which allows for one-time modifications
  to the set of enabled and disabled project groups.

- Running ``west list``, ``west diff``, ``west status``, or ``west forall``
  with no arguments does not print information for inactive projects by
  default. If the user specifies a list of projects explicitly at the command
  line, output for them is included regardless of whether they are active.

  These commands also now support ``--all`` arguments to include all
  projects, even inactive ones.

- ``west list`` now supports a ``{groups}`` format string key in its
  ``--format`` argument.

Bug fixes:

- The ``west config`` command and ``west.configuration`` API did not correctly
  store some configuration values, such as strings which contain commas. This
  has been fixed; see `commit 36f3f91e
  <https://github.com/zephyrproject-rtos/west/commit/36f3f91e270782fb05f6da13800f433a9c48f130>`_
  for details.

- A manifest file with an empty ``manifest: self: path:`` value is invalid, but
  west used to let it pass silently. West now rejects such manifests.

- A bug affecting the behavior of the ``west init -l .`` command was fixed; see
  `issue #435 <https://github.com/zephyrproject-rtos/west/issues/435>`_.

:ref:`API <west-apis>` changes:

- added ``west.manifest.Manifest.is_active()``
- added ``west.manifest.Manifest.group_filter``
- added ``submodules`` attribute to ``west.manifest.Project``, which has
  newly added type ``west.manifest.Submodule``

Other changes:

- The :ref:`west-manifest-import` feature now supports the terms ``allowlist``
  and ``blocklist`` instead of ``whitelist`` and ``blacklist``, respectively.

  The old terms are still supported for compatibility, but the documentation
  has been updated to use the new ones exclusively.

v0.8.0
******

This is a feature release which changes the manifest schema by adding support
for a ``path-prefix:`` key in an ``import:`` mapping, along with some other
features and fixes.

- Manifest import mappings now support a ``path-prefix:`` key, which places
  the project and its imported repositories in a subdirectory of the workspace.
  See :ref:`west-manifest-ex3.4` for an example.
- The west command line application can now also be run using ``python3 -m
  west``. This makes it easier to run west under a particular Python
  interpreter without modifying the :envvar:`PATH` environment variable.
- :ref:`west manifest --path <west-manifest-path>` prints the absolute path to
  west.yml
- ``west init`` now supports an ``--mf foo.yml`` option, which initializes the
  workspace using :file:`foo.yml` instead of :file:`west.yml`.
- ``west list`` now prints the manifest repository's path using the
  ``manifest.path`` :ref:`configuration option <west-config>`, which may differ
  from the ``self: path:`` value in the manifest data. The old behavior is
  still available, but requires passing a new ``--manifest-path-from-yaml``
  option.
- Various Python API changes; see :ref:`west-apis` for details.

v0.7.3
******

This is a bugfix release.

- Fix an error where a failed import could leave the workspace in an unusable
  state (see [PR #415](https://github.com/zephyrproject-rtos/west/pull/415) for
  details)

v0.7.2
******

This is a bugfix and minor feature release.

- Filter out duplicate extension commands brought in by manifest imports
- Fix ``west.Manifest.get_projects()`` when finding the manifest repository by
  path

v0.7.1
******

This is a bugfix and minor feature release.

- ``west update --stats`` now prints timing for operations which invoke a
  subprocess, time spent in west's Python process for each project, and total
  time updating each project.
- ``west topdir`` always prints a POSIX style path
- minor console output changes

v0.7.0
******

The main user-visible feature in west 0.7 is the :ref:`west-manifest-import`
feature. This allows users to load west manifest data from multiple different
files, resolving the results into a single logical manifest.

Additional user-visible changes:

- The idea of a "west installation" has been renamed to "west workspace" in
  this documentation and in the west API documentation. The new term seems to
  be easier for most people to work with than the old one.
- West manifests now support a :ref:`schema version
  <west-manifest-schema-version>`.
- The "west config" command can now be run outside of a workspace, e.g.
  to run ``west config --global section.key value`` to set a configuration
  option's value globally.
- There is a new :ref:`west topdir <west-built-in-misc>` command, which
  prints the root directory of the current west workspace.
- The ``west -vv init`` command now prints the git operations being performed,
  and their results.
- The restriction that no project can be named "manifest" is now enforced; the
  name "manifest" is reserved for the manifest repository, and is usable as
  such in commands like ``west list manifest``, instead of ``west list
  path-to-manifest-repository`` being the only way to say that
- It's no longer an error if there is no project named "zephyr". This is
  part of an effort to make west generally usable for non-Zephyr use cases.
- Various bug fixes.

The developer-visible changes to the :ref:`west-apis` are:

- west.build and west.cmake: deprecated; this is Zephyr-specific functionality
  and should never have been part of west. Since Zephyr v1.14 LTS relies on it,
  it will continue to be included in the distribution, but will be removed
  when that version of Zephyr is obsoleted.
- west.commands:

  - WestCommand.requires_installation: deprecated; use requires_workspace instead
  - WestCommand.requires_workspace: new
  - WestCommand.has_manifest: new
  - WestCommand.manifest: this is now settable
- west.configuration: callers can now identify the workspace directory
  when reading and writing configuration files
- west.log:

  - msg(): new
- west.manifest:

  - The module now uses the standard logging module instead of west.log
  - QUAL_REFS_WEST: new
  - SCHEMA_VERSION: new
  - Defaults: removed
  - Manifest.as_dict(): new
  - Manifest.as_frozen_yaml(): new
  - Manifest.as_yaml(): new
  - Manifest.from_file() and from_data(): these factory methods are more
    flexible to use and less reliant on global state
  - Manifest.validate(): new
  - ManifestImportFailed: new
  - ManifestProject: semi-deprecated and will likely be removed later.
  - Project: the constructor now takes a topdir argument
  - Project.format() and its callers are removed. Use f-strings instead.
  - Project.name_and_path: new
  - Project.remote_name: new
  - Project.sha() now captures stderr
  - Remote: removed

West now requires Python 3.6 or later. Additionally, some features may rely on
Python dictionaries being insertion-ordered; this is only an implementation
detail in CPython 3.6, but is is part of the language specification as of
Python 3.7.

v0.6.3
******

This point release fixes an error in the behavior of the deprecated
``west.cmake`` module.

v0.6.2
******

This point release fixes an error in the behavior of ``west
update --fetch=smart``, introduced in v0.6.1.

All v0.6.1 users must upgrade.

v0.6.1
******

.. warning::

   Do not use this point release. Make sure to use v0.6.2 instead.

The user-visible features in this point release are:

- The :ref:`west-update` command has a new ``--fetch``
  command line flag and ``update.fetch`` :ref:`configuration option
  <west-config>`. The default value, "smart", skips fetching SHAs and tags
  which are available locally.
- Better and more consistent error-handling in the ``west diff``, ``west
  status``, ``west forall``, and ``west update`` commands. Each of these
  commands can operate on multiple projects; if a subprocess related to one
  project fails, these commands now continue to operate on the rest of the
  projects. All of them also now report a nonzero error code from the west
  process if any of these subprocesses fails (this was previously not true of
  ``west forall`` in particular).
- The :ref:`west manifest <west-built-in-misc>` command also handles errors
  better.
- The :ref:`west list <west-built-in-misc>` command now works even when the
  projects are not cloned, as long as its format string only requires
  information which can be read from the manifest file. It still fails if the
  format string requires data stored in the project repository, e.g. if it
  includes the ``{sha}`` format string key.
- Commands and options which operate on git revisions now accept abbreviated
  SHAs. For example, ``west init --mr SHA_PREFIX`` now works. Previously, the
  ``--mr`` argument needed to be the entire 40 character SHA if it wasn't a
  branch or a tag.

The developer-visible changes to the :ref:`west-apis` are:

- west.log.banner(): new
- west.log.small_banner(): new
- west.manifest.Manifest.get_projects(): new
- west.manifest.Project.is_cloned(): new
- west.commands.WestCommand instances can now access the parsed
  Manifest object via a new self.manifest property during the
  do_run() call. If read, it returns the Manifest object or
  aborts the command if it could not be parsed.
- west.manifest.Project.git() now has a capture_stderr kwarg


v0.6.0
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
