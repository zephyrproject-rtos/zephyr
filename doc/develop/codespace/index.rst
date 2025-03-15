.. _github_codespace:

Developing with GitHub Codespace
################################

`GitHub codespace <https://github.com/features/codespaces>`_ is a development environment that's
hosted in the cloud. By leveraging _free*_ hours provided by GitHub, it allows users to get started
or get something done with Zephyr on any device that has a modern web browser and has access to the
internet.

   .. note::

      Read more about GitHub codespace billing `here <https://docs.github.com/en/billing/managing-billing-for-github-codespaces/about-billing-for-github-codespaces>`_.

When creating a codespace from a Zephyr repository, a custom devcontainer image will be built
on the fly following the
`Getting Started Guide <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>`_,
this process is expected to take quite a few minutes.

Once everything is installed, you will be presented with a Visual Studio Code (VSCode) web interface, with some
extensions configured and is ready to build firmware.

It is also possible to connect to GitHub codespace from VSCode. This might be advantageous
for users on company computers that restricts the installation of unapproved software. For example, this
setup allows users to develop with `Renode <https://renode.io/>`_, or compile applications for
:ref:`native_sim<native_sim>` on a Mac.


.. _limitations:

Limitations
***********

* USB access is not supported in GitHub codespace, and therefore any operations
  (i.e. firmware programming and debugging) that requires USB will not work.

.. _additional_configs:

Additional Configurations
*************************

This section documents the configuration of some VSCode extensions that could be
handy in developing with Zephyr.

.. _config_checkpatch:

Checkpatch
==========

Zephyr CI uses the checkpatch script to validate if the patch submitted conforms to our
coding guidelines, user can run the script by executing it in the terminal. Alternatively,
we can make use of VSCode and view the violation directly in the editor. To do that, download
the extension `here <https://marketplace.visualstudio.com/items?itemName=idanp.checkpatch>`_,
then apply the following configurations (copied from
`here <https://github.com/zephyrproject-rtos/zephyr/blob/main/.checkpatch.conf>`_)
to the setting.

.. code-block:: json

   {
      "checkpatch.checkpatchPath": "/home/vscode/zephyrproject/zephyr/scripts/checkpatch.pl",
      "checkpatch.checkpatchArgs": [
      "--emacs",
      "--summary-file",
      "--show-types",
      "--max-line-length=100",
      "--min-conf-desc-length=1",
      "--typedefsfile=/home/vscode/zephyrproject/zephyr/scripts/checkpatch/typedefsfile",
      "--ignore BRACES",
      "--ignore PRINTK_WITHOUT_KERN_LEVEL",
      "--ignore SPLIT_STRING",
      "--ignore VOLATILE",
      "--ignore CONFIG_EXPERIMENTAL",
      "--ignore PREFER_KERNEL_TYPES",
      "--ignore PREFER_SECTION",
      "--ignore AVOID_EXTERNS",
      "--ignore NETWORKING_BLOCK_COMMENT_STYLE",
      "--ignore DATE_TIME",
      "--ignore MINMAX",
      "--ignore CONST_STRUCT",
      "--ignore FILE_PATH_CHANGES",
      "--ignore SPDX_LICENSE_TAG",
      "--ignore C99_COMMENT_TOLERANCE",
      "--ignore REPEATED_WORD",
      "--ignore UNDOCUMENTED_DT_STRING",
      "--ignore DT_SPLIT_BINDING_PATCH",
      "--ignore DT_SCHEMA_BINDING_PATCH",
      "--ignore TRAILING_SEMICOLON",
      "--ignore COMPLEX_MACRO",
      "--ignore MULTISTATEMENT_MACRO_USE_DO_WHILE",
      "--ignore ENOSYS",
      "--ignore IS_ENABLED_CONFIG",
      "--ignore EXPORT_SYMBOL"
      ]
   }
