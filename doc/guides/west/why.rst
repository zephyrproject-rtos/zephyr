.. _west-history:

History and Motivation
######################

West was added to the Zephyr project to fulfill two fundamental requirements:

* The ability to work with multiple Git repositories
* The ability to provide a user-friendly command-line interface to the Zephyr
  build system and debug mechanisms

Additionally, it was desired that west be easily extensible by
downstream users.

During the development of west, a set of :ref:`west-design-constraints` were
identified to avoid the common pitfalls of tools of this kind.

Multiple Git Repositories
*************************

Zephyr intends to provide all required building blocks needed to deploy complex
IoT applications. This in turn means that the Zephyr project is much more than
an RTOS kernel, and is instead a collection of components that work together.
In this context, there are a few reasons to work with multiple Git
repositories in a standardized manner within the project:

* Clean separation of Zephyr original code and imported projects and libraries
* Avoidance of license incompatibilities between original and imported code
* Reduction in size and scope of the core Zephyr codebase, with additional
  repositories containing optional components instead of being imported
  directly into the tree
* Safety and security certifications
* Enforcement of modularization of the components
* Out-of-tree development based on subsets of the supported boards and SoCs

See :ref:`west-multi-repo` for a detailed explanation of west's handling of
multiple repository management.

.. _west-design-constraints:

Design Constraints
******************

West is:

- **Optional**: it is always *possible* to drop back to "raw"
  command-line tools, i.e. use Zephyr without using west (although west itself
  might need to be installed and accessible to the build system). It may not
  always be *convenient* to do so, however. (If all of west's features
  were already conveniently available, there would be no reason to
  develop it.)

- **Compatible with CMake**: building, flashing and debugging, and
  emulator support will always remain compatible with direct use of
  CMake.

- **Cross-platform**: West is written in Python 3, and works on all
  platforms supported by Zephyr.

- **Usable as a Library**: whenever possible, west features are
  implemented as libraries that can be used standalone in other
  programs, along with separate command line interfaces that wrap
  them.  West itself is a Python package named ``west``; its libraries
  are implemented as subpackages.

- **Conservative about features**: no features will be accepted without
  strong and compelling motivation.

- **Clearly specified**: West's behavior in cases where it wraps other
  commands is clearly specified and documented. This enables
  interoperability with third party tools, and means Zephyr developers
  can always find out what is happening "under the hood" when using west.

See `Zephyr issue #6205`_ and for more details and discussion.

.. _Zephyr issue #6205:
   https://github.com/zephyrproject-rtos/zephyr/issues/6205
