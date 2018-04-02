#!/bin/bash

#
# Copyright (c) 2015 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#


# crawls the source tree to find out the amount of checkpatch issues
# and optionally update scripts/known_checkpatch_issues
# usage: check_known_checkpatch_issues.sh [-u]
# where: -u updates the known_checkpatch_issues db and commits it
#        -q is the quiet mode (don't display the diff on stdout)

exe_name=$(basename $0)

do_checkpatch_bin=${ZEPHYR_BASE}/scripts/checkpatch/do_checkpatch.sh
timestamp_bin=${ZEPHYR_BASE}/scripts/checkpatch/timestamp

declare update=n
declare quiet=n

function usage {
	printf "usage: %s [-u][-q]\n" ${exe_name} >&2
}

function fail {
	usage
	exit -1
}
function verify_needed {
	needed="\
		${do_checkpatch_bin} \
		${timestamp_bin} \
	"
	for i in ${needed}; do
		type $i &>/dev/null
		if [ $? != 0 ]; then
			printf "need '%s' but not found in PATH\n" $i >&2
			exit -1
		fi
	done
}

function get_opts {
	declare -r optstr="quh"
	while getopts ${optstr} opt; do
		case ${opt} in
		u) update=y ;;
		q) quiet=y ;;
		h) usage; exit 0 ;;
		*) fail ;;
		esac
	done
}

verify_needed
get_opts $@

do_checkpatch=${do_checkpatch_bin}
timestamp="${timestamp_bin} -u"
ts=$(${timestamp})
uid=$(id -u)
pid=$$
suffix=${uid}-${pid}-${ts}
checkpatch_results=/tmp/checkpatch.results-${suffix}
known_checkpatch_issues=${ZEPHYR_BASE}/scripts/known_checkpatch_issues
checkpatch_issues=/tmp/checkpatch_issues-${suffix}
git_log_params="\
	--abbrev=8 \
	--abbrev-commit \
"

commit_id_str=$(git log ${git_log_params} HEAD | head -n 1)
echo ${commit_id_str} > ${checkpatch_issues}

${do_checkpatch} ${checkpatch_results} >> ${checkpatch_issues}

diff_file=/tmp/checkpatch.results.diff-${suffix}
diff -u ${known_checkpatch_issues} ${checkpatch_issues} > ${diff_file}

if [ ${quiet} = n ]; then
	cat ${diff_file}
fi

# find all lines that starts with '+' but not '+commit' or '+++ diff'
minuses_err_str=(\
	$(cat ${diff_file} | \
	grep -v -E "^\-\-\-" | grep -v -E "^\-commit " | grep -E "^\-" | \
	awk '{print $1}' | cut -d\- -f 2-) \
)
minuses_num_err=(\
	$(cat ${diff_file} | \
	grep -v -E "^\-\-\-" | grep -v -E "^\-commit " | grep -E "^\-" | \
	awk '{print $2}') \
)
plusses_err_str=(\
	$(cat ${diff_file} | \
	grep -v -E "^\+\+\+" | grep -v -E "^\+commit " | grep -E "^\+" | \
	awk '{print $1}' | cut -d\+ -f 2-) \
)
plusses_num_err=(\
	$(cat ${diff_file} | \
	grep -v -E "^\+\+\+" | grep -v -E "^\+commit " | grep -E "^\+" | \
	awk '{print $2}') \
)

exit_code=0
declare -i num_plusses=${#plusses_num_err[@]}
declare -i num_minuses=${#minuses_num_err[@]}
declare -i test_num=${num_plusses}
while [ ${test_num} -gt 0 ]; do
	test_num+=-1
	match=n
	declare -i i=${num_minuses}
	while [ $i -gt 0 ]; do
		i+=-1
		if [ ${plusses_err_str[${test_num}]} = ${minuses_err_str[$i]} ]; then
			n_minus=${minuses_num_err[$i]}
			n_plus=${plusses_num_err[${test_num}]}
			if [ ${n_plus} -gt ${n_minus} ]; then
				exit_code=1
				break 2
			fi
			match=y
			break 1
		fi
	done

	if [ ${match} = n ]; then
		# there was no match for the plus line, so that is a new error
		exit_code=1
		break 1
	fi
done

if [ ${update} = y ]; then
	msg="known_checkpatch_issues: updating to ${commit_id_str}"
	cp ${checkpatch_issues} ${known_checkpatch_issues}
	git add ${known_checkpatch_issues}
	git commit -m "${msg}"
fi

exit ${exit_code}
