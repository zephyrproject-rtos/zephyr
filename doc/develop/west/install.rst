.. _west-install:

Installing west
###############

West is written in Python 3 and distributed through `PyPI`_.
Use :file:`pip3` to install or upgrade west:

On Linux::

  pip3 install --user -U west

On Windows and macOS::

  pip3 install -U west

.. note::
   See :ref:`python-pip` for additional clarification on using the
   ``--user`` switch.

Afterwards, you can run ``pip3 show -f west`` for information on where the west
binary and related files were installed.

Once west is installed, you can use it to :ref:`clone the Zephyr repositories
<clone-zephyr>`.

.. _west-struct:

Structure
*********

West's code is distributed via PyPI in a Python package named ``west``.
This distribution includes a launcher executable, which is also named
``west`` (or ``west.exe`` on Windows).

When west is installed, the launcher is placed by :file:`pip3` somewhere in
the user's filesystem (exactly where depends on the operating system, but
should be on the ``PATH`` :ref:`environment variable <env_vars>`). This
launcher is the command-line entry point to running both built-in commands
like ``west init``, ``west update``, along with any extensions discovered
in the workspace.

In addition to its command-line interface, you can also use west's Python
APIs directly. See :ref:`west-apis` for details.

.. _west-shell-completion:

Enabling shell completion
*************************

West currently supports shell completion in the following shells:

* bash
* zsh
* fish

In order to enable shell completion, you will need to obtain the corresponding
completion script and have it sourced.
Using the completion scripts:

.. tabs::

  .. group-tab:: bash

    *One-time setup*:

    .. code-block:: bash

      source <(west completion bash)

    *Permanent setup*:

    .. code-block:: bash

      west completion bash > ~/west-completion.bash; echo "source ~/west-completion.bash" >> ~/.bashrc

  .. group-tab:: zsh

    *One-time setup*:

    .. code-block:: zsh

      source <(west completion zsh)

    *Permanent setup*:

    .. code-block:: zsh

      west completion zsh > "${fpath[1]}/_west"

  .. group-tab:: fish

    *One-time setup*:

    .. code-block:: fish

      west completion fish | source

    *Permanent setup*:

    .. code-block:: fish

      west completion fish > $HOME/.config/fish/completions/west.fish

.. _PyPI:
   https://pypi.org/project/west/
