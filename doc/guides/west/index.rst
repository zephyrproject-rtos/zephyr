.. _west:

West (Zephyr's meta-tool)
#########################

The Zephyr project includes a swiss-army knife command line tool named
``west``\ [#west-name]_. West is developed in its own `repository`_.  Like
``git`` and ``docker``, the top-level ``west`` command takes some common
options, a sub-command to run, and then options and arguments for that
sub-command::

  west [common-opts] <command> [opts] <args>

West's built-in commands provide a multiple repository management
system with features inspired by Google's Repo tool and Git
submodules. West simplifies configuration and is also pluggable: you
can write your own west "extension commands" which add additional
features to west.  Zephyr uses this feature to provide conveniences
for building applications, flashing and debugging them, and more.

It is possible not to use west for Zephyr development if you do not
require these features, prefer to use your own tools, or want to
eliminate the extra layer of indirection. However, this implies extra
effort and expert knowledge.

You can run ``west --help`` (or ``west -h`` for short) to get top-level help
for available west commands, and ``west <command> -h`` for detailed help on
each command.

The following pages document west's ``v0.6.x`` releases, and provide additional
context about the tool.

.. toctree::
   :maxdepth: 1

   install.rst
   moving-to-west.rst
   troubleshooting.rst
   repo-tool.rst
   manifest.rst
   config.rst
   extensions.rst
   build-flash-debug.rst
   sign.rst
   zephyr-cmds.rst
   why.rst
   without-west.rst
   planned.rst
   release-notes.rst

For details on west's Python APIs, see :ref:`west-apis`.

.. rubric:: Footnotes

.. [#west-name]

   Zephyr is an English name for the Latin `Zephyrus
   <https://en.wiktionary.org/wiki/Zephyrus>`_, the ancient Greek god of the
   west wind.

.. _repository:
   https://github.com/zephyrproject-rtos/west
