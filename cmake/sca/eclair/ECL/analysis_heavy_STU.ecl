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
-enable=MC3R1.D4.6
-enable=MC3R1.D4.9
-enable=MC3R1.R12.1
-enable=MC3R1.R13.3
-enable=MC3R1.R2.6
-enable=MC3R1.R10.1
-enable=MC3R1.R10.3
-enable=MC3R1.R10.4
-enable=MC3R1.R14.4
-enable=MC3R1.R20.7
-doc_end
