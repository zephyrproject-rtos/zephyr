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

.. _west-shell-completion:

Enabling shell completion
*************************

West currently supports shell completion in the following combinations of
platform and shell:

* Linux: bash
* macOS: bash
* Windows: not available

In order to enable shell completion, you will need to obtain the corresponding
completion script and have it sourced every time you enter a new shell session.

To obtain the completion script you can use the ``west completion`` command::

   cd /path/to/zephyr/
   west completion bash > ~/west-completion.bash

.. note::

   Remember to update your local copy of the completion script using ``west
   completion`` when you update Zephyr.

Next, you need to import :file:`west-completion.bash` into your bash shell.

On Linux, you have the following options:

* Copy :file:`west-completion.bash` to :file:`/etc/bash_completion.d/`.
* Copy :file:`west-completion.bash` to
  :file:`/usr/share/bash-completion/completions/`.
* Copy :file:`west-completion.bash` to a local folder and source it from your
  :file:`~/.bashrc`.

On macOS, you have the following options:

* Copy :file:`west-completion.bash` to a local folder and source it from your
  :file:`~/.bash_profile`
* Install the ``bash-completion`` package with ``brew``::

    brew install bash-completion

  then source the main bash completion script in your :file:`~/.bash_profile`::

    source /usr/local/etc/profile.d/bash_completion.sh

  and finally copy :file:`west-completion.bash` to
  :file:`/usr/local/etc/bash_completion.d/`.

.. _PyPI:
   https://pypi.org/project/west/
