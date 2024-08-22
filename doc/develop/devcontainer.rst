.. _devcontainer:

Developing with DevContainers
#############################

What is a devcontainer
**********************

To put it simply, a devcontainer is a disposable, self-contained development
environment, closely coupled to an editor or IDE.

It enables developers to not waste time installing and maintaining an up-to-date
toolchain for a given project.

The initial implementation comes from the visual studio code editor, but is now
an open specification and has support from other editors, e.g. `Jetbrains`_,
`command-line interface`_.

Why use devcontainers
*********************

Here are a few use-cases:

- Work on a computer with no admin rights
- Work on a computer that cannot install the tools (e.g. using babblesim on Windows)
- Author a small patch: faster feedback for ``check_compliance.py``
- Try a `pull request`_ without modifying your local west workspace

How do I use devcontainers
**************************

This document describes two ways of using devcontainers. One is to use a
cloud-based instance on github codespaces, the other is using a local
installation of the VSCode editor.

Github codespaces
=================

.. warning::
   The current CI image is too big for the Github codespace builder. Until that
   is fixed, the devcontainer can unfortunately only be used locally.

The `cloud option`_. It is a paid service, but they have `free minutes`_. At the
time of writing, those were 60 hours of uptime per month, using the smallest
machine option (2core, 8GB ram).

1. Log in to github
#. Using a recent web browser, go to `github codespaces`_
#. Click on the "Create codespace" button
#. Select the ``zephyrproject-rtos/zephyr`` repository
#. Select which codespace you want to instantiate
#. Click on "Create codespace"
#. Walk to starbucks, buy a coffee (it takes a while to load)
#. You are dropped into a vscode instance with the Zephyr toolchain. Depending
   on the chosen codespace, you may have a subset of the default west projects
   and some other tools installed (e.g. The Babblesim codespace has a minimal
   west checkout and the babblesim tools installed and updated).

Local VSCode
============

The local option. Completely free. Expect ~10GB of disk usage.

See the `official installation guide`_ for more details.

1. Install docker, add yourself to the ``docker`` group
#. Install and open `vscode`_
#. Install the `devcontainers extension`_
#. Clone the `zephyr repo`_
#. In VSCode, run the "Dev Containers: `open folder`_ in container" command and
   select the path where the zephyr project is cloned.
#. Select the right devcontainer from the list. E.g. "Bluetooth w/ Babblesim".
#. Brace for bandwidth (and disk) usage..
#. You are dropped into a vscode instance with the Zephyr toolchain. Depending
   on the chosen codespace, you may have a subset of the default west projects
   and some other tools installed (e.g. The Babblesim codespace has a minimal
   west checkout and the babblesim tools installed and updated).

.. _`Jetbrains`: https://plugins.jetbrains.com/plugin/21962-dev-containers
.. _`command-line interface`: https://code.visualstudio.com/docs/devcontainers/devcontainer-cli
.. _`github codespaces`: https://github.com/codespaces
.. _`pull request`: https://code.visualstudio.com/docs/devcontainers/containers#_quick-start-open-a-git-repository-or-github-pr-in-an-isolated-container-volume
.. _`free minutes`: https://docs.github.com/en/billing/managing-billing-for-github-codespaces/about-billing-for-github-codespaces#monthly-included-storage-and-core-hours-for-personal-accounts
.. _`cloud option`: https://docs.github.com/en/codespaces/overview
.. _`devcontainers extension`: https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers
.. _`vscode`: https://code.visualstudio.com/
.. _`zephyr repo`: https://github.com/zephyrproject-rtos/zephyr
.. _`official installation guide`: https://code.visualstudio.com/docs/devcontainers/containers#_installation
.. _`open folder`: https://code.visualstudio.com/docs/devcontainers/containers#_quick-start-open-an-existing-folder-in-a-container
