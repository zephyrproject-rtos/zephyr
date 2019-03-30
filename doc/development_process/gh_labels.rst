.. _gh_labels:

Labeling issues and pull requests in GitHub
###########################################

The project uses GitHub issues and pull requests (PRs) to track and manage
daily and long-term work and contributions to the Zephyr project. We use
GitHub **labels** to classify and organize these issues and PRs by area, type,
priority, and more, making it easier to find and report on relevant items.

All GitHub issues or pull requests must be appropriately labeled.
Issues and PRs often have multiple labels assigned,
to help classify them in the different available categories.
When reviewing a PR, if it has missing or incorrect labels, maintainers shall
fix it.

This saves us all time when searching, reduces the chances of the PR or issue
being forgotten, speeds up reviewing, avoids duplicate issue reports, etc.

These are the labels we currently have, grouped by type:

Area:
*****

=============  ===============================================================
Labels         ``Area:*``
Applicable to  PRs  and issues
Description    Indicates subsystems (e.g., Kernel, I2C, Memory Management),
               project functions (e.g., Debugging, Documentation, Process),
               or other categories (e.g., Coding Style, MISRA-C)  affected by
               the bug or pull request.
=============  ===============================================================

An area maintainer should be able to filter by an area label and
find all issues and PRs which relate to that area.

Platform
*********

=============  ===============================================================
Labels         ``Platform:*``
Applicable to  PRs  and issues
Description    An issue or PR which affects only a particular platform
=============  ===============================================================

To be discussed in a meeting
****************************

=============  ===============================================================
Labels         ``API``, ``dev-review``, ``TSC``
Applicable to  PRs  and issues
Description    The issue is to be discussed in the following
               `API/dev-review/TSC meeting`_ if time permits
=============  ===============================================================

.. _`API/dev-review/TSC meeting`: https://github.com/zephyrproject-rtos/zephyr/wiki/Zephyr-Committee-and-Working-Group-Meetings

Minimum PR review time
**********************

=============  ===============================================================
Labels         ``Hot Fix``, ``Trivial``, ``Maintainer``,
               ``Security Review``, ``TSC``
Applicable to  PRs only
Description    Depending on the PR complexity, an indication of how long a merge
               should be held to ensure proper review. See
               :ref:`review process <review_time>`
=============  ===============================================================

Issue priority labels
*********************

=============  ===============================================================
Labels         ``priority:{high|medium|low}``
Applicable to  Issues only
Description    To classify the impact and importance of a bug or feature
=============  ===============================================================

Note: Issue priorities are generally set or changed during the bug-triage or TSC
meetings.

Miscellaneous labels
********************

For both PRs and issues
=======================

+------------------------+-----------------------------------------------------+
|``Bug``                 | The issue is a bug, or the PR is fixing a bug       |
+------------------------+-----------------------------------------------------+
|``Coverity``            | A Coverity detected issue or its fix                |
+------------------------+-----------------------------------------------------+
|``Waiting for response``| The Zephyr developers are waiting for the submitter |
|                        | to respond to a question, or address an issue.      |
+------------------------+-----------------------------------------------------+
|``Blocked``             | Blocked by another PR or issue                      |
+------------------------+-----------------------------------------------------+
|``In progress``         | For PRs: is work in progress and should not be      |
|                        | merged yet. For issues: Is being worked on          |
+------------------------+-----------------------------------------------------+
|``RFC``                 | The author would like input from the community. For |
|                        | a PR it should be considered a draft                |
+------------------------+-----------------------------------------------------+
|``LTS``                 | Long term release branch related                    |
+------------------------+-----------------------------------------------------+
|``EXT``                 | Related to an external component (in ``ext/``)      |
+------------------------+-----------------------------------------------------+

PR only labels
==============

================ ===============================================================
``DNM``          This PR should not be merged (Do Not Merge).
                 For work in progress, GitHub "draft" PRs are preferred
``Stale PR``     PR which seems abandoned, and requires attention by the author
``Needs review`` The PR needs attention from the maintainers
``Backport``     The PR is a backport or should be backported
``Licensing``    The PR has licensing issues which require a licensing expert to
                 review it
================ ===============================================================

Issue only labels
=================

==================== ===========================================================
``Regression``       Something, which was working, but does not anymore
                     (bug subtype)
``Question``         This issue is a question to the Zephyr developers
``Enhancement``      Changes/Updates/Additions to existing features
``Feature request``  A request for a new feature
``Feature``          A planned feature with a milestone
``Duplicate``        This issue is a duplicate of another issue
                     (please specify)
``Good first issue`` Good for a first time contributor to take
``Release Notes``    Issues that need to be mentioned in release notes as known
                     issues with additional information
==================== ===========================================================

Any issue must be clasified and labeled as either ``Bug``, ``Question``,
``Enhancement``, ``Feature``, or ``Feature Request``.
