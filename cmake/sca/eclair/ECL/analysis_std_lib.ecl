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
-enable=MC3R1.R21.1
-enable=MC3R1.R21.12
-enable=MC3R1.R21.14
-enable=MC3R1.R21.15
-enable=MC3R1.R21.16
-enable=MC3R1.R21.2
-enable=MC3R1.R21.3
-enable=MC3R1.R21.4
-enable=MC3R1.R21.6
-enable=MC3R1.R21.7
-enable=MC3R1.R21.9
-enable=MC3R1.R22.1
-enable=MC3R1.R22.10
-enable=MC3R1.R22.7
-enable=MC3R1.R22.8
-enable=MC3R1.R22.9
-doc_end
