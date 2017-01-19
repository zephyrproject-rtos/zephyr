#!/usr/bin/env python2

#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

# 2 possible algos: at each new timestamp, update run/queue/sleep time for all threads vs update
# only the right process. 2nd algo is still easy
# because we have duration of this state.
# this presents run, queued, sleep time of processes sorted by run time. REL time starts when
# process was created while ABS time if from beginning. Therefore elapsed time between start and
# process creation is added as sleep time

import os, sys, re
from collections import defaultdict
from operator import itemgetter
from optparse import OptionParser

class FilterTrace:
    def __init__(self):
        self.compiled_regexp = []
        self.pid_names = {}
        self.run_time = defaultdict(int) # total run-time per tid
        self.queued_time = defaultdict(int)
        self.sleep_time = defaultdict(int)
        self.ctxswitch_run_times0 = defaultdict(int)
        self.ctxswitch_run_times1 = defaultdict(int)
        self.timestamp_current = 0
        self.convert = 1.0 / 1000

        self.timestamp_start = 0 # must be >=
        self.timestamp_end = 0 # must be <=, but if > to last context switch, will go until
			       # Last timestamp only
        self.runqueue_threads_only = False # see options help

        REGEXP_GLOSSARY = [
            # backup rule
            ['^.*psel.*$', None],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+)\s+(?P<duration>\d+).*$', self.callback_context_switch],
            ['^Last timestamp:\s*(?P<last_timestamp>\d+).*$', self.callback_end],
            ['^.*$', self.callback_no_rule]
        ]

        # Compile all regexp
        for regexp_string, regexp_rules in REGEXP_GLOSSARY:
            self.compiled_regexp.append([re.compile(regexp_string), regexp_rules])

    def filter_line(self, line):
        # Parse all regexps
        for regexp, regexp_rules in self.compiled_regexp:
            # Test if regexp matches
            res = regexp.search(line)
            # if regexp match
            if res:
               # Execute regexp_rules
               if regexp_rules:
                  regexp_rules(res, line)
               # Only one regexp per line
               break

    def callback_context_switch(self, res, line):
        timestamp_current = int(res.group('timestamp'))

        # end boundary
        if (timestamp_current >= self.timestamp_end):
            return

        duration = int(res.group('duration'))
	# wait to reach timestamp_start before processing
        if (timestamp_current < self.timestamp_start):
	    # thread will keep this state until start boundary so count this time
            if ((timestamp_current + duration) > self.timestamp_start):
		# remove time before start boundary
                duration = timestamp_current + duration - self.timestamp_start
                timestamp_current = self.timestamp_start
            else:
                return

        # if process goes beyond end, remove time after end
        if (duration > (self.timestamp_end - timestamp_current)):
            duration = self.timestamp_end - timestamp_current

        tid = int(res.group('tid'))
        if (res.group('state') == 'R'):
            self.run_time[tid] += duration
            if (int(res.group('cpu_id')) == 0):
                self.ctxswitch_run_times0[tid] += 1
            else:
                self.ctxswitch_run_times1[tid] += 1
        elif (res.group('state') == 'Q'):
            self.queued_time[tid] += duration
        elif ((res.group('state') == 'S') or (res.group('state') == 'X')):
            self.sleep_time[tid] += duration

        self.pid_names[tid] = res.group('task_name').rstrip(' ')


    def callback_no_rule(self, res, line):
        return

    def callback_end(self, res, line):
        max_sum = 0

        # we capture the process that started the first in order to get the sum for the % and set
	# START time at 0
        for tid in self.pid_names:
            sum = self.run_time[tid] + self.sleep_time[tid] + self.queued_time[tid]
            if sum > max_sum:
                max_sum = sum

        print("%-16s:%-10s %-10s %-10s %-10s %-10s %-10s %-7s" \
	      " %-7s %-7s %-7s %-7s %-7s %7s %7s %8s") % (
			"PID_NAME", "TID", "RUNNING", "SLEEPING", "QUEUED", "SUM", "START",
			"%R_ABS", "%S_ABS", "%Q_ABS", "%R_REL", "%S_REL", "%Q_REL", "N_RUN_0",
			"N_RUN_1", "N_RUN")

        items = self.run_time.items()
        items.sort(key = itemgetter(1), reverse=True) # inverse key and value to sort by value
        for tid, v in items:
             if ((not self.runqueue_threads_only) or
		 ((self.run_time[tid] != 0) or (self.queued_time[tid] != 0))):
                rel_sum = self.run_time[tid] + self.sleep_time[tid] + self.queued_time[tid]
                print("%-16s:%-10d %-10d %-10d %-10d %-10d %-10d %7.3f %7.3f %7.3f %7.3f" \
		      " %7.3f %7.3f %7d %7d %8d") % ( \
                         self.pid_names[tid],
                         tid,
                         self.run_time[tid],
                         self.sleep_time[tid],
                         self.queued_time[tid],
                         rel_sum, # script execution time - start time of process
                         max_sum - rel_sum, # start time of process
                         self.run_time[tid] * 100.0 / max_sum, # ABS R time
			 # ABS sleep time, start time is counted as sleep time
                         (self.sleep_time[tid] + max_sum - rel_sum) * 100.0 / max_sum,
                         self.queued_time[tid] * 100.0 / max_sum, # ABS Q time
                         self.run_time[tid] * 100.0 / rel_sum, # REL R time
                         self.sleep_time[tid] * 100.0 / rel_sum, # REL S time
                         self.queued_time[tid] * 100.0 / rel_sum, # REL Q time
                         self.ctxswitch_run_times0[tid],
                         self.ctxswitch_run_times1[tid],
                         self.ctxswitch_run_times0[tid] + self.ctxswitch_run_times1[tid]
                       )

global filterMaster
filterMaster = FilterTrace()

def Main(argv):
    usage = "Usage: %prog FILENAME [options]"
    parser = OptionParser(usage = usage)
    parser.add_option("-s", "--start", action="store", type="int", dest="start", default=0,
		      help="Start time of analysis in relative ms")
    parser.add_option("-e", "--end", action="store", type="int", dest="end", default=1000000000,
		      help="End time of analysis in relative ms")
    parser.add_option("-r", "--runqueue_threads_only", action="store_true",
		      dest="runqueue_threads_only", default=False,
		      help="Display only threads with some running time")
    (options, args) = parser.parse_args()
    filterMaster.timestamp_start = int(options.start / filterMaster.convert)
    filterMaster.timestamp_end = int(options.end / filterMaster.convert)
    filterMaster.runqueue_threads_only = options.runqueue_threads_only

    if len(args) != 1:
        print "Usage: type python <program>.py -h"
        return

    file = open(args[0], 'r')
    string = file.readline()

    # search for regexp
    while (string ):
        filterMaster.filter_line(string)
        string = file.readline()

if __name__=="__main__":
    Main(sys.argv[1:])
