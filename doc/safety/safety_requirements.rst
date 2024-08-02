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

Guidelines
**********

Below are the guidelines for the requirements repository and the expectations of the safety
committee when adding requirements to the repository.

Scope
=====

The scope of the requirements covers the KERNEL functionalities.

Consistency
===========

Maintain consistency across all requirements. The language and choice of words shall be consistent.
(See: `Syntax`_)

Levels of requirements in the repository
========================================

System Requirements
  System requirements describe the behaviour of the Zephyr RTOS (= the system here).
  They describe the functionality of the Zephyr RTOS from a very high-level perspective,
  without going into details of the functionality itself.
  The purpose of the system requirements is to get an overview of the currently implemented features
  of the Zephyr RTOS.
  In other words a person writing these requirements usually has some knowledge of the Zephyr RTOS
  Project as the requirements are specific to an RTOS.

Software Requirements
  Software requirements refine the system-level requirements  at a more granular level so
  that each requirement can be tested.
  These requirements define the specific actions the feature shall be able to execute and the
  behavior of the feature.

Requirement UID (Unique identifier) Handling
============================================

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

Requirement Types
=================

* Functional
* Non-Functional

Requirement title convention
============================

Use short and succinct requirement titles.

Pull Request requirement repository
===================================

* Adhere to the :ref:`contribute_guidelines` of the Zephyr Project.

  * As long as they are applicable to the requirements repository.

* Avoid creating large commits that contain both trivial and non-trivial changes.

* Avoid moving and changing requirements in the same commit.

Characteristics of a good requirement
=====================================

* Unambiguous
* Verifiable (e.g. testable for functional requirements)
* Clear (concise, succinct, simple, precise)
* Correct
* Understandable
* Feasible (realistic, possible)
* Independent
* Atomic
* Necessary
* Implementation-free (abstract)

Characteristics of a set of requirements
========================================

* Complete
* Consistent
* Non redundant

Syntax
======

* Use of a recognized Requirements Syntax is recommended.

  * `EARS <https://alistairmavin.com/ears/>`_ is a good reference. Particularly if you are
    unfamiliar with requirements writing.

  * Other formats are accepted as long as the characteristics of a requirement from above are met.
