.. _eclair:

ECLAIR support ( Experimental )
###############################

Bugseng `ECLAIR <https://www.bugseng.com/eclair/>`__ is a certified
static analysis tool and platform for software verification.
Applications range from coding rule validation, with a
particular emphasis on the MISRA and BARR-C coding standards, to the
computation of software metrics, to the checking of independence and
freedom from interference among software components, to the automatic
detection of important classes of software errors.

.. important::

   ECLAIR is a commercial tool, and it is not free software.
   You need to have a valid license to use it.

.. warning::

   The Integration of the tool is experimental right now!
   Following known features/use may lead to issues or differen analysis results:

   * Using it without :ref:`west <west>` build would not work
   * Passing extra configuration files or configurations to the build like ``-DEXTRA_CONF_FILE``
     or ``-- -DCONFIG_BAR=y`` may end up in not complete results.
   * Using the sysbuild (``--sysbuild`` or ``west config build.sysbuild True``) in combination with
     ECLAIR may not work as expected. Some side effects would be that some kconfigs couldn't be set
     correctly and the build may fail or generate not complete results.

Running ECLAIR
**************

To run ECLAIR, :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=eclair`` parameter.

.. code-block:: shell

    west build -b mimxrt1064_evk samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=eclair

.. note::
   This will only invoke the ECLAIR analysis with the predefined ruleset ``first_analysis``. If you
   want to use a different ruleset, you need to provide a configuration file. See the next section
   for more information.

Configurations
**************

The configure of the ECLAIR SCA environment can be done via a config file and Kconfig

To invoke a configuration file into the ECLAIR call, you can define the ``DEXTRA_CONF_FILE`` variable,
for example:

.. code-block:: shell

    west build -b mimxrt1064_evk samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=eclair -DEXTRA_CONF_FILE=$(pwd)/cmake/sca/eclair/eclair.config

There are different configuration sets that can be used to run ECLAIR without adapting
the rule set.

The default (if no config file is given) configuration is always ``first_analysis``,
which is a tiny selection of rules to verify that everything is correctly working.

Zephyr is a large and complex project, so the configuration sets are split the
Zephyr's guidelines selection
(taken from https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html)
in five sets to make it more digestible to use on a private machine:

* first_analysis (default): a tiny selection of the projects coding guidelines to verify that
  everything is correctly working.

* STU: Selection of the projects coding guidelines, which can be verified by analysing the single
  translation units independently.

* STU_heavy: Selection of complex STU project coding guidelines that require a significant amount
  of time.

* WP: All whole program project coding guidelines ("system" in MISRA's parlance).

* std_lib: Project coding guidelines about the C Standard Library.

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_RULESET_FIRST_ANALYSIS`
* :kconfig:option:`CONFIG_ECLAIR_RULESET_STU`
* :kconfig:option:`CONFIG_ECLAIR_RULESET_STU_HEAVY`
* :kconfig:option:`CONFIG_ECLAIR_RULESET_WP`
* :kconfig:option:`CONFIG_ECLAIR_RULESET_STD_LIB`

User defined ruleset
====================

If you want to use your own defined ruleset instead of the predefined zephyr coding guidelines
rulesets. You can do so by setting :code:`CONFIG_ECLAIR_RULESET_USER=y`.
Created your own rulset file for ECLAIR with the following naming format:
``analysis_<RULESET>.ecl``. After creating the file define the name of the ruleset for ECLAIR
with :code:`CONFIG_ECLAIR_USER_RULESET_NAME`.
If the ruleset file is not in the application source directory, you can define the path to the
ruleset file with :code:`CONFIG_ECLAIR_USER_RULESET_PATH`. This configuration takes relative paths
and absolute paths.

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_RULESET_USER`
* :kconfig:option:`CONFIG_ECLAIR_USER_RULESET_NAME`
* :kconfig:option:`CONFIG_ECLAIR_USER_RULESET_PATH`

Generate additional report formats
**********************************

ECLAIR can generate additional report formats (e.g. DOC, ODT, XLSX) and
different variants of repots in addition to the
default ecd file. Following additional reports and report formats can be generated:

* Metrics in spreadsheet format.

* Findings in spreadsheet format.

* Findings in SARIF format.

* Summary report in plain textual format.

* Summary report in DOC format.

* Summary report in ODT format.

* Detailed reports in txt format.

* Detailed report in DOC format.

* Detailed report in ODT format.

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_METRICS_TAB`
* :kconfig:option:`CONFIG_ECLAIR_REPORTS_TAB`
* :kconfig:option:`CONFIG_ECLAIR_REPORTS_SARIF`
* :kconfig:option:`CONFIG_ECLAIR_SUMMARY_TXT`
* :kconfig:option:`CONFIG_ECLAIR_SUMMARY_DOC`
* :kconfig:option:`CONFIG_ECLAIR_SUMMARY_ODT`
* :kconfig:option:`CONFIG_ECLAIR_FULL_TXT`
* :kconfig:option:`CONFIG_ECLAIR_FULL_DOC`
* :kconfig:option:`CONFIG_ECLAIR_FULL_ODT`

Detail level of full reports
============================

The detail level of the txt and doc full reports can also be adapted by a configuration.
In this case the following configurations are avilable:

* Show all areas

* Show only the first area

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_FULL_DOC_ALL_AREAS`
* :kconfig:option:`CONFIG_ECLAIR_FULL_DOC_FIRST_AREA`
* :kconfig:option:`CONFIG_ECLAIR_FULL_TXT_ALL_AREAS`
* :kconfig:option:`CONFIG_ECLAIR_FULL_TXT_FIRST_AREA`
