#!/usr/bin/env python2

#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import sys,re,os

class monitorEvent():
	def __init__(self, time, data1, data2, ptr):
		self.time = time
		self.data1 = data1
		self.data2 = data2
		self.ptr = ptr

class Params():
	# Quark X1000
	TICKS_PER_MSEC = 14318.1

def getData(monitorFile):

	eventList = []

	with open(monitorFile, 'rb') as f:
		for chunk in iter(lambda: f.read(16), ''):
			time = int(chunk[0:4][::-1].encode('hex'), 16)
			data1 = int(chunk[4:8][::-1].encode('hex'), 16)
			data2 = int(chunk[8:12][::-1].encode('hex'), 16)
			ptr = int(chunk[12:16][::-1].encode('hex'), 16)

			if ((time == 0) and (data1 == 0) and (data2 == 0)):
				break
			eventList.append(monitorEvent(time, data1, data2, ptr))
	return eventList

def formatTime(val):
	ticks_per_ms = Params.TICKS_PER_MSEC
	val_ms = val / ticks_per_ms
	return "{:15.3f}".format(val_ms)

def getTask(val):
	global symbols

	if val == 0:
		return "Idle"
	else:
		return symbols[val].replace('_k_task_obj_', '')

def formatEvent(time, data1, data2, ptr):
	global symbols
	global prevTaskName
	global prevTaskId

	mo_stbit0 = 0x20000000
	mo_stbit1 = 0x30000000
	mo_event = 0x40000000
	mo_lcomm = 0x50000000
	evt_type = data2 & 0xF0000000
	mask = 0xFFFFFFF
	evt_mask = 0xFFFFFFC

	if (evt_type == mo_stbit0):
		event = [ "State clear", decodeStateBits(data2 & mask), getTask(data1) ]
	elif (evt_type == mo_stbit1):
		event = [ "State set", decodeStateBits(data2 & mask), getTask(data1) ]
	elif (evt_type == mo_event):
		event = [ "Server event",
			  symbols[data2 & evt_mask].replace('_k_event_obj_',''),
			  "" ]
	elif (evt_type == mo_lcomm):
		event = [ "Server cmd", symbols[ptr], getTask(data1) ]
	elif (evt_type == 0):
		event = [ "Task switch to", "", getTask(data1) ]
		if (ftrace_format):
			ret = ""
			if (prevTaskId >= 0):
				ret = " {:>16}-{:<8} [000] .... {:12}: sched_switch: prev_comm={}" \
					" prev_pid={} prev_prio=0 prev_state=S ==> next_comm={}"   \
					" next_pid={} next_prio=0".format(prevTaskName,
									  prevTaskId,
									  formatTime(time),
									  prevTaskName,
									  prevTaskId,
									  getTask(data1),
									  data1)
			prevTaskName = getTask(data1)
			prevTaskId = data1
			return ret
	else:
		event = [ "UNDEFINED",
			  "data1={:08x}".format(data1),
			  "data2={:08x}".format(data2),
			  "ptr={:08x}".format(ptr) ]

	if (ftrace_format):
		return ""
	else:
		return "{} ms: {:15} {:13} {}".format(formatTime(time),
						      event[0],
						      event[2],
						      event[1])

def decode(eventList):
	for evt in eventList:
		s = formatEvent(evt.time, evt.data1, evt.data2, evt.ptr)
		if len(s) > 0:
			print s

def decodeStateBits(flags):
	bitValue = [ "STOP", "TERM", "SUSP", "BLCK", "GDBSTOP", "PRIO" , "NA" , "NA", "NA", "NA",
		     "NA", "TIME", "DRIV", "RESV", "EVNT", "ENQU", "DEQU", "SEND", "RECV", "SEMA",
		     "LIST", "LOCK", "ALLOC", "GTBL", "RESV", "RESV", "RECVDATA", "SENDDATA" ]
	s = ''

	for x in range(0, len(bitValue) - 1):
		if (flags & (1 << x)):
			s += bitValue[x] + " "

	return s

def loadSymbols(elfFile):
	os.system('readelf -s ' + elfFile + ' > symbols.txt')

	prog = re.compile("\s+\S+:\s+([a-fA-F0-9]+)\s+\d+\s+\S+\s+\S+\s+\S+\s+\S+\s+(\S+)")
	objList = {}

	with open('symbols.txt', 'r') as f:
		for line in f:
			match = prog.match(line)
			if match:
				idx = int(match.group(1), 16)
				if not objList.has_key(idx):
					objList[idx] = match.group(2)
				else:
					s = match.group(2)
					# Prioritize tasks / event symbols when multiple symbols
					# on same address
					if "_k_task_obj" in s or "_k_event_obj" in s:
						objList[idx] = s


	os.system('rm -rf symbols.txt')

	return objList

symbols = {}
prevTaskName = ""
prevTaskId = -1
ftrace_format=0

def Main(argv):
	global symbols

	dumpFile = ""
	elfFile = ""

	sys.argv.pop(0)

	for arg in sys.argv:
		if arg == "--qemu":
			Params.TICKS_PER_MSEC=100000.0
		else:
			if not dumpFile:
				dumpFile = arg
			elif not elfFile:
				elfFile = arg

	if not elfFile:
		print "profile.py [DUMP FILE] [ELF FILE]"
		sys.exit(0)

	eventList = getData(dumpFile)
	symbols = loadSymbols(elfFile)
	decode(eventList)

if __name__ == "__main__":
	Main(sys.argv[1:])
