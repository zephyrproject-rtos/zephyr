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
considered to be sufficiently stable and the quality metrics have been achieved
at which point the final 0.4.x release is made.

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

Release Quality Criteria
************************

The current backlog of prioritized bugs shall be used as a quality metric to
gate the final release. The following counts shall be used:

.. csv-table:: Bug Count Release Thresholds
   :header: "High", "Medium", "Low"
   :widths: auto


   "0","<20","<50"

.. note::

   The "low" bug count target of <50 will be a phased appoach starting with 150
   for release 2.4.0, 100 for release 2.5.0, and 50 for release 2.6.0


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

The final release and each release candidate shall be tagged using the following
steps:

.. note::

    Tagging needs to be done via explicit git commands and not via GitHub's release
    interface.  The GitHub release interface does not generate annotated tags (it
    generates 'lightweight' tags regardless of release or pre-release). You should
    also upload your gpg public key to your GitHub account, since the instructions
    below involve creating signed tags. However, if you do not have a gpg public
    key you can opt to remove the ``-s`` option from the commands below.

.. tabs::

    .. tab:: Release Candidate

        .. note::

            This section uses tagging 1.11.0-rc1 as an example, replace with
            the appropriate release candidate version.

        #. Update the version variables in the :zephyr_file:`VERSION` file
           located in the root of the Git repository to match the version for
           this release candidate. The ``EXTRAVERSION`` variable is used to
           identify the rc[RC Number] value for this candidate::

            EXTRAVERSION = rc1

        #. Post a PR with the updated :zephyr_file:`VERSION` file using
           ``release: Zephyr 1.11.0-rc1`` as the commit subject. Merge
           the PR after successful CI.
        #. Tag and push the version, using an annotated tag::

            $ git pull
            $ git tag -s -m "Zephyr 1.11.0-rc1" v1.11.0-rc1
            $ git push git@github.com:zephyrproject-rtos/zephyr.git v1.11.0-rc1

        #. Create a shortlog of changes between the previous release (use
           rc1..rc2 between release candidates)::

            $ git shortlog v1.10.0..v1.11.0-rc1

        #. Find the new tag at the top of the releases page and edit the release
           with the ``Edit tag`` button with the following:

            * Name it ``Zephyr 1.11.0-rc1``
            * Copy the shortlog into the release notes textbox (*don't forget
              to quote it properly so it shows as unformatted text in Markdown*)
            * Check the "This is a pre-release" checkbox

        #. Send an email to the mailing lists (``announce`` and ``devel``)
           with a link to the release

    .. tab:: Final Release

        .. note::

            This section uses tagging 1.11.0 as an example, replace with the
            appropriate final release version.

        When all final release criteria has been met and the final release notes
        have been approved and merged into the repository, the final release version
        will be set and repository tagged using the following procedure:

        #. Update the version variables in the :zephyr_file:`VERSION` file
           located in the root of the Git repository. Set ``EXTRAVERSION``
           variable to an empty string to indicate final release::

            EXTRAVERSION =

        #. Post a PR with the updated :zephyr_file:`VERSION` file using
           ``release: Zephyr 1.11.0`` as the commit subject. Merge
           the PR after successful CI.
        #. Tag and push the version, using two annotated tags::

            $ git pull
            $ git tag -s -m "Zephyr 1.11.0" v1.11.0
            $ git push git@github.com:zephyrproject-rtos/zephyr.git v1.11.0

            # This is the tag that will represent the release on GitHub, so that
            # the file you can download is named ``zephyr-v1.11.0.zip`` and not
            # just ``v1.11.0.zip``
            $ git tag -s -m "Zephyr 1.11.0" zephyr-v1.11.0
            $ git push git@github.com:zephyrproject-rtos/zephyr.git zephyr-v1.11.0

        #. Find the new ``zephyr-v1.11.0`` tag at the top of the releases page
           and edit the release with the ``Edit tag`` button with the following:

            * Name it ``Zephyr 1.11.0``
            * Copy the full content of ``docs/releases/release-notes-1.11.rst``
              into the release notes textbox

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

    $ git show -s --format=%ci zephyr-v1.10.0
    tag zephyr-v1.10.0
    Tagger: Kumar Gala <kumar.gala@linaro.org>

    Zephyr 1.10.0
    2017-12-08 13:32:22 -0600


#. Use available release tools to list all the issues that have been closed
   between that date and the day of the release.
