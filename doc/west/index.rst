.. _west:

West (Experimental)
###################

The Zephyr project is developing a swiss-army knife command line tool
named ``west``. (Zephyr is an English name for the Greek god of the
west wind.)

West is developed in its own `repository on GitHub`_. A copy is
currently maintained in Zephyr's :file:`scripts/meta/west` directory.

Usage
*****

West's usage is similar to Git's: general options are followed by a
command, which may also take options and arguments::

  $ west [common-opts] <command-name> [command-opts] [<command-args>]

Usage guides for specific groups of subcommands are in the following
pages.

.. toctree::

   flash-debug.rst

(This list will expand as additional features are developed.)

Planned Features
****************

Additional features are under development for future versions of
Zephyr:

- Building Zephyr images.

- Running Zephyr in emulation.

- Bootloader integration: bootloader-aware image building, signing,
  and flashing, as well as building and flashing the bootloader itself.

- Multiple repository support: fetching and updating repositories that
  integrate with Zephyr, such as `MCUboot`_, `OpenThread`_ and others.

See `Zephyr issue #6205`_ for more details and discussion.

.. _west-design-constraints:

Design Constraints
******************

West is:

- **Optional**: it is always *possible* to drop back to "raw"
  command-line tools, i.e. use Zephyr without using West. It may not
  always be *convenient* to do so, however. (If all of West's features
  were already conveniently available, there would be no reason to
  develop it.)

- **Compatible with CMake**: building, flashing and debugging, and
  emulator support will always remain compatible with direct use of
  CMake.

- **Cross-platform**: West is written in Python 3, and works on all
  platforms supported by Zephyr.

- **Usable as a Library**: whenever possible, West features are
  implemented as libraries that can be used standalone in other
  programs, along with separate command line interfaces that wrap
  them.  West itself is a Python package named ``west``; its libraries
  are implemented as subpackages.

- **Conservative about features**: no features will be accepted without
  strong and compelling motivation.

- **Clearly specified**: West's behavior in cases where it wraps other
  commands is clearly specified and documented. This enables
  interoperability with third party tools, and means Zephyr developers
  can always find out what is happening "under the hood" when using West.

.. _repository on GitHub:
   https://github.com/zephyrproject-rtos/west

.. _MCUboot:
   https://mcuboot.com/

.. _OpenThread:
   https://openthread.io/

.. _Zephyr issue #6205:
   https://github.com/zephyrproject-rtos/zephyr/issues/6205
