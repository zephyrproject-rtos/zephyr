.. _west-config:

Configuration
#############

This page documents west's configuration file system, the ``west config``
command, and configuration options used by built-in commands. For API
documentation on the ``west.configuration`` module, see
:ref:`west-apis-configuration`.

West Configuration Files
------------------------

West's configuration file syntax is INI-like; here is an example file:

.. code-block:: ini

   [manifest]
   path = zephyr

   [zephyr]
   base = zephyr

Above, the ``manifest`` section has option ``path`` set to ``zephyr``. Another
way to say the same thing is that ``manifest.path`` is ``zephyr`` in this file.

There are three types of configuration file:

1. **System**: Settings in this file affect west's behavior for every user
   logged in to the computer. Its location depends on the platform:

   - Linux: :file:`/etc/westconfig`
   - macOS: :file:`/usr/local/etc/westconfig`
   - Windows: :file:`%PROGRAMDATA%\\west\\config`

2. **Global** (per user): Settings in this file affect how west behaves when
   run by a particular user on the computer.

   - All platforms: the default is :file:`.westconfig` in the user's home
     directory.
   - Linux note: if the environment variable :envvar:`XDG_CONFIG_HOME` is set,
     then :file:`$XDG_CONFIG_HOME/west/config` is used.
   - Windows note: the following environment variables are tested to find the
     home directory: :envvar:`%HOME%`, then :envvar:`%USERPROFILE%`, then a
     combination of :envvar:`%HOMEDRIVE%` and :envvar:`%HOMEPATH%`.

3. **Local**: Settings in this file affect west's behavior for the
   current :term:`west workspace`. The file is :file:`.west/config`, relative
   to the workspace's root directory.

A setting in a file which appears lower down on this list overrides an earlier
setting. For example, if ``color.ui`` is ``true`` in the system's configuration
file, but ``false`` in the workspace's, then the final value is
``false``. Similarly, settings in the user configuration file override system
settings, and so on.

.. _west-config-cmd:

west config
-----------

The built-in ``config`` command can be used to get and set configuration
values. You can pass ``west config`` the options ``--system``, ``--global``, or
``--local`` to specify which configuration file to use. Only one of these can
be used at a time. If none is given, then writes default to ``--local``, and
reads show the final value after applying overrides.

Some examples for common uses follow; run ``west config -h`` for detailed help,
and see :ref:`west-config-index` for more details on built-in options.

To set ``manifest.path`` to :file:`some-other-manifest`:

.. code-block:: console

   west config manifest.path some-other-manifest

Doing the above means that commands like ``west update`` will look for the
:term:`west manifest` inside the :file:`some-other-manifest` directory
(relative to the workspace root directory) instead of the directory given to
``west init``, so be careful!

To read ``zephyr.base``, the value which will be used as ``ZEPHYR_BASE`` if it
is unset in the calling environment (also relative to the workspace root):

.. code-block:: console

   west config zephyr.base

You can switch to another zephyr repository without changing ``manifest.path``
-- and thus the behavior of commands like ``west update`` -- using:

.. code-block:: console

   west config zephyr.base some-other-zephyr

This can be useful if you use commands like ``git worktree`` to create your own
zephyr directories, and want commands like ``west build`` to use them instead
of the zephyr repository specified in the manifest. (You can go back to using
the directory in the upstream manifest by running ``west config zephyr.base
zephyr``.)

To set ``color.ui`` to ``false`` in the global (user-wide) configuration file,
so that west will no longer print colored output for that user when run in any
workspace:

.. code-block:: console

   west config --global color.ui false

To undo the above change:

.. code-block:: console

   west config --global color.ui true

.. _west-config-index:

Built-in Configuration Options
------------------------------

The following table documents configuration options supported by west's
built-in commands. Configuration options supported by Zephyr's extension
commands are documented in the pages for those commands.

.. NOTE: docs authors: keep this table sorted by section, then option.

.. list-table::
   :widths: 10 30
   :header-rows: 1

   * - Option
     - Description
   * - ``color.ui``
     - Boolean. If ``true`` (the default), then west output is colorized when
       stdout is a terminal.
   * - ``commands.allow_extensions``
     - Boolean, default ``true``, disables :ref:`west-extensions` if ``false``
   * - ``manifest.file``
     - String, default ``west.yml``. Relative path from the manifest repository
       root directory to the manifest file used by ``west init`` and other
       commands which parse the manifest.
   * - ``manifest.group-filter``
     - String, default empty. A comma-separated list of project groups to
       enable and disable within the workspace. Prefix enabled groups with
       ``+`` and disabled groups with ``-``. For example, the value
       ``"+foo,-bar"`` enables group ``foo`` and disables ``bar``. See
       :ref:`west-manifest-groups`.
   * - ``manifest.path``
     - String, relative path from the :term:`west workspace` root directory
       to the manifest repository used by ``west update`` and other commands
       which parse the manifest. Set locally by ``west init``.
   * - ``update.fetch``
     - String, one of ``"smart"`` (the default behavior starting in v0.6.1) or
       ``"always"`` (the previous behavior). If set to ``"smart"``, the
       :ref:`west-update` command will skip fetching
       from project remotes when those projects' revisions in the manifest file
       are SHAs or tags which are already available locally. The ``"always"``
       behavior is to unconditionally fetch from the remote.
   * - ``zephyr.base``
     - String, default value to set for the :envvar:`ZEPHYR_BASE` environment
       variable while the west command is running. By default, this is set to
       the path to the manifest project with path :file:`zephyr` (if there is
       one) during ``west init``. If the variable is already set, then this
       setting is ignored unless ``zephyr.base-prefer`` is ``"configfile"``.
   * - ``zephyr.base-prefer``
     - String, one the values ``"env"`` and ``"configfile"``. If set to
       ``"env"`` (the default), setting :envvar:`ZEPHYR_BASE` in the calling
       environment overrides the value of the ``zephyr.base`` configuration
       option. If set to ``"configfile"``, the configuration option wins
       instead.
