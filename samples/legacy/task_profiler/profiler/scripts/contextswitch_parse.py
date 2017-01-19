#!/usr/bin/env python2

#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import os, sys, re
from collections import defaultdict
from optparse import OptionParser

class FilterTrace:
    def __init__(self):
        # 2 pass parsing
        self.compiled_regexp = [] # regexp for 1st pass
        self.compiled_regexp2 = [] # regexp for 2nd pass
        self.final_text = [] # contains all lines before they are written to file
        self.previous_line = {} # index in self.final_text of previous status change per
				# tid + timestamp. So we can compute easily duration of
				# state for 1 tid

        # thread information
        self.tid_names = {} # process name per tid
        self.tid_states = defaultdict(int) # state per tid 0:RUNNING 1:QUEUED 2:SLEEPING
					   # 4:QUEUED or SLEEPING

        # error checking per tid
        self.t1_errors = defaultdict(int)
        self.t2_errors = defaultdict(int)
        self.t3_errors = defaultdict(int)

        # context switch info
        self.prev_tids = defaultdict(int) # previous tid per cpu when tracing new switch

        # Time (relative and absolute)
        self.last_timestamp_found = 0
        self.last_timestamp = 0
        self.reference_timestamp = 0 # time offset used in target script
        self.convert = 1 / 1000.0 #1.0 / 1 * 1000 # 32kHz -> ms conversion

        REGEXP_GLOSSARY = [

            ## irqs
            # <idle>-0     [001]   294.876831: irq_handler_entry: irq=30 name=arch_timer
            ['^.*\[(?P<cpu_id>\d+)\]\s+(?P<foo>[\w+\.]+)\s+(?P<timestamp>\d+\.\d+):\s+' \
	     'irq_handler_entry:\s+irq=(?P<irq_id>\d+)\s+name=(?P<irq_name>[\w \.-_]+).*$',
		self.callback_irq_start_ftrace
	    ],
            ['^.*\[(?P<cpu_id>\d+)\]\s+(?P<timestamp>\d+\.\d+):\s+irq_handler_entry:' \
	     '\s+irq=(?P<irq_id>\d+)\s+name=(?P<irq_name>[\w \.-_]+).*$',
		self.callback_irq_start_ftrace
	    ],
            # kworker/u:0-5     [000]   294.876862: irq_handler_exit: irq=30 ret=handled
            ['^.*\[(?P<cpu_id>\d+)\]\s+(?P<foo>[\w+\.]+)\s+(?P<timestamp>\d+\.\d+):' \
	     '\s+irq_handler_exit:\s+irq=(?P<irq_id>\d+)\s+ret=.*$',
	     self.callback_irq_stop_ftrace
	    ],
            ['^.*\[(?P<cpu_id>\d+)\]\s+(?P<timestamp>\d+\.\d+):\s+irq_handler_exit:\s+' \
	     'irq=(?P<irq_id>\d+)\s+ret=.*$',
	     self.callback_irq_stop_ftrace
	    ],

            ## scheduler
            #sh-2505  [000]   271.147491: sched_switch: prev_comm=sh prev_pid=2505 prev_prio=120
	    # prev_state=S ==> next_comm=swapper next_pid=0 next_prio=120
            ['^.*\[(?P<cpu_id>\d+)\]\s+(?P<foo>[\w+\.]+)\s+(?P<timestamp>\d+\.\d+):\s+' \
	     'sched_switch:\s+.*==>\s+next_comm=(?P<next_task_name>[\w \.#-_/]+)\s+' \
	     'next_pid=(?P<next_tid>\d+)\s+next_prio.*$',
	     self.callback_context_switch_ftrace
	    ],
            ['^.*\[(?P<cpu_id>\d+)\]\s+(?P<timestamp>\d+\.\d+):\s+sched_switch:\s+.*==>\s+' \
	     'next_comm=(?P<next_task_name>[\w \.#-_/]+)\s+next_pid=(?P<next_tid>\d+)\s+' \
	     'next_prio.*$',
	     self.callback_context_switch_ftrace
	    ],
            ['^TIMESTAMP.*$', None],
            ['^.*$', self.callback_no_rule]
        ]

        REGEXP_GLOSSARY2 = [
            ['^Ref timestamp:\s*(?P<ref_time>\d+)$', self.callback_reference_timestamp],
            ['^.*psel.*$', None],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+).*$',
	     self.callback_context_switch2
	    ],
            ['^Last timestamp:\s*(?P<last_timestamp>\d+).*$', self.callback_end2],
            ['^.*$', self.callback_no_rule2]
        ]

        # Compile all regexp
        for regexp_string, regexp_rules in REGEXP_GLOSSARY:
            self.compiled_regexp.append([re.compile(regexp_string), regexp_rules])

        for regexp_string, regexp_rules in REGEXP_GLOSSARY2:
            self.compiled_regexp2.append([re.compile(regexp_string), regexp_rules])

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

    def filter_line2(self):
        # Parse self.final_text to add duration of state per tid
        # Memorize index and timestamp of previous state change per tid and compute duration of
	# previous state when new state change is seen
        # For last state change, duration will be from state change until "Last timestamp"
        for line_index in range(0, len(self.final_text)):
            line = self.final_text[line_index]

            for regexp, regexp_rules in self.compiled_regexp2:
                # Test if regexp matches
                res = regexp.search(line)
                # if regexp match
                if res:
                    # Execute regexp_rules
                    if regexp_rules:
                        regexp_rules(res, line_index)
                    # Only one regexp per line
                    break



    def callback_context_switch_post(self, timestamp, cpu_id, next_tid, name, prev_tid,
				     prev_task_name, prev_state):
        self.last_timestamp = timestamp

        if (prev_task_name != ""):
            self.tid_names[prev_tid] = prev_task_name
            self.prev_tids[cpu_id] = prev_tid

	# short format requires storage of previous running thread per core
        if (cpu_id in self.prev_tids):
            prev_tid = self.prev_tids[cpu_id]

            if (prev_tid in self.tid_states):
                if (self.tid_states[prev_tid] > 0): # goes to queue/sleep but was not running
                    # choice is to do as if running and we keep simply trace of that
                    self.t1_errors[prev_tid] += 1

            if (prev_task_name != "") and (prev_state != ""):
                if (res.group('prev_state') == 'R'):
                    self.tid_states[prev_tid] = 1 # queued
                    new_state = "Q"
                else:
                    self.tid_states[prev_tid] = 2 # sleep
                    new_state = "S"

            else:
                self.tid_states[prev_tid] = 4 # Queued or sleep
                new_state = "X"

            # new state of previous thread if previous thread not seen for first time
            self.final_text.append("%-10d %s %-16s:%-5s %s" % (self.last_timestamp,
							       cpu_id,
							       self.tid_names[prev_tid],
							       prev_tid,
							       new_state))


        # handle "next task"
        if (next_tid == 0):
            next_tid = -cpu_id
        if (name == "swapper"):
            name += str(cpu_id)
        self.tid_names[next_tid] = name
        if (next_tid not in self.tid_states):
	    # consider was queued first time we see it to trigger no transition error
            self.tid_states[next_tid] = 1
        if (self.tid_states[next_tid] != 1): # process lifecycle: process must be queued to run
            # goes running but was not queued (missing wakeup ?)
            self.t2_errors[next_tid] += 1

        self.tid_states[next_tid] = 0 # running
        self.final_text.append("%-10s %s %-16s:%-5s R" % (self.last_timestamp,
							  cpu_id,
							  self.tid_names[next_tid],
							  next_tid))
        self.prev_tids[cpu_id] = next_tid

    def callback_context_switch_ftrace(self, res, line):
        timestamp = int(float(res.group('timestamp')) * 1000.0)
	if self.reference_timestamp == 0:
		self.reference_timestamp = timestamp
	timestamp = timestamp - self.reference_timestamp
        cpu_id = int(res.group('cpu_id'))
        name = res.group('next_task_name').rstrip(' ')
        tid = int(res.group('next_tid'))

        self.callback_context_switch_post(timestamp, cpu_id, tid, name, 0, "", "" )

    def callback_irq_start_ftrace(self, res, line):
        timestamp = int(float(res.group('timestamp')) * 1000.0)
	if self.reference_timestamp == 0:
		self.reference_timestamp = timestamp
	timestamp = timestamp - self.reference_timestamp
        cpu_id = int(res.group('cpu_id'))
        irq_id = int(res.group('irq_id'))
        irq_name = res.group('irq_name')
        self.final_text.append("%d %d irqin %d unknown_func %s 0x0" % (timestamp,
								       cpu_id,
								       irq_id,
								       irq_name))

    def callback_irq_stop_ftrace(self, res, line):
        timestamp = int(float(res.group('timestamp')) * 1000.0)
	if self.reference_timestamp == 0:
		self.reference_timestamp = timestamp
	timestamp = timestamp - self.reference_timestamp
        cpu_id = int(res.group('cpu_id'))
        irq_id = int(res.group('irq_id'))
        self.final_text.append("%d %d irqout %d" % (timestamp, cpu_id, irq_id))

    def callback_no_rule(self, res, line):
        self.final_text.append(line.rstrip('\n'))
        return

    def callback_end(self, res, line):
        if res != None:
            self.final_text.append(line.rstrip('\n'))
        else:
            self.final_text.append("Last timestamp: %d" % self.last_timestamp)

        self.last_timestamp_found = 1

        print("\nT1: goes to queued/sleep but was not running")
        keys = self.t1_errors.keys()
        keys.sort()
        for key in keys:
            print("%s: %s %d") % (self.tid_names[key], key, self.t1_errors[key])

        print("\nT2: goes running but was not queued (missing wakeup ?)")
        keys = self.t2_errors.keys()
        keys.sort()
        for key in keys:
            print("%s: %s %d") % (self.tid_names[key], key, self.t2_errors[key])

        print("\nT3: wakes up but was not sleeping")
        keys = self.t3_errors.keys()
        keys.sort()
        for key in keys:
            print("%s: %s %d") % (self.tid_names[key], key, self.t3_errors[key])

    def callback_context_switch2(self, res, line_index):
        tid = int(res.group('tid'))
        self.last_timestamp = int(res.group('timestamp'))

        # per tid, go back to previous line, compute stamp current - previous stamp and add
	# to previous line
        if (tid in self.previous_line):
            duration = self.last_timestamp - self.previous_line[tid][1]
            self.final_text[self.previous_line[tid][0]] = \
		self.final_text[self.previous_line[tid][0]] + \
		" %-10d (%-10d %.3f ms)" % (duration,
					    self.previous_line[tid][1] + self.reference_timestamp,
					    self.previous_line[tid][1] * self.convert)

        self.previous_line[tid] = [line_index, self.last_timestamp]


    def callback_reference_timestamp(self, res, line_index):
        self.reference_timestamp = int(res.group('ref_time'))

    def callback_end2(self, res, line_index):
        # add diff from last timestamp to current time to all tids
        if res != None:
            self.last_timestamp = int(res.group('last_timestamp'))

        # add timing to end of file for last switch of all threads
        for tid in self.previous_line:
            last_duration = self.last_timestamp - self.previous_line[tid][1]
            self.final_text[self.previous_line[tid][0]] = \
		self.final_text[self.previous_line[tid][0]] + \
		" %-10d (%-10d %.3f ms)" % (last_duration,
					    self.previous_line[tid][1] + self.reference_timestamp,
					    self.previous_line[tid][1] * self.convert)

        # dump everything
        print "\n%-9s %s %-15s:%-4s STA %-9s (%-10s)" % ("TIMESTAMP", "CPU", "NAME", "PID",
							 "DURATION", "ABS_TIME tick/REL_TIME ms")
        for i in range(0, len(self.final_text)):
            print self.final_text[i]

    def callback_no_rule2(self, res, line_index):
        return

global filterMaster
filterMaster = FilterTrace()

def Main(argv):
    usage = "Usage: %prog FILENAME [options]"
    parser = OptionParser(usage = usage)
    (options, args) = parser.parse_args()

    if len(args) != 1:
        print "Usage: type python <program>.py -h"
        return

    # First pass, fill each array index with 1 thread + its new status + timestamp
    # (basically 1 context switch will give 2 indexes, 1 wakeup 1 index)
    # only missing info is duration of state
    file = open(args[0], 'r')
    string = file.readline()

    while string:
        filterMaster.filter_line(string)
        string = file.readline()

    if filterMaster.last_timestamp_found == 0:
        filterMaster.callback_end(None, 0)

    # 2nd pass, add duration of state of tid
    filterMaster.filter_line2()

if __name__=="__main__":
    Main(sys.argv[1:])
