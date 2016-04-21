#!/usr/bin/python

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

# IMPORTANT: timeslice must be bigger than max running time of tasks

import os, sys, re, collections
from operator import itemgetter
from collections import defaultdict
from optparse import OptionParser
import pylab as P
import matplotlib
import Tkinter as Tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg

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

class FilterTrace:
    def __init__(self):
        self.compiled_regexp_pre = []
        self.compiled_regexp = []
        self.compiled_regexp2 = []
        self.pid_names = {}
        self.pid_state = {}
        self.timestamp_start_b = 0 # start boundary in tick
        self.timestamp_end_b = 0 # stop boundary in tick
        self.timeslice_ms = 0 # timeslice duration in ms
        self.plots = {} # all run-time per tid and timeslice
        self.leftstamp = [] # left of histograms
        self.widthslices = [] # width of histogram
        self.countplots = 0 # exact number of bars to trace TODO is it not size of plots ?
        self.filename = "" # used to label figure
        self.threshold_trace = 0 # see options
        self.threshold_average_percentage = 0
        self.threshold_peak_percentage = 0
        self.run_time = defaultdict(int) # run time per tid, computed on basis of 1 timeslice then
					 # reset
        self.timestamp_end = 0 # end of current timeslice in tick
        self.convert = 1.0
        self.ref_time = 0
        self.num_of_cores = 0
        self.previous_timestamp = 0

        REGEXP_GLOSSARY_PRE = [
            # backup rule
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+)\s+(?P<duration>\d+).*$', self.callback_context_switch_pre],
            ['^.*$', self.callback_no_rule]
        ]

        REGEXP_GLOSSARY = [
            # backup rule
            ['^.*psel.*$', None],
            ['^(?P<timestamp>\d+)\s+(?P<cpu_id>\d+)\s+(?P<task_name>[\w \.#-_/]+):(?P<tid>-?\d+)' \
	     '\s+(?P<state>\w+)\s+(?P<duration>\d+).*$', self.callback_context_switch],
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

    def setFilename(self, filename):
        self.filename = filename

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
            print("Found %d core(s)") % (cpu_id + 1)

    def callback_context_switch(self, res, line, line_number):
	if self.timestamp_start_b == 0:
		self.timestamp_start_b = ref_ts.convert_t32k_offset(0)

        # 1st slice time
        if (self.timestamp_end == 0):
            self.timestamp_end = self.timestamp_start_b + self.timeslice_ms
            self.previous_timestamp = self.timestamp_start_b
            print("Time slice: %d ms") % self.timeslice_ms

            # pre-create idle tasks as others = num_of_cores - tids_to_draw - idle load
            for i in range(0, self.num_of_cores):
                self.plots[0 - i] = []
                self.pid_names[0 - i] = "swapper"

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

            # update all thread with %, even if no contribution so as to plot them after
	    # (Note: if will create keys in run_time so do it before print of contributors)
            for tid in self.plots:
                self.plots[tid].append(self.run_time[tid] * 100.0 / timeslice)
            self.widthslices.append(timeslice)
            self.leftstamp.append(timestamp_start)

            # start next slice, start is timestamp_end and end is start + timeslice
            self.run_time.clear()
            self.countplots += 1
            self.previous_timestamp = self.timestamp_end
            self.timestamp_end = self.timestamp_start_b + \
				 ((self.countplots + 1) * self.timeslice_ms)

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
        ref_ts.set_32k_offset(self.ref_time)

    def callback_no_rule(self, res, line, line_number):
        return

    def callback_end(self, res, line, line_number):
        return

    # goal is to keep only some contributors in their contribution order then put rest as
    # others then tid 0
    def draw(self):
        contributors = defaultdict(int) # sum of all %
        peak_contributors = [] # contribution to peak load in 1 slice
        tids_to_draw = [] # final keys to draw after selection
        others_plot = [] # plot of all non contributing processes, not including tid 0

        # compute the sum on slices of all % of 1 thread and store in contributors.
	# It is needed only to sort by % not tid, otherwise could be done on the fly
        for tid in self.plots:
            for i in range(0, self.countplots):
                contributors[tid] += self.plots[tid][i]
        items = contributors.items()
        items.sort(key = itemgetter(1), reverse=True)

        # get tids to draw according to 2 criterias
        for tid, perc in items: # contribution is more than X% overall
            #print("\n%-16s:%d ") %( self.pid_names[tid], tid),
            # get main contributors to the load, the ones with overall high load...
	    # Do not put Idle task
            if (perc >= (self.threshold_average_percentage * self.countplots)) and (tid > 0):
                tids_to_draw.append(tid)
            # ... and the ones with high load within 1 slice
            for i in range(0, self.countplots):
                #print(" %.2f") % (self.plots[tid][i]), # this is the occasion to dump all values !
                if ((self.plots[tid][i] >= self.threshold_peak_percentage) and
		    (tid not in tids_to_draw) and (tid > 0)):
                    tids_to_draw.append(tid)

        # last possibility to force tids to draw
        #tids_to_draw.append(xxx); tids_to_draw.append(yyy)

        # draw bars and make legend in reverse order
        col = 0 # to cycle among colors
        bars = [] # accumulation of bars, used for legend
        tid_bars = [] # the key associated to the tid traced in a bar
        colors = ['b', 'g', 'r', 'c', 'm', 'k',
                  '#6495ed', '#98fb98', '#cd5c5c', '#e0ffff', '#9370db', '#d3d3d3',
                  '#00bfff', '#adff2f', '#ffa500', '#1e90ff', '#ee82ee', '#708090']
	# bottom of next bar to draw, matplotlib does not handle it itself, what a pain !
        plots_bottom = []
        for i in range(0, self.countplots):
            plots_bottom.append(0)

        # bar drawing itself, note the use of plots_bottom
        for tid in tids_to_draw:
            bars.append(P.bar(self.leftstamp, self.plots[tid], self.widthslices,
			bottom=plots_bottom, color=colors[col % len(colors)]))
            tid_bars.append(tid)
            col += 1
            for i in range(0, self.countplots):
                plots_bottom[i] += self.plots[tid][i]
        # compute others=remaining tids not drawn so 100 - current_sum(plots_bottom) - idle
        for i in range(0, self.countplots):
            others = self.num_of_cores * 100 - plots_bottom[i]
            for j in range(0, self.num_of_cores):
                others -= self.plots[0 - j][i]
            others_plot.append(others)

        others_bar = P.bar(self.leftstamp, others_plot, self.widthslices,
			   bottom=plots_bottom, color='#ffff00')

        P.xlabel('time (s) with timeslice: %s ms' % self.timeslice_ms)
        P.ylabel('CPU Load(%%) - Max is %d00%%' % self.num_of_cores)
        P.title(self.filename)
        P.gca().grid(True)

        # legends in reverse order. We did resizing before as we create new axes
        legends1 = []
        legends2 = []
        legends1.append(others_bar[0])
        legends2.append("others")
        for i in range (0, len(bars)):
            legends1.append(bars.pop()[0])
            tid = tid_bars.pop()
            legends2.append("%s:%d" %(self.pid_names[tid], tid))
        # work-around for bbox_to_anchor only in latest releases,
	# P.legend(legends1, legends2, loc='upper left', bbox_to_anchor=(0.8, 1))
        wa_axes = P.axes([0.8, 0.9, 0.25, 0.01], frameon=False)
        wa_axes.xaxis.set_visible(False);wa_axes.yaxis.set_visible(False)
        legg = wa_axes.legend(legends1, legends2, mode="expand", ncol = 1, borderaxespad = 0)

        for t in legg.get_texts():
            t.set_fontsize(10)


global filterMaster
filterMaster = FilterTrace()

def Main(argv):
    usage = "Usage: %prog FILENAME [options]"
    parser = OptionParser(usage = usage)
    parser.add_option("-s", "--start", action="store", type="int", dest="start", default=0,
		      help="Start time of analysis in ms")
    parser.add_option("-e", "--end", action="store", type="int", dest="end", default=1000000000,
		      help="End time of analysis in ms")
    parser.add_option("-t", "--timeslice", action="store", type="int", dest="timeslice",
		      default=150,
		      help="Timeslice in ms, default 150")
    parser.add_option("--thr_avg", action="store", type="float", dest="thr_avg", default=4,
		      help="Threshold % in average to draw a thread, default 4, can be a flot" \
			   " like 3.39")
    parser.add_option("--thr_peak", action="store", type="float", dest="thr_peak", default=20,
		      help="Threshold % peak in 1 timeslice to draw a thread, default 20, " \
			   "can be a float")
    parser.add_option("--thr_trace", action="store", type="float", dest="thr_trace", default=1,
		      help="Threshold % in 1 timeslice to trace a thread, default 1, "\
			   "can be a float")
    parser.add_option("-c", "--core", action="store", type="int", dest="num_of_cores", default=0,
		      help="Number of cores, default is 0, i.e. tool automatically tries to" \
			   " identify num of cores, can fail if 1 core stays in Idle")
    parser.add_option("", "--tsref", action="store", type="str", dest="timestamp_file", default="",
		      help="Use timestamp alignment file")

    (options, args) = parser.parse_args()
    filterMaster.timestamp_start_b = int(options.start / filterMaster.convert)
    filterMaster.timestamp_end_b = int(options.end / filterMaster.convert)
    filterMaster.timeslice_ms = options.timeslice
    filterMaster.timeslice = int(filterMaster.timeslice_ms / filterMaster.convert)
    filterMaster.threshold_average_percentage = options.thr_avg
    filterMaster.threshold_peak_percentage = options.thr_peak
    filterMaster.threshold_trace = options.thr_trace
    filterMaster.num_of_cores = options.num_of_cores

    if str(options.timestamp_file):
	    if timestamp_ref_module:
		ref_ts.set_file(str(options.timestamp_file))
	    else:
		print "\'timestamp_ref\' module not found: \'--tsref\' discarded"

    if len(args) != 1:
        print "Usage: type python program.py -h"
        return

    file = open(args[0], 'r')
    line_number = 0
    filterMaster.setFilename(args[0])
    string = file.readline()

    # search num of cores
    if (filterMaster.num_of_cores == 0):
        while (string):
            filterMaster.filter_line_pre(string, line_number)
            string = file.readline()

    file.close()

    # search for regexp
    file = open(args[0], 'r')
    line_number = 0
    string = file.readline()

    while (string):
        line_number += 1
        filterMaster.filter_line(string, line_number)
        string = file.readline()

    root = Tk.Tk()
    root.title(string="CPU timeslice (" + args[0] + ")")
    fig = P.gca().get_figure()

    canvas = FigureCanvasTkAgg(fig, master=root)
    canvas.show()
    canvas.get_tk_widget().pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)
    toolbar = NavigationToolbar2TkAgg( canvas, root )
    toolbar.update()
    canvas._tkcanvas.pack(side=Tk.TOP, fill=Tk.BOTH, expand=1)

    filterMaster.draw()

    def _quit():
	root.quit()     # stops mainloop
	root.destroy()  # this is necessary on Windows to prevent
            # Fatal Python Error: PyEval_RestoreThread: NULL tstate


    root.protocol("WM_DELETE_WINDOW", _quit)
    root.mainloop()



if __name__=="__main__":
    Main(sys.argv[1:])
