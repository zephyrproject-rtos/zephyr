---
name: External Source Code
about: Submit a proposal to integrate external source code
title: ''
labels: TSC
assignees: ''

---

## Origin

Name of project hosting the original open source code
Provide a link to the source

## Purpose

Brief description of what this software does

## Mode of integration

Describe whether you'd like to integrate this external component in the main tree
or as a module, and why. If the mode of integration is a module, suggest a
repository name for the module

## Maintainership

List the person(s) that will be maintaining the integration of this external code
for the foreseeable future. Please use GitHub IDs to identify them. You can
choose to identify a single maintainer only or add collaborators as well

## Pull Request

Pull request (if any) with the actual implementation of the integration, be it
in the main tree or as a module (pointing to your own fork for now). Make sure
the PR is correctly labeled as "DNM"

## Description

Long description that will help reviewers discuss suitability of the
component to solve the problem at hand (there may be a better options
available.)

What is its primary functionality (e.g., SQLLite is a lightweight
database)?

What problem are you trying to solve? (e.g., a state store is
required to maintain ...)

Why is this the right component to solve it (e.g., SQLite is small,
easy to use, and has a very liberal license.)

## Security

Does this component include any cryptographic functionality?
If so, please describe the cryptographic algorithms and protocols used.

How does this component handle security vulnerabilities and updates?
Are there any known vulnerabilities in this component? If so, please
provide details and references to any CVEs or security advisories.

## Dependencies

What other components does this package depend on?

Will the Zephyr project have a direct dependency on the component, or
will it be included via an abstraction layer with this component as a
replaceable implementation?

## Revision

Version or SHA you would like to integrate initially

## License

Please use an SPDX identifier (https://spdx.org/licenses/), such as
``BSD-3-Clause``

