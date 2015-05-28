#!/bin/bash

#
# Copyright (c) 2015 Wind River Systems, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1) Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2) Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3) Neither the name of Wind River Systems nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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
ignore_list=BRACES,PRINTK_WITHOUT_KERN_LEVEL,SPLIT_STRING,FILE_PATH_CHANGES

timestamp_bin=${TIMO_BASE}/scripts/timestamp
timestamp="${timestamp_bin} -u"
checkpatch_bin=${TIMO_BASE}/scripts/checkpatch.pl
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
