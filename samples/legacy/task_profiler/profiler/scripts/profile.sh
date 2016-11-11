#!/bin/bash

#
# Copyright (c) 2016 Intel Corporation.
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

function usage() {
	echo "Usage: ./profile <options> [profiler log] [ELF file]"
	echo " Post-process profiler output. matplotlib is required for graphical output"
	echo ""
	echo "   Type:"
	echo "      default       Text output"
	echo "      -r|--run      Running task view (running context over time)"
	echo "      -t|--total    Display total CPU load per context"
	echo "      -s|--slice    Timeslice graph (Context load average per period)"
	echo ""
	echo "   Options:"
	echo "      -c|--clock [ticks_per_sec]    HW cycle period (system timer)"
	echo "     Only applicable for graphical output:"
	echo "      -b|--begin [time_ms]          Start timestamp (ms since boot)"
	echo "      -e|--end [time_ms]            End time (ms since boot)"
	echo "     Only applicable for slice:"
	echo "      -p|--period [period_ms]       Timeslice average period"\
		"(for timeslice graph only)"
}

TEXT=0
RUN=1
TOTAL=2
SLICE=3

TYPE=$TEXT

PERIOD=500

TS_START=0
TS_END=0

NO_PARAM=0

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ] ; do SOURCE="$(readlink "$SOURCE")"; done
TOP_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

while [ -n "$1" ]; do
	case "$1" in
		-h|--help)
			usage
			exit
		;;

		-r|--run) TYPE=$RUN ;;
		-t|--total) TYPE=$TOTAL ;;
		-s|--slice) TYPE=$SLICE ;;
		-b|--begin) shift; TS_START=$1 ;;
		-e|--end) shift; TS_END=$1 ;;
		-p|--period) shift; PERIOD=$1 ;;
		-c|--clock) shift; TICKS_PER_SEC="-c "$1 ;;
		*)
			case "$NO_PARAM" in
				0) LOG_FILE=$1 ;;
				1) ELF_FILE=$1 ;;
			esac
			NO_PARAM=$(($NO_PARAM+1))
		;;

	esac
	shift
done

if [ -z $LOG_FILE ] || [ -z $ELF_FILE ]; then
	usage
	exit
fi

if [ "$TYPE" == "$TEXT" ]; then
	$TOP_DIR/profile_kernel.py $TICKS_PER_SEC $LOG_FILE $ELF_FILE
	exit
else
	$TOP_DIR/profile_kernel.py --ftrace $TICKS_PER_SEC $LOG_FILE $ELF_FILE > tmp1.txt
fi

$TOP_DIR/contextswitch_parse.py tmp1.txt > tmp2.txt

if [ "$TS_END" -gt "$TS_START" ] ; then
	ARG1="-s "$TS_START" -e "$TS_END
else
	ARG1=""
fi

if [ "$TYPE" == "$RUN" ]; then
	$TOP_DIR/contextswitch_run.py $ARG1 --nocoreload tmp2.txt
elif [ "$TYPE" == "$TOTAL" ]; then
	$TOP_DIR/contextswitch_totals.py $ARG1 tmp2.txt
elif [ "$TYPE" == "$SLICE" ]; then
	ARG2="-t "$PERIOD
	$TOP_DIR/contextswitch_timeslice.py $ARG1 $ARG2 tmp2.txt
fi

rm -rf tmp1.txt tmp2.txt
