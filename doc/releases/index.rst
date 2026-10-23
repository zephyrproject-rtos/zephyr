.. _zephyr_release_notes:

Releases
########

Zephyr is distributed as source code and build scripts, not as a binary image.
Use :ref:`west` to :ref:`get the code <get_the_code>` for a specific version,
and see the `GitHub repository`_ for the full history of tagged releases.

The technical documentation for current and past releases is available at
https://docs.zephyrproject.org/ (use the version selector to select your release
of interest).

.. _supported_releases:

Supported Releases
******************

The table below lists all actively supported releases. For most users, the
recommended starting point is either the **latest stable release** or the
**current LTS release**.

.. toctree::
   :hidden:
   :maxdepth: 1
   :glob:
   :reversed:

   release-notes-3.7
   release-notes-4.[3-5]
   migration-guide-3.7
   migration-guide-4.[3-5]

.. note::
   | The next planned release is **Zephyr 4.5**, targeted for **October 2026**.
   | The working drafts of the associated :doc:`Release Notes <release-notes-4.5>`
     and :doc:`Migration Guide <migration-guide-4.5>` are already available.

.. list-table::
    :header-rows: 1

    * - Release
      - Release Date
      - EOL
      - Status
      - Supporting Documentation
    * - `Zephyr 4.4.0`_
      - 2026-04-14
      - 2027-04-12
      - Latest stable
      - * :doc:`Release Notes <release-notes-4.4>`
        * :doc:`Migration Guide <migration-guide-4.4>`
    * - `Zephyr 4.3.0`_
      - 2025-11-14
      - 2026-10-15
      - Stable
      - * :doc:`Release Notes <release-notes-4.3>`
        * :doc:`Migration Guide <migration-guide-4.3>`
    * - `Zephyr 3.7.0 (LTS3)`_
      - 2024-07-26
      - 2029-07-27
      - Long Term Support
      - * :doc:`Release Notes <release-notes-3.7>`
        * :doc:`Migration Guide <migration-guide-3.7>`

Previous LTS releases that have reached end-of-life:

+-------------------------+---------------+
| Release                 | EOL           |
+=========================+===============+
| `Zephyr 2.7.6 (LTS2)`_  | 2025-01-26    |
+-------------------------+---------------+
| `Zephyr 1.14.1 (LTS1)`_ | 2022-01-01    |
+-------------------------+---------------+

End-of-life releases
=====================

.. toctree::
   :hidden:
   :maxdepth: 1

   eol_releases

End-of-life releases are no longer maintained and do not receive security fixes.
The release notes and migration guides for these releases are available
:ref:`here <eol_releases>`.

.. _zephyr_release_cycle:

Release Life Cycle and Maintenance
**********************************

Major and Maintenance Releases
==============================

Zephyr delivers major releases on a **six-month cadence**, targeting April and
October each year. This schedule provides regular, well-tested releases without
overwhelming users with too-frequent updates, and avoids major holidays across
geographies.

Maintenance (point) releases are published on an unscheduled basis when enough
significant fixes have accumulated on a major release branch. Each point release
goes through a full QA cycle before publishing.

Long Term Support and Maintenance
=================================

While stable releases are supported for the duration of 2 release cycles (roughly 1 year),
some specific ones will be supported for a longer period by the Zephyr Project,
and are called Long Term Support (LTS) releases.

A Zephyr :ref:`Long Term Support (LTS) <release_process_lts>` release is
published every 2.5 to 3 years and is branched and maintained independently from the
main tree for approximately 5 years after it was released.

This offers more stability to project users and leaves more time to
upgrade to the following LTS release.


Transitioning to the new Release Cadence
========================================

The transition to the new release cadence will begin in 2026. Zephyr 4.4 is scheduled
for release in April 2026, and subsequent releases will occur every six months.

The projected timeline for upcoming releases is as follows:

+---------+-------------------+---------------------+
| Release | Planned Date      | Notes               |
+=========+===================+=====================+
| 4.4     | April 2026        |                     |
+---------+-------------------+---------------------+
| 4.5     | October 2026      |                     |
+---------+-------------------+---------------------+
| 4.6     | April 2027        | LTS4                |
+---------+-------------------+---------------------+
| 5.0     | October 2027      | Start of 5.x cycle  |
+---------+-------------------+---------------------+
| 5.1     | April 2028        |                     |
+---------+-------------------+---------------------+
| 5.2     | October 2028      |                     |
+---------+-------------------+---------------------+
| 5.3     | April 2029        |                     |
+---------+-------------------+---------------------+
| 5.4     | October 2029      | LTS5                |
+---------+-------------------+---------------------+

Starting with the 5.x release cycle, all releases will follow the new six-month
cadence from the beginning.


Security Fixes
==============

Each security issue fixed within Zephyr is backported or submitted to the
following releases:

- Currently supported Long Term Support (LTS) release.

- The most recent two releases.

For more information, see  :ref:`Security Vulnerability Reporting <reporting>`.

.. _release_documentation:

Release documentation
*********************

Each release is documented in three related documents, each with a distinct purpose:

- Release notes document the capabilities and the impact of the release.
- Migration guides document the actions required when moving an application from one major
  release to the next.
- The change inventory, generated from the tree and published as an appendix to the release notes,
  documents the individual interface and configuration changes.

During the development cycle, contributors add to the working drafts for the next release,
:zephyr_file:`doc/releases/release-notes-draft.rst` and
:zephyr_file:`doc/releases/migration-guide-draft.rst`, in the same pull request as the change
they describe. An entry that misses a release stays in the draft and targets the next one.

.. _release_notes_guidelines:

Release Notes
=============

Release notes provide a concise, curated summary of the significant changes in a release, focusing
on new capabilities, major improvements, and changes that materially affect users and developers.
They explain what changed and why it matters, rather than provide an exhaustive inventory of new
APIs, Kconfig options, devicetree bindings, drivers, or other implementation details.

Individual changes are included only when they introduce meaningful functionality, affect existing
behavior, or require user awareness. Related changes are grouped and summarized, with links to the
detailed documentation where appropriate. Breaking changes, deprecations, and required upgrade
actions belong in the migration guide, while comprehensive lists of interface and configuration
changes are maintained separately in the change inventory. The goal is to communicate the
significance and impact of a release, not to reproduce its development history.

Updates to the release notes post release cycle is permitted but limited to
style, typographical fixes and to upmerge the notes from maintenance release
branches with the sole purpose of keeping the latest documentation consistent
with the changes in the project.

.. _migration_guide_guidelines:

Migration Guides
================

Zephyr provides migration guides for all major releases, in order to assist
users transition from the previous release.

As mentioned in the previous section, changes in the code that require an action
(i.e. a modification of the source code or configuration files) on the part of
the user in order to keep the existing behavior of their application belong in
in the migration guide. This includes:

- Breaking API changes
- Deprecations
- Devicetree or Kconfig changes that affect the user (changes to defaults,
  renames, etc)
- Treewide changes that have an effect (e.g. changing the include path or
  defaulting to a different C standard library)
- Anything else that can affect the compilation or runtime behavior of an
  existing application

Each entry in the migration guide must include a brief explanation of the change
as well as refer to the Pull Request that introduced it, in order for the user
to be able to understand the context of the change.

.. _change_inventory_guidelines:

Change Inventory
================

The change inventory lists every board, shield, SoC, devicetree binding, Kconfig option, C API,
code sample and module that was added, removed, deprecated or changed since the previous release,
each linked to its documentation. It is generated from the tree by
:zephyr_file:`scripts/release/gen_change_inventory.py`, which compares two revisions, and is never
edited by hand: contributors do not list new or removed items in the release notes, they make sure
the item is documented where the inventory links to, such as the binding description, the Kconfig
help text, the API documentation or the board page.

The release manager regenerates the inventory for each release candidate and for the release,
for example::

   scripts/release/gen_change_inventory.py --title "Zephyr 4.5 change inventory" \
       v4.4.0 v4.5.0-rc1 doc/releases/change-inventory-4.5

.. _`GitHub repository`: https://github.com/zephyrproject-rtos/zephyr
.. _`GitHub tagged releases`: https://github.com/zephyrproject-rtos/zephyr/tags
.. _`Zephyr 1.14.1 (LTS1)`: https://docs.zephyrproject.org/1.14.1/
.. _`Zephyr 2.7.6 (LTS2)`: https://docs.zephyrproject.org/2.7.6/
.. _`Zephyr 3.7.0 (LTS3)`: https://docs.zephyrproject.org/3.7.0/
.. _`Zephyr 4.3.0`: https://docs.zephyrproject.org/4.3.0/
.. _`Zephyr 4.4.0`: https://docs.zephyrproject.org/4.4.0/
