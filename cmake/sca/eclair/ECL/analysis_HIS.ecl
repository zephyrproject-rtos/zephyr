# This file must be named analyze_<RULESET>.ecl, where <RULESET> is the first
# argument of analyze.sh.
#
# The aim of this file is to define the analysis configuration for <RULESET>.
#
# The essential portions of this file are marked with "# NEEDED":
# they may be adapted of course.

# Enable the desired metrics.
-enable=MET.HIS.COMF
-enable=MET.HIS.PATH
-enable=MET.HIS.GOTO
-enable=MET.HIS.v_G
-enable=MET.HIS.CALLING
-enable=MET.HIS.CALLS
-enable=MET.HIS.PARAM
-enable=MET.HIS.STMT
-enable=MET.HIS.LEVEL
-enable=MET.HIS.RETURN
-enable=MET.HIS.VOCF
-enable=MET.HIS.ap_cg_cycle
