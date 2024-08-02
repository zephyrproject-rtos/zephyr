.. _west-history:

History and Motivation
######################

West was added to the Zephyr project to fulfill two fundamental requirements:

* The ability to work with multiple Git repositories
* The ability to provide an extensible and user-friendly command-line interface
  for basic Zephyr workflows

During the development of west, a set of :ref:`west-design-constraints` were
identified to avoid the common pitfalls of tools of this kind.

Requirements
************

Although the motivation behind splitting the Zephyr codebase into multiple
repositories is outside of the scope of this page, the fundamental requirements,
along with a clear justification of the choice not to use existing tools and
instead develop a new one, do belong here.

The basic requirements are:

* **R1**: Keep externally maintained code in separately maintained repositories
  outside of the main zephyr repository, without requiring users to manually
  clone each of the external repositories
* **R2**: Provide a tool that both Zephyr users and distributors can make use of
  to benefit from and extend
* **R3**: Allow users and downstream distributions to override or remove
  repositories without having to make changes to the zephyr repository
* **R4**: Support both continuous tracking and commit-based (bisectable) project
  updating


Rationale for a custom tool
***************************

Some of west's features are similar to those provided by
`Git Submodules <https://git-scm.com/book/en/v2/Git-Tools-Submodules>`_ and
Google's `repo <https://gerrit.googlesource.com/git-repo/>`_.

Existing tools were considered during west's initial design and development.
None were found suitable for Zephyr's requirements. In particular, these were
examined in detail:

* Google repo

  - Does not cleanly support using zephyr as the manifest repository (**R4**)
  - Python 2 only
  - Does not play well with Windows
  - Assumes Gerrit is used for code review

* Git submodules

  - Does not fully support **R1**, since the externally maintained repositories
    would still need to be inside the main zephyr Git tree
  - Does not support **R3**, since downstream copies would need to either
    delete or replace submodule definitions
  - Does not support continuous tracking of the latest ``HEAD`` in external
    repositories (**R4**)
  - Requires hardcoding of the paths/locations of the external repositories

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

See :ref:`west-basics` for information on how west workspaces manage multiple
git repositories.

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

See :github:`Zephyr issue #6205 <6205>` and for more details and discussion.
