# This file must be named analyze_<RULESET>.ecl, where <RULESET> is the first
# argument of analyze.sh.
#
# The aim of this file is to define the analysis configuration for <RULESET>.
#
# The essential portions of this file are marked with "# NEEDED":
# they may be adapted of course.

-eval_file=zephyr_common_config.ecl

-doc_begin="Selection of guidelines from
https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html"
-enable=MC3A2.R2.3
-enable=MC3A2.R2.1
-enable=MC3A2.R5.9
-enable=MC3A2.R5.1
-enable=MC3A2.R5.6
-enable=MC3A2.R5.7
-enable=MC3A2.R5.8
-enable=MC3A2.R8.9
-enable=MC3A2.R8.3
-enable=MC3A2.R8.5
-enable=MC3A2.R8.6
-enable=MC3A2.R22.4
-enable=MC3A2.R22.3
-enable=MC3A2.D1.1
-enable=MC3A2.D3.1
-enable=MC3A2.D4.1
-enable=MC3A2.D4.10
-enable=MC3A2.R17.2
-enable=MC3A2.R17.7
-doc_end
