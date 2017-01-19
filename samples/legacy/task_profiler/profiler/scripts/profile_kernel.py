#!/usr/bin/env python2

#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

import sys,re,os
import datetime

class ContextSwitch():
	def __init__(self, context):
		self.context = context

class Interrupt():
	def __init__(self, irq):
		self.irq = irq

class Sleep():
	def __init__(self, duration, irq):
		self.duration = duration
		self.irq = irq

class TaskState():
	SWITCH = 0
	RESET_BIT = 1
	SET_BIT = 2

	def __init__(self, task, switch, bits):
		self.task = task
		self.switch = switch
		self.bits = bits

class PacketCmd():
	def __init__(self, task, command_ptr):
		self.task = task
		self.command_ptr = command_ptr

class Kevent():
	def __init__(self, event_ptr):
		self.event_ptr = event_ptr

class Event():
	TYPE_ERROR = -1
	TYPE_CONTEXTSWITCH = 1
	TYPE_INTERRUPT = 2
	TYPE_SLEEP = 3
	TYPE_WAKEUP = 4
	TYPE_TASK_STATE_CHANGE = 5
	TYPE_COMMAND_PACKET = 6
	TYPE_KEVENT = 7

	def __init__(self, time, event_type, data):
		self.time = time
		self.etype = event_type
		self.data = data

class EventType():
	PLATFORM_INFO = 255
	CONTEXTSWITCH = 1
	INTERRUPT = 2
	SLEEP = 3
	TASK_STATE_CHANGE = 4
	COMMAND_PACKET = 5
	KEVENT = 6

class Params():
	TICKS_PER_MSEC = 0

def getData(monitorFile):
	global symbols

	MO_STBIT0 = 0x20000000
	MO_STBIT1 = 0x30000000
	MO_EVENT  = 0x40000000
	MO_MASK   = 0xFFFFFFF
	EVT_MASK  = 0xFFFFFFC

	eventList = []
	count = 0

	with open(monitorFile, 'rb') as f:
		while (1):
			c = f.read(1)
			count += 1

			if (len(c) < 1):
				return eventList

			code = int(c.encode('hex'), 16)
			if (code > 0):
				#print "Count={:x}".format(count-1)
				header = c
				cc = f.read(3)
				if (len(cc) < 3):
					return eventList

				header += cc
				count += 3

				evt_type = int(header[0:2][::-1].encode('hex'), 16)
				size = int(header[2:4][::-1].encode('hex'), 16)

				#print "Event {}({})".format(evt_type, size)
				chunk = f.read(size*4)
				if (len(chunk) < size*4):
					return eventList

				count += size*4

				if (evt_type == EventType.PLATFORM_INFO):
					Params.TICKS_PER_MSEC = \
						int(chunk[0:4][::-1].encode('hex'), 16) / 1000.0
				elif (evt_type == EventType.CONTEXTSWITCH):
					time = int(chunk[0:4][::-1].encode('hex'), 16)
					context = int(chunk[4:8][::-1].encode('hex'), 16)
					eventList.append(Event(time,
							       Event.TYPE_CONTEXTSWITCH,
							       ContextSwitch(context)))
				elif (evt_type == EventType.INTERRUPT):
					time = int(chunk[0:4][::-1].encode('hex'), 16)
					irq = int(chunk[4:8][::-1].encode('hex'), 16)
					eventList.append(Event(time,
							       Event.TYPE_INTERRUPT,
							       Interrupt(irq)))
				elif (evt_type == EventType.SLEEP):
					time = int(chunk[0:4][::-1].encode('hex'), 16)
					duration = int(chunk[4:8][::-1].encode('hex'), 16)
					cause = int(chunk[8:12][::-1].encode('hex'), 16)
					eventList.append(Event(time,
							       Event.TYPE_SLEEP,
							       Sleep(duration, cause)))
					eventList.append(Event(time,
							       Event.TYPE_WAKEUP,
							       Sleep(duration, cause)))
				elif (evt_type == EventType.TASK_STATE_CHANGE):
					time = int(chunk[0:4][::-1].encode('hex'), 16)
					task = int(chunk[4:8][::-1].encode('hex'), 16)
					data = int(chunk[8:12][::-1].encode('hex'), 16)
					if data == 0:
						eventList.append(Event(
							time,
							Event.TYPE_TASK_STATE_CHANGE,
							TaskState(task,
								  TaskState.SWITCH,
								  0)))
					elif (data & 0xF0000000) == MO_STBIT0:
						eventList.append(Event(
							time,
							Event.TYPE_TASK_STATE_CHANGE,
							TaskState(task,
								  TaskState.RESET_BIT,
								  data & MO_MASK )))
					elif (data & 0xF0000000) == MO_STBIT1:
						eventList.append(Event(
							time,
							Event.TYPE_TASK_STATE_CHANGE,
							TaskState(task,
								  TaskState.SET_BIT,
								  data & MO_MASK )))
				elif (evt_type == EventType.COMMAND_PACKET):
					time = int(chunk[0:4][::-1].encode('hex'), 16)
					task = int(chunk[4:8][::-1].encode('hex'), 16)
					cmd_ptr = int(chunk[8:12][::-1].encode('hex'), 16)
					eventList.append(Event(time,
							       Event.TYPE_COMMAND_PACKET,
							       PacketCmd(task, cmd_ptr)))
				elif (evt_type == EventType.KEVENT):
					time = int(chunk[0:4][::-1].encode('hex'), 16)
					event = int(chunk[4:8][::-1].encode('hex'), 16)
					eventList.append(Event(time,
							       Event.TYPE_KEVENT,
							       Kevent(event & EVT_MASK)))
				else:
					eventList.append(Event(0, Event.TYPE_ERROR, evt_type))

def findFollowingTask(events, i):
	i = i + 1
	while (i < len(events)) and (events[i].etype != Event.TYPE_CONTEXTSWITCH):
		i = i + 1
	if (i == len(events)):
		return -1
	else:
		return events[i].data.context

def decodeStateBits(flags):
	bitValue = [ "STOP", "TERM", "SUSP", "BLCK", "GDBSTOP", "PRIO" , "NA" , "NA", "NA", "NA",
		     "NA", "TIME", "DRIV", "RESV", "EVNT", "ENQU", "DEQU", "SEND", "RECV", "SEMA",
		     "LIST", "LOCK", "ALLOC", "GTBL", "RESV", "RESV", "RECVDATA", "SENDDATA" ]
	s = ''

	for x in range(0, len(bitValue) - 1):
		if (flags & (1 << x)):
			s += bitValue[x] + " "

	return s

def display(events):
	global ftrace_format
	global isr

	current_task = -1
	for i in range(0, len(events) -1):
		evt = events[i]
		if ftrace_format == 0:
			if (evt.etype == Event.TYPE_CONTEXTSWITCH):
				fTask = findFollowingTask(events, i)
				if (fTask >= 0):
					print "{:12} : {:>16} ---> {:<16}".format(
						formatTime(evt.time),
						getTask(evt.data.context),
						getTask(fTask))
			elif (evt.etype == Event.TYPE_INTERRUPT):
				print "{:12} : IRQ{} handler={}".format(
						formatTime(evt.time),
						evt.data.irq,
						getIsr(evt.data.irq))
			elif (evt.etype == Event.TYPE_SLEEP):
				print "{:12} : SLEPT {} OS ticks".format(
						formatTime(evt.time),
						evt.data.duration)
			elif (evt.etype == Event.TYPE_WAKEUP):
				print "{:12} : WAKEUP IRQ{}".format(
						formatTime(evt.time),
						evt.data.irq)
			elif (evt.etype == Event.TYPE_TASK_STATE_CHANGE):
				if (evt.data.task == 0):
					task = "main_task"
				else:
					task = getSymbol(evt.data.task).replace('_k_task_obj_', '')

				if (evt.data.switch == TaskState.SWITCH):
					print "{:12} :                       " \
					      "Task switch to {}".format(formatTime(evt.time),
									 task)
				elif (evt.data.switch == TaskState.SET_BIT):
					print "{:12} :                       " \
					      "Task bits set ({}) {}".format(
								formatTime(evt.time),
								task,
								decodeStateBits(evt.data.bits))
				elif (evt.data.switch == TaskState.RESET_BIT):
					print "{:12} :                       " \
					      "Task bits reset ({}) {}".format(
								formatTime(evt.time),
								task,
								decodeStateBits(evt.data.bits))
			elif (evt.etype == Event.TYPE_COMMAND_PACKET):
				if (evt.data.task == 0):
					task = "main_task"
				else:
					task = getSymbol(evt.data.task).replace('_k_task_obj_', '')
				print "{:12} :                       " \
				      "Command ({}) {}".format(formatTime(evt.time),
							       task,
							       getSymbol(evt.data.command_ptr))
			elif (evt.etype == Event.TYPE_KEVENT):
				print "{:12} :                       " \
				      "Event {}".format(
						formatTime(evt.time),
						getSymbol(evt.data.event_ptr).replace(
							'_k_event_obj_',''
							)
						)
			else:
				print "ERROR type={:08x}".format(evt.data)
		else:
			if (evt.etype == Event.TYPE_CONTEXTSWITCH):
				ftask_id = findFollowingTask(events, i)
				if (ftask_id > 0):
					task_id = evt.data.context
					task_name = getTask(evt.data.context)
					if task_name == "main_task":
						task_id = 0
					if task_name == "_k_server":
						task_id = 1
					ftask_name = getTask(ftask_id)
					if ftask_name == "main_task":
						ftask_id = 0
					if ftask_name == "_k_server":
						ftask_id = 1

					print " {:>16}-{:<8} [000] .... {:12}: sched_switch:" \
					      " prev_comm={} prev_pid={} prev_prio=0"	      \
					      " prev_state=S ==> next_comm={} next_pid={}"    \
					      " next_prio=0".format(task_name,
								    task_id,
								    formatTime(evt.time),
								    task_name,
								    task_id,
								    ftask_name,
								    ftask_id)
					current_task = evt.data.context
			elif (evt.etype == Event.TYPE_INTERRUPT):
				print " {:>16}-{:<8} [000] .... {:12}: irq_handler_entry: irq={}" \
				      " name={} handler={}".format(getTask(current_task),
								   current_task,
								   formatTime(evt.time),
								   evt.data.irq,
								   evt.data.irq,
								   getIsr(evt.data.irq))
				print " {:>16}-{:<8} [000] .... {:12}: irq_handler_exit: irq={}" \
				      " ret=handled".format(getTask(current_task),
							    current_task,
							    formatTime(evt.time),
							    evt.data.irq)


last_timestamp = 0.0
base_timestamp = 0.0

def formatTime(val):
	global last_timestamp
	global base_timestamp

	ticks_per_ms = Params.TICKS_PER_MSEC
	val_ms = base_timestamp + (val / ticks_per_ms)

	if val_ms < last_timestamp:
		val_ms = val_ms + (0xFFFFFFFF / ticks_per_ms)
		base_timestamp = base_timestamp + (0xFFFFFFFF / ticks_per_ms)

	last_timestamp = val_ms

	return "{:15.3f}".format(val_ms)

def getSymbol(val):
	global symbols

	if symbols.has_key(val):
		return symbols[val]
	else:
		mem_key = 0
		for key, value in symbols.iteritems():
			if key < val:
				if key > mem_key:
					mem_key = key
		symbols[val] = symbols[mem_key] + "+{}".format(val - mem_key)
		return symbols[val]

def getTask(val):
	return getSymbol(val).replace("_stack", "")

def getIsr(val):
	global isr

	if (isr.has_key(val)):
		return isr[val].replace("$","").replace("_stub","")
	else:
		return "??{}".format(val)

def loadSymbols(elfFile):
	os.system('readelf -s ' + elfFile + ' > symbols.txt')

	prog = re.compile("\s+\S+:\s+([a-fA-F0-9]+)\s+\d+\s+\S+\s+\S+\s+\S+\s+\S+\s+(\S+)")
	objList = {}

	with open('symbols.txt', 'r') as f:
		for line in f:
			match = prog.match(line)
			if match:
				address = int(match.group(1), 16)
				if not objList.has_key(address):
					objList[address] = match.group(2)
				else:
					s = match.group(2)
					# Prioritize tasks / event symbols when multiple symbols
					# on same address
					if (("_k_task_obj" in s) or ("_k_event_obj" in s) or
					    ("_idt_base_address" in s)):
						objList[address] = s

	os.system('rm -rf symbols.txt')

	return objList

def getIdtFunc(str1, str2):
	# Format: cc1d0800 008e1000 => 0x00101dcc
	# IDT encoding (see _IdtEntCreate function in zephyr)
	address = str2[6:8]+str2[4:6]+str1[2:4]+str1[0:2]
	if symbols.has_key(int(address, 16)):
		return symbols[int(address, 16)]
	else:
		return "??{}".format(address)

def getSection(address, elfFile):
	os.system('readelf -S ' + elfFile + ' > sections.txt')

	prog = re.compile("\s+\[.+\]\s(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)")

	with open("sections.txt", 'r') as f:
		for line in f:
			match = prog.match(line)
			if match:
				start = int(match.group(2), 16)
				end = start + int(match.group(3), 16)
				name = match.group(1)
				if (address >= start) and (address < end):
					os.system('rm -rf sections.txt')
					return name

	os.system('rm -rf sections.txt')
	return ''

def getIsrTable(elfFile):
	# First get IDT table address '_idt_base_address' symbol
	idt_address = 0
	for addr,sym in symbols.iteritems():
		if sym == '_idt_base_address':
			idt_address = addr

	if idt_address == 0:
		print "IDT table address not found"
		return 0

	sectionName = getSection(idt_address, elfFile)

	if sectionName == '':
		print "IDT section not found"
		return 0

	os.system('readelf -x ' + sectionName + ' ' + elfFile + ' > ' + sectionName + '.txt')

	prog = re.compile(
		"\s+0x([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)" \
		"\s+([0-9a-fA-F]+)\s+\S+")
	symbol_table = {}
	first = 0

	with open(sectionName + '.txt', 'r') as f:
		for line in f:
			match = prog.match(line)
			if match:
				address = int(match.group(1), 16)
				if (first == 0):
					first = address
				symbol_table[address] = match.group(2)
				symbol_table[address+4] = match.group(3)
				symbol_table[address+8] = match.group(4)
				symbol_table[address+12] = match.group(5)

	address_end = address + 12
	capture_on = 0
	index = 0
	isr = {}
	address = first

	while address < address_end:
		if not capture_on:
			if (address == idt_address):
				capture_on = 1

		if (capture_on):
			isr[index] = getIdtFunc(symbol_table[address], symbol_table[address+4])
			index += 1
			isr[index] = getIdtFunc(symbol_table[address+8], symbol_table[address+12])
			index += 1
			if (index == 256):
				break
			address = address + 16
		else:
			address = address + 4

	os.system('rm -rf ' + sectionName + '.txt')
	return isr

symbols = {}
isr = {}
prevTaskName = ""
prevTaskId = -1
ftrace_format = 0

def Main(argv):
	global symbols, isr
	global ftrace_format

	dumpFile = ""
	elfFile = ""

	sys.argv.pop(0)

	iterator = sys.argv.__iter__()
	for arg in iterator:
		if arg == "--ftrace":
			ftrace_format = 1
		elif arg == "-c":
			Params.TICKS_PER_MSEC = float(iterator.next()) / 1000.0
		else:
			if not dumpFile:
				dumpFile = arg
			elif not elfFile:
				elfFile = arg

	if not elfFile:
		print "profile.py [DUMP FILE] [ELF FILE]"
		sys.exit(0)

	symbols = loadSymbols(elfFile)
	isr = getIsrTable(elfFile)
	eventList = getData(dumpFile)
	if (Params.TICKS_PER_MSEC == 0):
		print "Platform info not found ! Use -c option if decoding dump from JTAG"
		sys.exit(0)
	display(eventList)

if __name__ == "__main__":
	Main(sys.argv[1:])
