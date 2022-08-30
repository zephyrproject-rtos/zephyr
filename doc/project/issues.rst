.. _bug_reporting:

Bug Reporting
##############

To maintain traceability and relation between proposals, changes, features, and
issues, it is recommended to cross-reference source code commits with the
relevant GitHub issues and vice versa.
Any changes that originate from a tracked feature or issue should contain a
reference to the feature by mentioning the corresponding issue or pull-request
identifiers.

At any time it should be possible to establish the origin of a change and the
reason behind it by following the references in the code.

Reporting a regression issue
****************************

It could happen that the issue being reported is identified as a regression,
as the use case is known to be working on earlier commit or release.
In this case, providing directly the guilty commit when submitting the bug
gains a lot of time in the eventual bug fixing.

To identify the commit causing the regression, several methods could be used,
but tree bisecting method is an efficient one that doesn't require deep code
expertise and can be used by every one.

For this, `git bisect`_ is the recommended tool.

Recommendations on the process:

* Run ``west update`` on each bisection step.
* Once the bisection is over and a culprit identified, verify manually the result.

.. _git bisect:
   https://git-scm.com/docs/git-bisect
