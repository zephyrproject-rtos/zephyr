.. _eclair:

ECLAIR support
##############

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

Running ECLAIR
**************

To run ECLAIR, :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=eclair`` parameter.

.. code-block:: shell

    west build -b mimxrt1064_evk samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=eclair

Configurations
**************

The configure of the ECLAIR SCA environment can be done via a config file and Kconfig

To invoke a configuration file into the ECLAIR call, you can define the ``DECLAIR_CONFIG`` variable,
for example:

.. code-block:: shell

    west build -b mimxrt1064_evk samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=eclair -DECLAIR_CONFIG=$(pwd)/cmake/sca/eclair/eclair.config

There are different configuration sets that can be used to run ECLAIR without adapting
the rule set.

The default (if no config file is given) configuration is always ``first_analysis``,
which is a tiny selection of rules to verify that everything is correctly working.

Zephyr is a large and complex project, so the configuration sets are split the
Zephyr's guidelines selection
(taken from https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html)
in five sets to make it more digestible to use on a private machine:

* first_analysis (default): a tiny selection of rules to verify that everything
  is correctly working.

* STU: selection of MISRA guidelines that can be verified by analysing the single
  translation units independently.

* STU_heavy: selection of complex STU guidelines that require a significant amount
  of time.

* WP: all whole program guidelines of your selection ("system" in MISRA's parlance).

* std_lib: guidelines about the C Standard Library.

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_RULES_SET_first_analysis`
* :kconfig:option:`CONFIG_ECLAIR_RULES_SET_STU`
* :kconfig:option:`CONFIG_ECLAIR_RULES_SET_STU_heavy`
* :kconfig:option:`CONFIG_ECLAIR_RULES_SET_WP`
* :kconfig:option:`CONFIG_ECLAIR_RULES_SET_std_lib`

Generate additional report formats
**********************************

ECLAIR can generate additional report formats (e.g. DOC, ODT, XLSX) and
different variants of repots in addition to the
default ecd file. Following additional reports and report formats can be generated:

* Metrics in spreadsheet format.

* Findings in spreadsheet format.

* Summary report in plain textual format.

* Summary report in DOC format.

* Summary report in ODT format.

* Detailed reports in txt format.

* Detailed report in DOC format.

* Detailed report in ODT format.

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_metrics_tab`
* :kconfig:option:`CONFIG_ECLAIR_reports_tab`
* :kconfig:option:`CONFIG_ECLAIR_summary_txt`
* :kconfig:option:`CONFIG_ECLAIR_summary_doc`
* :kconfig:option:`CONFIG_ECLAIR_summary_odt`
* :kconfig:option:`CONFIG_ECLAIR_full_txt`
* :kconfig:option:`CONFIG_ECLAIR_full_doc`
* :kconfig:option:`CONFIG_ECLAIR_full_odt`

Detail level of full reports
============================

The detail level of the txt and doc full reports can also be be adapted by a configuration.
In this case the following configurations are avilable:

* Show all areas

* Show only the first area

Related configuration options:

* :kconfig:option:`CONFIG_ECLAIR_full_doc_areas_AREAS`
* :kconfig:option:`CONFIG_ECLAIR_full_doc_areas_FIRST_AREA`
* :kconfig:option:`CONFIG_ECLAIR_full_txt_areas_AREAS`
* :kconfig:option:`CONFIG_ECLAIR_full_txt_areas_FIRST_AREA`
