.. _contribute_non-Apache:

Contributing non-Apache 2.0 licensed components
###############################################

Importing code into the Zephyr OS from other projects that use a license
other than the Apache 2.0 license needs to be fully understood in
context and approved by the `Zephyr governing board`_.

.. _Zephyr governing board:
   https://www.zephyrproject.org/about/organization

By carefully reviewing potential contributions and also enforcing a
:ref:`DCO` for contributed code, we ensure that
the Zephyr community can develop products with the Zephyr Project
without concerns over patent or copyright issues.

Submission and review process
*****************************

All contributions to the Zephyr project are submitted through GitHub
pull requests (PR) following the Zephyr Project's :ref:`Contribution workflow`.

Before you begin working on including a new component to the Zephyr
Project (Apache-2.0 licensed or not), you should start up a conversation
on the `developer mailing list <https://lists.zephyrproject.org>`_
to see what the Zephyr community thinks about the idea.  Maybe there's
someone else working on something similar you can collaborate with, or a
different approach may make the new component unnecessary.

If the conclusion is that including a new component is the best
solution, and this new component uses a license other than Apache-2.0,
these additional steps must be followed:

#. Complete a README for your code component and add it to your source
   code pull request (PR).  A recommended README template can be found in
   :file:`doc/contribute/code_component_README` (and included
   `below`_ for reference)

#. The Zephyr Technical Steering Committee (TSC) will evaluate the code
   component README as part of the PR
   commit and vote on accepting it using the GitHub PR review tools.

   - If rejected by the TSC, a TSC member will communicate this to
     the contributor and the PR will be closed.

   - If approved by the TSC, the TSC chair will forward the README to
     the Zephyr governing board for further review.

#. The Zephyr governing board has two weeks to review and ask questions:

   - If there are no objections, the matter is closed. Approval can be
     accelerated by unanimous approval of the board before the two
     weeks are up.

   - If a governing board member raises an objection that cannot be resolved
     via email, the board will meet to discuss whether to override the
     TSC approval or identify other approaches that can resolve the
     objections.

#. On approval of the Zephyr TSC and governing board, final review of
   the PR may be made to ensure its proper placement in the
   Zephyr Project :ref:`source_tree_v2`, (in the ``ext`` folder), and
   inclusion in the :ref:`zephyr_licensing` document.

.. note::

   External components not under the Apache-2.0 license **cannot** be
   included in a Zephyr OS release without approval of both the Zephyr TSC
   and the Zephyr governing board.

.. _below:

Code component README template
******************************

.. literalinclude:: code_component_README
