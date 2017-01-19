#!/usr/bin/env python2

#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

# A S ==> R B, A goes to sleep state, A runtime is timestamp_switch - previous_timestamp.
# B state goes to running
# A R ==> R B, A goes to queue state, A runtime is timestamp_switch - previous_timestamp.
# B state goes to running

# IMPORTANT: timeslice must be lower than max running time of tasks

import os, sys, re, collections
from operator import itemgetter
from collections import defaultdict
from optparse import OptionParser
import pylab as P
import matplotlib
import array
import Tkinter as Tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg
from random import randint

sys.path.append(os.path.dirname(sys.argv[0]))

### Timestamp handling functions

timestamp_ref_module = False

try:
	from timestamp_ref import *
	timestamp_ref_module = True
except:
	pass

if not timestamp_ref_module:
	class Timestamp_Ref:
	    def __init__(self):
	        self.convert = 1.0 / 1000

	    def convert_t32k_absolute(self, t32k):
		return (t32k * self.convert)

	    def convert_t32k_offset(self, t32k):
		return (t32k * self.convert)

	    def set_32k_offset(self, ts):
		return

	global ref_ts
	ref_ts = Timestamp_Ref()

class IRQTrace:
    def __init__(self):
        self.compiled_regexp = []
        self.timestamp_start_b = 0
        self.timestamp_end_b = 0
        self.irqtable = {}
        self.irqstack = {}
        self.last_ts = {}
        self.duration = {}
        self.plotx = []
        self.ploty = []

        REGEXP_GLOSSARY = [
            # backup rule
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d)\s+irqin\s+(?P<irq_id>\d+)\s+\S+\s+' \
	     '(?P<irq_name>[\w \.#-_/]+)\s+0x\S+.*$', self.callback_irq_start],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d)\s+irqin\s+x(?P<hexa_irq_id>[0-9a-fA-F]+)' \
	     '\s+\S+\s+(?P<irq_name>[\w \.#-_/{}]+)\s+0x\S+.*$', self.callback_irq_start],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d)\s+irqout\s+(?P<irq_id>\d+).*$',
		self.callback_irq_stop
	    ],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d)\s+irqout\s+x(?P<hexa_irq_id>[0-9a-fA-F]+).*$',
	     self.callback_irq_stop],
            ['^Last timestamp:\s*(?P<last_timestamp>\d+).*$', self.callback_end],
            ['^.*$', self.callback_no_rule]
        ]

        # Compile all regexp
        for regexp_string, regexp_rules in REGEXP_GLOSSARY:
            self.compiled_regexp.append([re.compile(regexp_string), regexp_rules])

    def filter_line(self, line, line_number):
        # Parse all regexps
        for regexp, regexp_rules in self.compiled_regexp:
            # Test if regexp matches
            res = regexp.search(line)
            # if regexp match
            if res:
               # Execute regexp_rules
               if regexp_rules:
                  regexp_rules(res, line, line_number)
               # Only one regexp per line
               break

    def callback_irq_stop(self, res, line, line_number):
        ts = int(res.group('timestamp'))
	timestamp = ref_ts.convert_t32k_offset(ts)

        if (timestamp < self.timestamp_start_b) or (timestamp > self.timestamp_end_b):
            return

        try:
            irqid = int(res.group('irq_id'))
        except IndexError:
            irqid = int(res.group('hexa_irq_id'), 16)
        cpu_id = int(res.group('cpu_id'))

        irq_current = self.irqstack[cpu_id].pop()
        if (irq_current != irqid):
            print "WARNING: removing %u but %u on irq stack" % (irqid, irq_current)
            self.irqstack[cpu_id].append(irq_current)
            return

        irq_prev = self.irqstack[cpu_id][len(self.irqstack[cpu_id]) - 1]
        self.plotx[cpu_id] += [timestamp, timestamp]
        self.ploty[cpu_id] += [irqid, irq_prev]

        if self.last_ts[str(cpu_id)+str(irqid)] != -1:
            if self.duration.has_key(irqid):
                self.duration[irqid] += timestamp - self.last_ts[str(cpu_id)+str(irqid)]
            else:
                self.duration[irqid] = timestamp - self.last_ts[str(cpu_id)+str(irqid)]
        else:
            print "ERROR: missing start event in line", line
            return

        self.last_ts[str(cpu_id)+str(irqid)] = -1

    def callback_irq_start(self, res, line, line_number):
        ts = int(res.group('timestamp'))
	timestamp = ref_ts.convert_t32k_offset(ts)

        if (timestamp < self.timestamp_start_b) or (timestamp > self.timestamp_end_b):
            return

        try:
            irqid = int(res.group('irq_id'))
        except IndexError:
            irqid = int(res.group('hexa_irq_id'), 16)
        cpu_id = int(res.group('cpu_id'))

        if (cpu_id + 1) > len(self.plotx):
            for i in range(len(self.plotx), cpu_id + 1):
                self.plotx.append([])
                self.ploty.append([])
                self.irqtable[-i-1] = 'noirq_cpu'+str(i)
                self.irqstack[i] = [-cpu_id - 1]

        irqid_prev = self.irqstack[cpu_id][len(self.irqstack[cpu_id]) - 1]
        self.plotx[cpu_id] += [timestamp, timestamp]
        self.ploty[cpu_id] += [irqid_prev, irqid]
        self.irqstack[cpu_id].append(irqid)

        self.irqtable[irqid] = res.group('irq_name')
        self.last_ts[str(cpu_id)+str(irqid)] = timestamp


    def callback_no_rule(self, res, line, line_number):
        return

    def callback_end(self, res, line, line_number):
        return

    # goal is to keep only some contributors in their contribution order then put rest as
    # others then tid 0
    def draw(self):
        ploty_final = []
        plot_conv = {}
        count = 1

        for i in range(0, len(self.plotx)):
            ploty_final.append([])
            plot_conv[-i-1] = -i-1

        items = self.duration.items()
        items.sort(key = itemgetter(1), reverse=True)

        # class processes, <= 0 at bottom then up by order of frequence
        for irqid, duration in items:
            if irqid >= 0:
                plot_conv[irqid] = count
                count += 1

        # prepare y axe. <= 0 do not move, other processes are classified
        for i in range(0, len(self.plotx)):
            for irqid in self.ploty[i]:
                ploty_final[i].append(plot_conv[irqid])

        ticks_pos = []
        ticks_names = []

        for key, value in self.irqtable.items():
            ticks_pos.append(plot_conv[key])
            if key >= 0:
                ticks_names.append(value + ":" + str(key))
            else:
                ticks_names.append(value)
        colors = [ 'blue', 'green', 'magenta', 'cyan', 'brown', 'orange' ]

        for i in range(0, len(self.plotx)):
            P.plot(self.plotx[i], ploty_final[i], colors[i % len(colors)])
        P.yticks(ticks_pos, ticks_names)
        P.ylabel('IRQs')
        P.gca().grid(True)

        y1, y2 = P.ylim()
        if y2 > 15:
           y2 = 15
        P.ylim(-3, y2)

class FilterTrace:
    def __init__(self):
        self.filename = ""
        self.compiled_regexp = []
        self.pid_names = {}
        self.timestamp_start_b = 0 # start boundary in tick
        self.timestamp_end_b = 0  # stop boundary in tick
        self.filename = "" # used to label figure
        self.run_time = defaultdict(int) # run time per tid, computed on basis of 1 timeslice
					 # then reset
        self.plotx = []
        self.ploty = []
        self.last_timestamp = 0
	self.label_enh = {}

        REGEXP_GLOSSARY = [
            # backup rule
            ['^.*psel.*$', None],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+)\s+(?P<duration>\d+).*$',
	     self.callback_context_switch
	    ],
            # End
            ['^Last timestamp:\s*(?P<last_timestamp>\d+).*$', self.callback_end],
            ['^Ref timestamp:\s*(?P<ref_time>\d+)$', self.callback_reference_timestamp],
            ['^.*$', self.callback_no_rule]
	]

        # Compile all regexp
        for regexp_string, regexp_rules in REGEXP_GLOSSARY:
            self.compiled_regexp.append([re.compile(regexp_string), regexp_rules])

    def filter_line(self, line, line_number):
        # Parse all regexps
        for regexp, regexp_rules in self.compiled_regexp:
            # Test if regexp matches
            res = regexp.search(line)
            # if regexp match
            if res:
               # Execute regexp_rules
               if regexp_rules:
                  regexp_rules(res, line, line_number)
               # Only one regexp per line
               break

    def callback_context_switch(self, res, line, line_number):
        ts = int(res.group('timestamp'))
	timestamp = ref_ts.convert_t32k_offset(ts)

        if (timestamp < self.timestamp_start_b) or (timestamp > self.timestamp_end_b):
            return

        # only update on running processes
        if (res.group('state') != 'R'):
            return

        # before start but eventually timestamp + duration crosses boundary
        duration = ref_ts.convert_t32k_absolute(int(res.group('duration')))

        cpu_id = int(res.group('cpu_id'))

        if (cpu_id + 1) > len(self.plotx):
            for i in range(0, cpu_id + 1 - len(self.plotx)):
                self.plotx.append([])
                self.ploty.append([])

        run_tid = int(res.group('tid'))
        self.pid_names[run_tid] = res.group('task_name').rstrip(' ') + ":" + "%d" % run_tid
        self.run_time[run_tid] += duration
        self.plotx[cpu_id] += [timestamp, timestamp + duration]
        self.ploty[cpu_id] += [run_tid, run_tid]


    def callback_no_rule(self, res, line, line_number):
        return

    def callback_end(self, res, line, line_number):
        ts = int(res.group('last_timestamp'))
        self.last_timestamp = ref_ts.convert_t32k_offset(ts)

    def callback_reference_timestamp(self, res, line, line_number):
	ref_ts.set_32k_offset(int(res.group('ref_time')))

    # goal is to keep only some contributors in their contribution order then put rest as
    # others then tid 0
    def draw(self):
        ploty_final = []
        plot_conv = {}
        count = 2
        for i in range(0, len(self.plotx)):
            ploty_final.append([])
            plot_conv[-i] = -i

        items = self.run_time.items()
        items.sort(key = itemgetter(1), reverse=True)

        # class pirocesses, <= 0 at bottom then up by order of frequence
	plot_conv[1] = 1
	plot_conv[0] = 0
        for run_tid, duration in items:
            if run_tid > 1:
                plot_conv[run_tid] = count
                count += 1

        # prepare y axe. <= 0 do not move, other processes are classified

        for i in range(0, len(self.plotx)):
            for run_tid in self.ploty[i]:
                ploty_final[i].append(plot_conv[run_tid])

        ticks_pos = []
        ticks_names = []
        for key, value in self.pid_names.items():
            ticks_pos.append(plot_conv[key])
            ticks_names.append(value)
        colors = [ 'blue', 'green', 'magenta', 'cyan', 'brown', 'orange' ]

        for i in range(0, len(self.plotx)):
            P.plot(self.plotx[i], ploty_final[i], colors[i])

        P.yticks(ticks_pos, ticks_names)
        P.ylabel('Running threads')
        P.title(self.filename)

        y1, y2 = P.ylim()
        if y2 > 30:
           y2 = 30
        P.ylim(-3, y2)

        P.gca().grid(True, linestyle=':', color='Grey')

class FilterTrace_ts:
    def __init__(self):
        self.compiled_regexp_pre = []
        self.compiled_regexp = []
        self.pid_names = {}
        self.pid_state = {}
        self.timestamp_start_b = 0 # start boundary in ms
        self.timestamp_end_b = 0 # stop boundary in ms
        self.timeslice_ms = 0 # timeslice duration in ms
        self.plots = {} # all run-time per tid and timeslice
        self.leftstamp = [] # left of histograms
        self.widthslices = [] # width of histogram
        self.countplots = 0 # exact number of bars to trace TODO is it not size of plots ?
        self.filename = "" # used to label figure
        self.run_time = defaultdict(int) # run time per tid, computed on basis of 1 timeslice
					 # then reset
        self.timestamp_end = 0 # end of current timeslice in tick
        self.ref_time = 0
        self.num_of_cores = 0
        self.previous_timestamp = 0
        self.durationperfreq = []
	self.mhz = False

        REGEXP_GLOSSARY_PRE = [
            # backup rule
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+)\s+(?P<duration>\d+).*$',
	     self.callback_context_switch_pre
	    ],
            ['^.*$', self.callback_no_rule]
        ]

        REGEXP_GLOSSARY = [
            # backup rule
            ['^.*psel.*$', None],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+)\s+(?P<duration>\d+).*$',
	     self.callback_context_switch
	    ],
            ['^Last timestamp:\s*(?P<last_timestamp>\d+).*$', self.callback_end],
            ['^Last timestamp:\s*(?P<last_timestamp>\d+).*$', self.callback_end],
            ['^Ref timestamp:\s*(?P<ref_time>\d+)$', self.callback_reference_timestamp],
            ['^.*$', self.callback_no_rule]
        ]

        # Compile all regexp
        for regexp_string, regexp_rules in REGEXP_GLOSSARY_PRE:
            self.compiled_regexp_pre.append([re.compile(regexp_string), regexp_rules])
        for regexp_string, regexp_rules in REGEXP_GLOSSARY:
            self.compiled_regexp.append([re.compile(regexp_string), regexp_rules])

    def filter_line_pre(self, line, line_number):
        # Parse all regexps
        for regexp, regexp_rules in self.compiled_regexp_pre:
            # Test if regexp matches
            res = regexp.search(line)
            # if regexp match
            if res:
               # Execute regexp_rules
               if regexp_rules:
                  regexp_rules(res, line, line_number)
               # Only one regexp per line
               break

    def filter_line(self, line, line_number):
        # Parse all regexps
        for regexp, regexp_rules in self.compiled_regexp:
            # Test if regexp matches
            res = regexp.search(line)
            # if regexp match
            if res:
               # Execute regexp_rules
               if regexp_rules:
                  regexp_rules(res, line, line_number)
               # Only one regexp per line
               break

    def callback_context_switch_pre(self, res, line, line_number):
        cpu_id = int(res.group('cpu_id'))
        if (cpu_id + 1 > self.num_of_cores):
            self.num_of_cores = cpu_id + 1
            #print("Found %d core(s)") % (cpu_id + 1)

    def callback_context_switch(self, res, line, line_number):
	if self.timestamp_start_b == 0:
		self.timestamp_start_b = ref_ts.convert_t32k_offset(0)

        # 1st slice time
        if (self.timestamp_end == 0):
            self.timestamp_end = self.timestamp_start_b + self.timeslice_ms
            self.previous_timestamp = self.timestamp_start_b
            # pre-create idle tasks as others = num_of_cores - tids_to_draw - idle load
            for i in range(0, self.num_of_cores):
                self.plots[0 - i] = []
                self.pid_names[0 - i] = "main_task"

        timestamp = ref_ts.convert_t32k_offset(int(res.group('timestamp')))
        current_tid = int(res.group('tid'))
        self.pid_names[current_tid] = res.group('task_name').rstrip(' ')

        # if process is seen for the 1st time, plots are missing.Add them set to 0 to draw histogram
        if current_tid not in self.plots:
            self.plots[current_tid] = []
            for i in range(0, self.countplots):
                self.plots[current_tid].append(0)

        if (timestamp >= self.timestamp_end_b) or (timestamp < self.timestamp_start_b): # after end
            if (res.group('state') != 'R'):
                self.pid_state[current_tid] = 'S'
            else:
                self.pid_state[current_tid] = 'R'

            return

        # potentially conclude current slice and several other slices
        while timestamp > self.timestamp_end:
            # start by adding reamining time until timestamp_end
            for tid in self.pid_state:
                if self.pid_state[tid] == 'R':
                    self.run_time[tid] += self.timestamp_end - self.previous_timestamp

            # get only items contributing to this slice and sort them to display them by order of
	    # contribution
            items = self.run_time.items()
            items.sort(key = itemgetter(1), reverse=True) # inverse key and value to sort by value

            timestamp_start = self.timestamp_start_b + (self.countplots * self.timeslice_ms)
            timeslice = self.timestamp_end - timestamp_start

            # update all thread with %, even if no contribution so as to plot them after (Note: if
	    # will create keys in run_time so do it before print of contributors)
            for tid in self.plots:
                self.plots[tid].append(self.run_time[tid] * 100.0 / timeslice)
            self.widthslices.append(timeslice)
            self.leftstamp.append(timestamp_start)

            # start next slice, start is timestamp_end and end is start + timeslice
            self.run_time.clear()
            self.countplots += 1
            self.previous_timestamp = self.timestamp_end
            self.timestamp_end = self.timestamp_start_b + ((self.countplots + 1) * self.timeslice_ms)

        # accumulate running times from previous_timestamp to current_timestamp
        for tid in self.pid_state:
            if self.pid_state[tid] == 'R':
                self.run_time[tid] += timestamp - self.previous_timestamp

        # current process state shall be updated
        if (res.group('state') != 'R'):
            self.pid_state[current_tid] = 'S'
        else:
            self.pid_state[current_tid] = 'R'

        # update previous_timestamp to current one
        self.previous_timestamp = timestamp


    def callback_reference_timestamp(self, res, line, line_number):
        self.ref_time = int(res.group('ref_time'))

    def callback_no_rule(self, res, line, line_number):
        return

    def callback_end(self, res, line, line_number):
        return

    def draw(self):

	total_per_cpu = []
	x_val = []

        for j in range(0, self.num_of_cores):
            total_per_cpu.append([])

        for i in range(0, self.countplots):
            # Remove idle
            for j in range(0, self.num_of_cores):
		load = 100 - self.plots[0 - j][i]

		if self.mhz:
		   # Process average CPU clock speed
		   clock = filterMaster.getClock("MPU", self.leftstamp[i],
						 self.leftstamp[i] + self.timeslice_ms)
		   load *= clock / 100

		# Append twice (stairs effect)
		total_per_cpu[j].append(load)
		total_per_cpu[j].append(load)
            x_val.append(self.leftstamp[i])
            if i < (len(self.leftstamp) -1):
               x_val.append(self.leftstamp[i+1])
            else:
               x_val.append(self.leftstamp[i] + self.timeslice_ms)

        colors = [ 'blue', 'green', 'magenta', 'cyan', 'brown', 'orange' ]

        for j in range(0, self.num_of_cores):
            P.plot(x_val, total_per_cpu[j], colors[j], label='CPU'+str(j))

	if self.mhz:
	        P.ylabel('CPU load (MHz per core)')
	else:
		P.yticks([0, 20, 40, 60, 80, 100])
		P.ylim(-10, 110)
	        P.ylabel('CPU load (% per core)')

        P.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc=3, ncol=self.num_of_cores, mode="expand",
		 borderaxespad=0., prop={'size':11})
        P.gca().grid(True)


global filterMaster
filterMaster = FilterTrace()

global filterMaster_ts
filterMaster_ts = FilterTrace_ts()

global irqTrace
irqTrace = IRQTrace()

def Main(argv):

    line_number = 0

    usage = "Usage: %prog FILENAME [options]"
    parser = OptionParser(usage = usage)
    parser.add_option("-s", "--start", action="store", type="int", dest="start", default=0,
		      help="Start time of analysis in ms")
    parser.add_option("-e", "--end", action="store", type="int", dest="end", default=1000000000,
		      help="End time of analysis in ms")
    parser.add_option("-c", "--core", action="store", type="int", dest="num_of_cores", default=0,
		      help="Number of cores, default is 0, i.e. tool automatically tries to " \
			   "identify num of cores, can fail if 1 core stays in Idle")
    parser.add_option("", "--norun", action="store_false", dest="running_threads_plot",
		      default=True,
		      help="Disable graph displaying running threads")
    parser.add_option("", "--nocoreload", action="store_false", dest="load_per_core_plot",
		      default=True,
		      help="Disable graph displaying CPU load per core")
    parser.add_option("", "--noirq", action="store_false", dest="irq_plot", default=True,
		      help="Disable graph displaying IRQs")
    parser.add_option("-t", "--timeslice", action="store", type="int", dest="timeslice",
		      default=150, help="Timeslice in ms for CPU load per core plot, default 150")
    parser.add_option("", "--tsref", action="store", type="str", dest="timestamp_file", default="",
		      help="Use timestamp alignment file")
    parser.add_option("", "--mhz", action="store_true", dest="cpu_mhz", default=False,
		      help="CPU load per core in MHz")

    (options, args) = parser.parse_args()

    if len(args) != 1:
        print "Usage: type python <program>.py -h"
        return

    filterMaster.timestamp_start_b = int(options.start)
    filterMaster.timestamp_end_b = int(options.end)

    if str(options.timestamp_file):
	    if timestamp_ref_module:
		ref_ts.set_file(str(options.timestamp_file))
	    else:
		print "\'timestamp_ref\' module not found: \'--tsref\' discarded"

    filterMaster_ts.timestamp_start_b = int(options.start)
    filterMaster_ts.timestamp_end_b = int(options.end)
    filterMaster_ts.timeslice_ms = options.timeslice
    filterMaster_ts.num_of_cores = options.num_of_cores
    filterMaster_ts.mhz = options.cpu_mhz

    irqTrace.timestamp_start_b = int(options.start)
    irqTrace.timestamp_end_b = int(options.end)

    # CPU load detailed diagram
    ###########################

    file = open(args[0], 'r')
    string = file.readline()

    # search for regexp
    while (string ):
        line_number += 1
        filterMaster.filter_line(string, line_number)
        string = file.readline()
    file.close()

    # CPU load per core
    ###################

    if options.load_per_core_plot:
       file = open(args[0], 'r')
       line_number = 0
       string = file.readline()

       # search num of cores
       if (filterMaster_ts.num_of_cores == 0):
           while (string):
               filterMaster_ts.filter_line_pre(string, line_number)
               string = file.readline()

       file.close()

       # search for regexp
       file = open(args[0], 'r')
       line_number = 0
       string = file.readline()

       while (string):
           line_number += 1
           filterMaster_ts.filter_line(string, line_number)
           string = file.readline()

       file.close()

    # IRQ diagram
    #############

    if options.irq_plot:
       file = open(args[0], 'r')
       string = file.readline()

       # search for regexp
       while (string ):
           line_number += 1
           irqTrace.filter_line(string, line_number)
           string = file.readline()
       file.close()

    # Display
    #########

    #Update label size for Y axis for all charts
    matplotlib.rc('ytick', labelsize=10)

    bottom_margin = 0.05
    margin = 0.03
    first_graph = None

    #############################################################
    # Window 1: MPU Thread/IRQ execution                        #
    #############################################################
    win1_num_plot = []
    if options.running_threads_plot:
        win1_num_plot.append(2)
    if options.irq_plot:
        win1_num_plot.append(1)

    current_vert = 1 - margin
    current_plot = 0

    if len(win1_num_plot) > 0:
       left_margin = 0.2
       win1_vert_size = ((1 - bottom_margin - len(win1_num_plot) * margin) / sum(win1_num_plot))


       fig1 = P.figure()

       if options.running_threads_plot:
           current_vert -= win1_vert_size * win1_num_plot[current_plot]
           gr = P.axes([left_margin, current_vert, 1-(margin + left_margin),
		       win1_vert_size * win1_num_plot[current_plot]])
           current_vert -= margin
           current_plot += 1
           if (first_graph == None):
               first_graph = gr
           filterMaster.draw()

       if options.irq_plot:
           current_vert -= win1_vert_size * win1_num_plot[current_plot]
           gr = P.axes([left_margin, current_vert, 1-(margin + left_margin),
		       win1_vert_size * win1_num_plot[current_plot]], sharex = first_graph)
           current_vert -= margin
           current_plot += 1
           if (first_graph == None):
               first_graph = gr
           irqTrace.draw()

       P.xlabel('time (ms)')

       root1 = Tk.Tk()
       root1.title(string="MPU Thread/IRQ execution (" + args[0] + ")")
       canvas1 = FigureCanvasTkAgg(fig1, master=root1)
       canvas1.show()
       canvas1.get_tk_widget().pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)
       toolbar1 = NavigationToolbar2TkAgg( canvas1, root1 )
       toolbar1.update()
       canvas1._tkcanvas.pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)


    #############################################################
    # Window 2: MPU load                                        #
    #############################################################
    win2_num_plot = []
    cpu_label_margin = 0.05

    if options.load_per_core_plot:
        win2_num_plot.append(1)

    current_vert = 1 - margin
    current_plot = 0


    if len(win2_num_plot) > 0:
       left_margin = 0.2
       win2_vert_size = ((1 - bottom_margin - len(win2_num_plot) * margin) / sum(win2_num_plot))
       fig2 = P.figure()

       if options.load_per_core_plot:
           current_vert -= win2_vert_size * win2_num_plot[current_plot]
           gr = P.axes([left_margin, current_vert, 1-(margin + left_margin),
		       (win2_vert_size - cpu_label_margin) * win2_num_plot[current_plot]],
		       sharex = first_graph)
           current_vert -= margin
           current_plot += 1
           if (first_graph == None):
               first_graph = gr
           filterMaster_ts.draw()

           P.xlabel('time (ms)')

       if len(win1_num_plot) > 0:
           root2 = Tk.Toplevel(root1)
       else:
           root2 = Tk.Tk()

       root2.title(string="MPU load (" + args[0] + ")")
       canvas2 = FigureCanvasTkAgg(fig2, master=root2)
       canvas2.show()
       canvas2.get_tk_widget().pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)
       toolbar2 = NavigationToolbar2TkAgg( canvas2, root2 )
       toolbar2.update()
       canvas2._tkcanvas.pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)

    #############################################################

    if len(win1_num_plot) > 0:
        root = root1
    else:
        root = root2

    def _quit():
        root.quit()     # stops mainloop
        root.destroy()  # this is necessary on Windows to prevent
                    # Fatal Python Error: PyEval_RestoreThread: NULL tstate

    root.protocol("WM_DELETE_WINDOW", _quit)
    root.mainloop()


if __name__=="__main__":
    Main(sys.argv[1:])
