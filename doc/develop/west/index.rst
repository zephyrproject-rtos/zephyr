.. _west:

West (Zephyr's meta-tool)
#########################

The Zephyr project includes a swiss-army knife command line tool named
``west``\ [#west-name]_. West is developed in its own `repository`_.

West's built-in commands provide a multiple repository management system with
features inspired by Google's Repo tool and Git submodules. West is also
"pluggable": you can write your own west extension commands which add
additional features to west. Zephyr uses this to provide conveniences for
building applications, flashing and debugging them, and more.

Like ``git`` and ``docker``, the top-level ``west`` command takes some common
options, a sub-command to run, and then options and arguments for that
sub-command::

  west [common-opts] <command> [opts] <args>

Since west v0.8, you can also run west like this::

  python3 -m west [common-opts] <command> [opts] <args>

You can run ``west --help`` (or ``west -h`` for short) to get top-level help
for available west commands, and ``west <command> -h`` for detailed help on
each command.

The following pages document west's ``v1.0.y`` releases, and provide additional
context about the tool.

.. toctree::
   :maxdepth: 1

   install.rst
   release-notes.rst
   troubleshooting.rst
   basics.rst
   built-in.rst
   workspaces.rst
   manifest.rst
   config.rst
   extensions.rst
   build-flash-debug.rst
   sign.rst
   zephyr-cmds.rst
   why.rst
   moving-to-west.rst
   without-west.rst

For details on west's Python APIs, see :ref:`west-apis`.

.. rubric:: Footnotes

.. [#west-name]

   Zephyr is an English name for the Latin `Zephyrus
   <https://en.wiktionary.org/wiki/Zephyrus>`_, the ancient Greek god of the
   west wind.

.. _repository:
   https://github.com/zephyrproject-rtos/west
