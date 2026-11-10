.. _west:

West (Zephyr's meta-tool)
#########################

The Zephyr project includes a swiss-army knife command line tool named
``west``. West is developed in its own `repository`_, and the `west
documentation`_ covers its installation, built-in commands, workspaces,
manifest files, configuration and extension commands.

West is "pluggable": extension commands add features to it. Zephyr uses this
to provide conveniences for building applications, flashing and debugging
them, and more. These Zephyr extension commands, and how to use Zephyr without
west, are documented here.

.. toctree::
   :maxdepth: 1

   build-flash-debug.rst
   sign.rst
   zephyr-cmds.rst
   without-west.rst

.. _repository:
   https://github.com/zephyrproject-rtos/west

.. _west documentation:
   https://docs.zephyrproject.org/west/latest/
