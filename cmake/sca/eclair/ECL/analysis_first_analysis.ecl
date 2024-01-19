# This file must be named analyze_<RULESET>.ecl, where <RULESET> is the first
# argument of analyze.sh.
#
# The aim of this file is to define the analysis configuration for <RULESET>.
#
# The essential portions of this file are marked with "# NEEDED":
# they may be adapted of course.

-eval_file=zephyr_common_config.ecl

-doc_begin="Selection of guidelines from
https://docs.zephyrproject.org/latest/guides/coding_guidelines/index.html"
-enable=MC3R1.R8.2
-enable=MC3R1.R10.2
-enable=MC3R1.R10.5
-enable=MC3R1.R10.6
-enable=MC3R1.R11.2
-enable=MC3R1.R12.4
-enable=MC3R1.R13.4
-enable=MC3R1.R16.1
-doc_end
