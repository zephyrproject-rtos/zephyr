#!/bin/bash

#
# Copyright (c) 2015 Wind River Systems, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

exe_name=$(basename $0)

# check the last n patches patches from the current branch for errors
# usage: maintainer-checkpatch.bash [(-n <num commits>) | (-c <commit>)] [-s]
# where: -n <num commits> selects the last n commits (default: 1)
#        -c <commit> selects the "since" commit
#        -s asks for a summary instead of details
#
# -c and -n are mutually exclusive

checkpatch_switches="\
	--patch \
	--no-tree \
	--show-types \
	--max-line-length=100 \
"
ignore_list=BRACES,PRINTK_WITHOUT_KERN_LEVEL,SPLIT_STRING,FILE_PATH_CHANGES,GERRIT_CHANGE_ID

timestamp_bin=${ZEPHYR_BASE}/scripts/timestamp
timestamp="${timestamp_bin} -u"
checkpatch_bin=${ZEPHYR_BASE}/scripts/checkpatch.pl
checkpatch="${checkpatch_bin} ${checkpatch_switches} --ignore ${ignore_list}"

ts=$(${timestamp})
outdir=/tmp/${exe_name}-${ts}

declare num_commits=1
declare summary=n
declare since_commit=""

function usage {
	printf "usage: %s [(-n <num commits>) | (-c <commit>)] [-s]\n" $exe_name >&2
}

function fail {
	usage
	exit -1
}

function format_patch_fail {
	printf "'git format-patch' failed\n"
	exit -1
}

function verify_needed {
	needed="\
		${timestamp_bin} \
		${checkpatch_bin} \
	"
	for i in $needed; do
		type $i &>/dev/null
		if [ $? != 0 ]; then
			printf "need '%s' but not found in PATH\n" $i >&2
			exit -1
		fi
	done
}

function get_opts {
	declare -r optstr="n:c:sh"
	while getopts ${optstr} opt; do
		case ${opt} in
			n) num_commits=${OPTARG} ;;
			c) since_commit=${OPTARG} ;;
			s) summary=y ;;
			h) usage; exit 0 ;;
			*) fail ;;
		esac
	done

	if [ ${num_commits} != 1 -a "x${since_commit}" != x ]; then
		fail
	fi
}

verify_needed
get_opts $@

if [ x${since_commit} != x ]; then
	since=${since_commit}
else
	since=HEAD~${num_commits}
fi
git format-patch ${since} -o ${outdir} 2>/dev/null >/dev/null
[ $? = 0 ] || format_patch_fail

for i in $(ls ${outdir}/*); do
	printf "\n$(basename ${i})\n"
	if [ ${summary} = y ]; then
		${checkpatch} $i | grep "total:"
	else
		${checkpatch} $i
	fi
done

rm -rf ${outdir}
