.. _release_process:

Release Process
###############

The Zephyr project releases on a time-based cycle, rather than a feature-driven
one. Zephyr releases represent an aggregation of the work of many contributors,
companies, and individuals from the community.

A time-based release process enables the Zephyr project to provide users with a
balance of the latest technologies and features and excellent overall quality. A
roughly 3-month release cycle allows the project to coordinate development of
the features that have actually been implemented, allowing the project to
maintain the quality of the overall release without delays because of one or two
features that are not ready yet.

The Zephyr release model is loosely based on the Linux kernel model:

- Release tagging procedure:

  - linear mode on master,
  - release branches for maintenance after release tagging.
- Each release period will consist of a merge window period followed by one or
  more release candidates on which only stabilization changes, bug fixes, and
  documentation can be merged in.

  - Merge window mode: all changes are accepted (subject to approval from the
    respective maintainers.)
  - When the merge window is closed, the release owner lays a vN-rc1 tag and the
    tree enters the release candidate phase
  - CI sees the tag, builds and runs tests; QA analyses the report from the
    build and test run and gives an ACK/NAK to the build
  - The release owner, with QA and any other needed input, determines if the
    release candidate is a go for release
  - If it is a go for a release, the release owner lays a tag release vN at the
    same point
- Development on new features continues in topic branches. Once features are
  ready, they are submitted to mainline during the merge window period and after
  the release is tagged.

.. figure:: release_cycle.png
    :align: center
    :alt: Release Cycle
    :figclass: align-center
    :width: 80%

    Release Cycle

Merge Window
*************

A relatively straightforward discipline is followed with regard to the merging
of patches for each release.  At the beginning of each development cycle, the
"merge window" is said to be open.  At that time, code which is deemed to be
sufficiently stable (and which is accepted by the development community) is
merged into the mainline tree.  The bulk of changes for a new development cycle
(and all of the major changes) will be merged during this time.

The merge window lasts for approximately two months.  At the end of this time,
the release owner will declare that the window is closed and release the first
of the release candidates.  For the codebase release which is destined to be
0.4.0, for example, the release which happens at the end of the merge window
will be called 0.4.0-rc1.  The -rc1 release is the signal that the time to merge
new features has passed, and that the time to stabilize the next release of the
code base has begun.

Over the next weeks, only patches which fix problems should be submitted to the
mainline.  On occasion, a more significant change will be allowed, but such
occasions are rare and require a TSC approval (Change Control Board). As a
general rule, if you miss the merge window for a given feature, the best thing
to do is to wait for the next development cycle.  (An occasional exception is
made for drivers for previously unsupported hardware; if they do not touch any
other in-tree code, they cannot cause regressions and should be safe to add at
any time).

As fixes make their way into the mainline, the patch rate will slow over time.
The mainline release owner releases new -rc drops once or twice a week; a normal
series will get up to somewhere between -rc4 and -rc6 before the code base is
considered to be sufficiently stable and the final 0.4.x release is made.

At that point, the whole process starts over again.

Here is the description of the various moderation levels:

- Low:

  - Major New Features
  - Bug Fixes
  - Refactoring
  - Structure/Directory Changes
- Medium:

  - Bug Fixes, all priorities
  - Enhancements
  - Minor "self-contained" New Features
- High:

  - Bug Fixes: P1 and P2
  - Documentation + Test Coverage

Releases
*********

The following syntax should be used for releases and tags in Git:

- Release [Major].[Minor].[Patch Level]
- Release Candidate [Major].[Minor].[Patch Level]-rc[RC Number]
- Tagging:

  - v[Major].[Minor].[Patch Level]-rc[RC Number]
  - v[Major].[Minor].[Patch Level]
  - v[Major].[Minor].99 - A tag applied to master branch to signify that work on
    v[Major].[Minor+1] has started. For example, v1.7.99 will be tagged at the
    start of v1.8 process. The tag corresponds to
    VERSION_MAJOR/VERSION_MINOR/PATCHLEVEL macros as defined for a
    work-in-progress master version. Presence of this tag allows generation of
    sensible output for "git describe" on master, as typically used for
    automated builds and CI tools.


.. figure:: release_flow.png
    :align: center
    :alt: Releases
    :figclass: align-center
    :width: 80%

    Zephyr Code and Releases


Long Term Support (LTS)
=======================

Long-term support releases are designed to be supported for a longer than normal
period and will be the basis for products and certification for various usages.

An LTS release is made every 2 years and is branched and maintained
independently from the mainline tree.

An LTS release will be branched and maintained independently of the mainline
tree.


.. figure:: lts.png
    :align: center
    :alt: Long Term Support Release
    :figclass: align-center
    :width: 80%

    Long Term Support Release

Changes and fixes flow in both directions. However, changes from master to an
LTS branch will be limited to fixes that apply to both branches and for existing
features only.

All fixes for an LTS branch that apply to the mainline tree are pushed to
mainline as well.


Auditable Code Base
===================

An auditable code base is to be established from a defined subset of Zephyr OS
features and will be limited in scope. The LTS,  development tree, and the
auditable code bases shall be kept in sync after the audit branch is created,
but with a more rigorous process in place for adding new features into the audit
branch used for certification.

This process will be applied before new features move into the
auditable code base.

The initial and subsequent certification targets will be decided by the Zephyr project
governing board.

Processes to achieve selected certification will be determined by the Security and
Safety Working Groups and coordinated with the TSC.


Release Procedure
******************

This section documents the Release manager responsibilities so that it serves as
a knowledge repository for Release managers.

Milestones
==========

The following graphic shows the timeline of phases and milestones associated
with each release:

.. figure:: milestones.jpg
    :align: center
    :alt: Release Milestones
    :figclass: align-center
    :width: 80%

    Release milestones

This shows how the phases and milestones of one release overlap with those of
the next release:


.. figure:: milestones2.jpg
    :align: center
    :alt: Release Milestones
    :figclass: align-center
    :width: 80%

    Release milestones with planning


.. csv-table:: Milestone Description
   :header: "Milestone", "Description", "Definition"
   :widths: auto


   "P0","Planning Kickoff","Start Entering Requirements"
   "P1","","TSC Agrees on Major Features and Schedule"
   "M0","Merge Window Open","All features, Sized, and AssignedMerge Window Is Opened"
   "M1","M1 Checkpoint","Major Features Ready for Code Reviews |br| Test Plans Reviewed and Approved"
   "M2","Feature Merge Window Close","Feature Freeze |br| Feature Development Complete (including Code Reviews and Unit Tests Passing) |br| P1 Stories Implemented |br| Feature Merge Window Is Closed |br| Test Development Complete |br| Technical Documentation Created/Updated and Ready for Review |br| CCB Control Starts"
   "M3","Code Freeze","Code Freeze |br| RC3 Tagged and Built"
   "M4","Release","TSC Reviews the Release Criteria Report and Approves Release
   |br| Final RC Tagged |br| Make the Release"

Release Checklist
=================

Each release has a GitHub issue associated with it that contains the full
checklist. After a release is complete, a checklist for the next release is
created.

Tagging
=======

.. note::

    This section uses tagging 1.11.0-rc1 as an example, replace with the
    appropriate version.

Every time a release candidate (or the final release) needs to be tagged, the
following steps need to be followed:

.. note::

    Tagging needs to be done via explicit git commands and not via GitHub's release
    interface.  The GitHub release interface does not generate annotated tags (it
    generates 'lightweight' tags regardless of release or pre-release).

#. Update the :zephyr_file:`VERSION` file in the root of the Git repository. If it's a
release candidate, use ``EXTRAVERSION`` variable::

    EXTRAVERSION = rc1

#. Commit the update to the :zephyr_file:`VERSION` file, use ``release:`` as a commit
   tag.
#. Check that CI has completed successfully before tagging.
#. Tag and push the version, using annotated tags:

   * If it's a release candidate::

      $ git tag -a v1.11.0-rc1
      <Use "Zephyr 1.11.0-rc1" as the tag annotation>
      $ git push git@github.com:zephyrproject-rtos/zephyr.git v1.11.0-rc1

  * If it's a release::

      $ git tag -a v1.11.0
      <Use "Zephyr 1.11.0" as the tag annotation>
      $ git push git@github.com:zephyrproject-rtos/zephyr.git v1.11.0

      $ git tag -a zephyr-v1.11.0
      <Use "Zephyr 1.11.0" as the tag annotation>
      $ git push git@github.com:zephyrproject-rtos/zephyr.git zephyr-v1.11.0

#. If it's a release candidate, create a shortlog of changes between the
   previous release::

    $ git shortlog v1.10.0..v.1.11.0-rc1

#. Find the new tag at the top of the releases page, edit the release with the
   ``Edit`` button and then do the following:

  * If it's a release candidate:

    * Name it ``Zephyr 1.11.0-rc1``
    * Copy the shortlog into the release notes textbox (don't forget to quote it
      properly so it shows as unformatted text in Markdown)
    * Check the "This is a pre-release" checkbox
  * If it's a release:

    * Name it ``Zephyr 1.11.0``
    * Copy the full content of ``docs/release-notes-1.11.rst`` into the the
      release notes textbox
    * Copy the full list of GitHub issues closed with this release into the
      release notes textbox (see below on how to generate this list)

#. Send an email to the mailing lists (``announce`` and ``devel``) with a link
   to the release

Listing all closed GitHub issues
=================================

The release notes for a final release contain the list of GitHub issues that
have been closed during the development process of that release.

In order to obtain the list of issues closed during the release development
cycle you can do the following:

#. Look for the last release before the current one and find the day it was
   tagged::

    $ git show zephyr-v1.10.0
    tag zephyr-v1.10.0
    Tagger: Kumar Gala <kumar.gala@linaro.org>
    Date:   Fri Dec 8 14:26:35 2017 -0600


#. Use available release tools to list all the issues that have been closed
   between that date and the day of the release.
