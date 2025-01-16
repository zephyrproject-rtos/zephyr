.. _development_model:

Project and Governance
#######################


.. toctree::
   :maxdepth: 1

   tsc
   project_roles.rst
   working_groups
   release_process
   proposals
   code_flow
   dev_env_and_tools
   issues
   communication
   documentation



The Zephyr project defines a development process workflow using GitHub
**Issues** to track feature, enhancement, and bug reports together with GitHub
**Pull Requests** (PRs) for submitting and reviewing changes.  Zephyr
community members work together to review these Issues and PRs, managing
feature enhancements and quality improvements of Zephyr through its regular
releases, as outlined in the
`program management overview <https://wiki.zephyrproject.org/Program-Management>`_.

We can only manage the volume of Issues and PRs, by requiring timely reviews,
feedback, and responses from the community and contributors, both for initial
submissions and for followup questions and clarifications.  Read about the
project's :ref:`development processes and tools <dev-environment-and-tools>`
and specifics about :ref:`review timelines <review_time>` to learn about the
project's goals and guidelines for our active developer community.

:ref:`project_roles` describes in detail the Zephyr project roles and associated permissions
with respect to the development process workflow.


Terminology
***********

- mainline: The main tree where the core functionality and core features are
  being developed.
- subsystem/feature branch: is a branch within the same repository. In our case,
  we will use the term branch also when referencing branches not in the same
  repository, which are a copy of a repository sharing the same history.
- upstream: A parent branch the source code is based on. This is the branch you
  pull from and push to, basically your upstream.
- LTS: Long Term Support
