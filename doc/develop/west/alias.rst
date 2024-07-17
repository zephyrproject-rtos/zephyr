.. _west-aliases:

West aliases
############

West allows to add alias commands to the local, global or system configuration files.
These aliases make it easy to add shortcuts for frequently used, or hard to memorize
commands for ease of development.

Similar to how ``git`` aliases work, the alias command is replaced with the alias'
full text and parsed as a new shell argument list. This enables adding argument
parameters as they were passed to the original command. Spaces are considered
argument separators, use proper escaping if arguments shouldn't be split.

To add a new alias simply call the ``west config`` command:

.. code-block:: console

   west config alias.command "other-command --with-arg=\"some value\""

Recursive aliases are allowed as an alias command can contain other aliases, effectively
building more complex, but easy to remember commands.

It is possible to override an existing command, for example to pass default arguments:

.. code-block:: console

   west config alias.update "update -o=--depth=1 -n"

.. warning::

   Overriding/shadowing other or built-in commands is an advanced use case, it can lead to
   strange side-effects and should be done with great care.

Examples
--------

Add ``west run`` and ``west menuconfig`` shortcuts to your global configuration that
call ``west build`` with the corresponding CMake targets:

.. code-block:: console

   west config --global alias.run "build --pristine=never --target run"
   west config --global alias.menuconfig "build --pristine=never --target menuconfig"

Create an alias for the sample you are actively developing with additional options:

.. code-block:: console

   west config alias.sample "build -b native_sim samples/hello_world -t run -- -DCONFIG_ASSERT=y"

Override ``west update`` to check a local cache:

.. code-block:: console

   west config alias.update "update --path-cache $HOME/.cache/zephyrproject"
