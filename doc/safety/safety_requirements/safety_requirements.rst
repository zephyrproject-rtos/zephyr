.. _safety_requirements:

Safety Requirements
###################

Introduction
************

The safety committee leads the effort to gather requirements that reflect the **actual** state of
the implementation, following the `route 3s <https://docs.zephyrproject.org/latest/safety/safety_overview.html#general-safety-scope>`_
approach of the project's safety effort. The goal is **NOT** to create new requirements to request
additional features for the project.

The requirements are gathered in the separate repository:
`Requirement repository
<https://github.com/zephyrproject-rtos/reqmgmt>`__

The current rendered version of the Requirement Repository's content can be found at `Zephyr Project Requirements <https://zephyrproject-rtos.github.io/reqmgmt/>`_.

Objections of Requirements Management in the Zephyr Project
***********************************************************
In the development and for documentation of implemented or expected functionality, effective Requirements Management
is essential to ensure that the Zephyr RTOS meets its intended purpose, performs reliably under its constraints, and can be
integrated easily both with hardware platforms, within an embedded software stack and software application layers.

Requirements Management provides a structured approach to capturing, organizing, and tracing the needs and expectations
of stakeholders through a system's lifecycle.
The documentation provided in the `project's docs
<https://docs.zephyrproject.org/latest/index.html>`__
already emphazises implementation details, from kernel functionality, to API configuration options and specific module behaviour. While this is essential for developers,
relying solely on low-level documentation can lead to fragmented understanding, limited traceability, and difficulty scaling or certifying a final system using Zephyr RTOS.

To enhance this understanding and add value for non-coding roles like product owners, software architects, quality management, safety management and assessors,
the Zephyr Project needs to have higher level requirements that describe overall functionality, structure and implementation constraints.


Guideline to Requirements Management
************************************

Below are the guidelines for the requirements repository and the expectations regarding suitable requirement structure and syntax when adding requirements to the repository.

Note: these are the guidelines to create requirements in the requirements repository, which is not written using .rst like the docs published to the Zephyr users.
To learn more about the guidelines to create the docs, please refer to `Documentation Guideline https://docs.zephyrproject.org/latest/contribute/documentation/guidelines.html`__

Scope
=====

The initial scope of the requirements covers the KERNEL functionalities.
Requirements for non-kernel functionality are also welcome, but currently not the focus.

Managing Zephyr Requirements: Structure, Tools, and Grammar
===========================================================

Levels of requirements in the repository
----------------------------------------
The safety working group has evaluated a suitable approach to enhance the existing low level descriptions with requirements.
For this a two level structure of requirements, has been created on top of the existing documentation structure, System Requirements at the highest level to define overarching
capabilities of the Zephyr RTOS as a whole, and Software Requirements to break down the System Requirements' expectations into detailed specifictations for individual components within the RTOS.

System Requirements
  System requirements describe the behaviour of the Zephyr RTOS (= the system here).
  They describe the functionality and constraints of the Zephyr RTOS from a high-level perspective,
  without going into details of the functionality itself.
  The purpose of the system requirements is to get an overview of the currently implemented features
  of the Zephyr RTOS.
  These requirements articulate what the RTOS must achieve from a functional, performance, and interface perspective —
  such as deterministic task scheduling, interrupt handling latency, memory footprint limits,
  and compliance with industry standards.
  System requirements serve as the foundation for design decisions and validation criteria.
  To create these requirements it is beneficial if a person writing these requirements already has some knowledge of the Zephyr RTOS
  Project and/or requirements and constraints that are specific to an RTOS.

Software Requirements
  Beneath the System Requirements layer, Software requirements refine the system-level requirements to a more granular level
  These componeent level requirements break down the system-level expectations into detailed specifications for individual components within the RTOS.
  These include modules such as the scheduler, memory manager and inter-process communication mechanisms.
  Software requirements describe the behavior, interfaces, and constraints of each component, ensuring that they collectively fulfill the system-level goals.
  These requirements define the specific actions the feature shall be able to execute and the
  behavior of the feature so that each requirement can be verified by tests, analysis and/or inspection.
  Also this level of requirements still has to be implementation free in its definitions. Implementation definitions should be in the docs, if not already there, they need to be added
  as another task of this requirements creation effort.

Requirements Management Toolchain
---------------------------------

Requirement Repository:
~~~~~~~~~~~~~~~~~~~~~~~
The `Requirement repository
<https://github.com/zephyrproject-rtos/reqmgmt>`__
represents Zephyr's structured appraoch to requirements management, currently focusing on the creation of requirements within the scope of
its targeted safety certification, but not limited to this scope.
To work with this repository, follow the normal GitHub workflow of branching and pull requests.
The pull requests on this repository are currently reviewed by members of the safety working group and need approval at least from one of the following
roles to get merged: Zephyr Safety Manager, Zephyr Safety Chair or Zephyr Safety Architect.

Requirement Tooling:
~~~~~~~~~~~~~~~~~~~~
For authoring, linking and rendering a browsable .html version of the Zephyr Project's requirements we are using the
tool `strictDoc <https://strictdoc.readthedocs.io/en/stable/>`_
StrictDoc is a lightweight, open-source tool for writing, browsing, and exporting structured requirements.
It supports hierarchical requirements, traceability, and HTML export for easy review.
Requirements are written in StrictDoc's own markdown syntax.
For those that prefer editing in a wisiwig fashion, the .html exports of StrictDoc can also be edited using StrictDoc's local server.

To set up your Toolchain, jump to the `Getting started with Requirements management`_ section.

Verification of Requirements:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
New and changed requirements (what is in the PRs to the Requirements Repository) need to be verified by a review. The review comments can be captured directly in the
comments during the PR review. A requirements review has to consider both technical correctness and the formal need that requirements need be be created following certain criteria.
More details regarding the checklist and the formal expectations can be found in the Safety `Requirements Checklist <https://docs.zephyrproject.org/latest/safety/safety_requirements_checklist.html>`_
In the tests of the Zephyrproject adherence of the implementation to the functionality described in the requirements must be evaluated.

Glossary of Requirements Grammar Elements
-----------------------------------------
All requirements must use the defined grammar:

.. list-table:: Structure for requirements caputured in StrictDoc
   :widths: 20 80
   :header-rows: 1

   * - Property
     - Description

   * - UID
     - A unique ID that enables identification of the requirement. Is assigned automatically by StrictDoc.

   * - STATUS
     - Describing the status of the requirement. Draft, Approved, Retired

   * - TYPE
     - Functional, Non-functional

   * - COMPONENT
     - the component the requirement shall be implemented in

   * - REFERENCE
     - Reference to parent and/or child requirement

   * - TITLE
     - A desciptive title of the requirement. Use short and concise requirement titles.

   * - STATEMENT
     - Description of the requirement

   * - USER_STORY
     - A user story describing the requirement (optional, only add if it adds additional value to the requireents statement)


Requirement UID (Unique identifier) Handling
--------------------------------------------

The tool used to manage requirements, `strictDoc <https://strictdoc.readthedocs.io/en/stable/>`_, is
responsible for handling the Unique Identifier (UID) associated with each requirement. To manage
UIDs, follow these steps:

#. Don't add a requirement UID and UID field for new requirements
#. After completing work on the new requirements execute: ``strictDoc manage auto-uid .``
#. Establish links between the requirements with the new attributed UIDs if needed

After doing this, the requirements are ready and a pull request can be created.
The CI in the PR will check if the requirements UIDs are valid or if there are duplicates in it.
If there are duplicates in the PR, these need to be resolved by rebasing and re-executing
the steps above.


Consistency
-----------

Consistency reagrding language and choice of words shall be maintained accross all requirements.
(See: `Syntax`_)

It also is recommended to take a look at the already existing requirements in the `Requirement repository
<https://github.com/zephyrproject-rtos/reqmgmt>`__ as well as at discussions going on in the PRs to this repository to get a feeling what makes sense in this context.


Pull Request requirement repository
-----------------------------------

* Adhere to the :ref:`contribute_guidelines` of the Zephyr Project.

  * As long as they are applicable to the requirements repository.

* Avoid creating large commits that contain both trivial and non-trivial changes.

* Avoid moving and changing requirements in the same commit.


Characteristics of a good requirement
----------------------------------------

* Unambiguous
* Verifiable (e.g. testable for functional requirements)
* Clear (concise, brief, simple, precise)
* Correct
* Understandable
* Feasible (realistic, possible)
* Independent
* Atomic
* Necessary
* Implementation-free (abstract)

Characteristics of a set of requirements
----------------------------------------

* Complete
* Consistent
* Non redundant

Syntax
======

* Use of a recognized Requirements Syntax is recommended.

  * `EARS <https://alistairmavin.com/ears/>`_ is a good reference. Particularly if you are
    unfamiliar with requirements writing.

  * Other formats are accepted as long as the characteristics of a requirement from above are met.
