# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0
import time
import serial
import threading
import queue
from scripts.pm.abstract_power_monitor import Power_Monitor
import numpy as np

class PowerShield(Power_Monitor):
    # metadata format: ([startPattern], byteLength)
    metadataTimestamp = ([0xF0, 0xF3], 9)

    file_path = 'measurementData.csv'

    combinedPatternInfo = []

    acqComplete = False
    endPatternsRead = False
    dataQueue = queue.Queue()

    # adjust this based on available RAM
    chunkSize = 1024 * 1024 # 1MB
    chunkCounter = 0

    def init_power_monitor(self, device_id):
        # PowerShield serial interface
        self.pwsh = serial.Serial(device_id,
            baudrate=3686400,
            bytesize=serial.EIGHTBITS,
            stopbits=serial.STOPBITS_ONE,
            parity=serial.PARITY_NONE,
            timeout=1)
        if not self.pwsh.is_open:
            self.pwsh.open()

        return bool(self.pwsh.is_open)

    def get_data(self):
        data = np.genfromtxt(self.file_path, delimiter=None)
        return data

    def measure_current(self, acquisition_time):
        self.sampling_frequency = '1k'
        self.acquisition_time = acquisition_time

        self.pwsh_cmd("htc")
        print(self.pwsh_get())

        self.pwsh_cmd("format bin_hexa")
        print(self.pwsh_get())
        self.pwsh_cmd("freq " + self.sampling_frequency)
        print(self.pwsh_get())
        self.pwsh_cmd("acqtime 0")
        print(self.pwsh_get())
        self.pwsh_cmd("volt 3300m")
        print(self.pwsh_get())
        self.pwsh_cmd("funcmode high")
        print(self.pwsh_get())

        acqTimeThread = threading.Thread(target=self.stopAcquisition, args=(self.acquisition_time,))
        acqTimeThread.start()
        rawToFileThread = threading.Thread(target=self.rawToFile, args=("rawData.csv",))
        rawToFileThread.start()
        self.pwsh_cmd("start")
        self.pwsh_get_data()

        rawToFileThread.join()
        metadataTimestampTuple = self.findPattern("rawData.csv", self.metadataTimestamp)
        self.rawToSanitized("rawData.csv", "sanitizedData.csv", metadataTimestampTuple)
        self.sanitizedToMeasurement("sanitizedData.csv", "measurementData.csv")
        self.pwsh.close()
        while not self.acqComplete:
            time.sleep(1)

    def bytes_to_twobyte_values(self, byte_array):
        twobyte_values = []
        for i in range(0, len(byte_array), 2):
            if i + 1 < len(byte_array):
                value = (byte_array[i][0] << 8) | byte_array[i + 1][0]
                twobyte_values.append(value)
        return twobyte_values

    def convert_value(self, value):
        return (value & 4095) * (16**(0 - (value >> 12)))

    def stopAcquisition(self, acqTime):
        time.sleep(acqTime)
        self.acqComplete = True
        self.pwsh_cmd("stop")

    def rawToFile(self, filePath):
        with open(filePath, 'wb+') as file:
            while True:
                if self.dataQueue.empty() and bool(self.acqComplete):
                    file.close()
                    return
                if not self.dataQueue.empty():
                    data = self.dataQueue.get()
                    file.write(data)
                    file.flush()
                else:
                    time.sleep(0.1)

    def findPattern(self, filePath, patternTuple):
        pattern = patternTuple[0]
        patternIndexes = []
        patternLength = len(pattern)

        chunkSize = self.chunkSize

        with open(filePath, 'rb') as file:
            chunk = file.read(chunkSize)
            chunkArray = [item for item in chunk]
            prevFrag = []
            currentPos = 0
            while chunk:
                self.chunkCounter = self.chunkCounter + 1
                # add stored prevFrag to current chunk,
                # so we dont lose pattern recognition across buffers
                prevFragArray = [item for item in prevFrag]
                combChunk = prevFragArray + chunkArray
                chunkArray = [item for item in combChunk]
                for byte in range(len(chunkArray) - patternLength):
                    if chunkArray[byte:byte+patternLength] == pattern:
                        actualIndex = currentPos + byte - len(prevFrag)
                        patternIndexes.append(actualIndex)

                currentPos += len(chunk)
                prevFrag = chunk[chunkSize-(patternLength-1):]
                chunk = file.read(chunkSize)
                chunkArray = [item for item in chunk]
        return (patternIndexes, patternTuple[1])

    def rawToSanitized(self, inputFilePath, outputFilePath, patternTuple):
        indexesPerChunk = [[] for _ in range(self.chunkCounter)]
        patternLength = patternTuple[1]
        # initialize this with a value of 30 to cut out "PowerShield > ack start"
        nextLeftoverCut = 30
        currentLeftoverCut = 0
        for index in range(len(patternTuple[0])):
            # round chunkNum down
            chunkNum = int(patternTuple[0][index] // self.chunkSize)
            indexesPerChunk[chunkNum].append(patternTuple[0][index] % self.chunkSize)
        chunkSize = self.chunkSize
        with open(inputFilePath, 'rb+') as inputFile:
            with open(outputFilePath, "w+") as outputFile:
                for chunkNumber in range(self.chunkCounter):
                    currentLeftoverCut = nextLeftoverCut
                    chunk = inputFile.read(chunkSize)
                    chunkArray = [item for item in chunk]
                    # sort the indexes in reverse, to avoid index shifting when removing patterns
                    indexesToRemove = sorted(indexesPerChunk[chunkNumber], reverse=True)
                    if indexesToRemove:
                        if indexesToRemove[0] + patternLength > self.chunkSize:
                            nextLeftoverCut = patternLength - (self.chunkSize - indexesToRemove[0]) - 1
                            chunkArray = chunkArray[:indexesToRemove[0]]
                        else:
                            nextLeftoverCut = 0
                            chunkArray = chunkArray[:indexesToRemove[0]] + chunkArray[indexesToRemove[0]+patternLength:]
                        for index in range(1, len(indexesToRemove)):
                            chunkArray = chunkArray[:indexesToRemove[index]] + chunkArray[indexesToRemove[index]+patternLength:]
                    # perform the leftoverCut
                    chunkArray = chunkArray[currentLeftoverCut:]
                    nextLeftoverCut = 0
                    for item in chunkArray:
                        outputFile.write(str(item) + '\n')
        return 1

    def sanitizedToMeasurement(self, inputFilePath, outputFilePath):
        with open(inputFilePath, 'r') as inputFile:
            with open(outputFilePath, "w+") as outputFile:
                while True:
                    secondByte = inputFile.readline()
                    secondByte.rstrip("\r\n")
                    firstByte = inputFile.readline()
                    firstByte.rstrip("\r\n")

                    if not firstByte or not secondByte:
                        return

                    twoByte = (int(firstByte) << 8) | int(secondByte)
                    converted = self.convert_value(twoByte)

                    outputFile.write(str(converted) + '\n')

    def read_lines_from_string(self, input_string):
        lines = input_string.split("\r\n")
        return lines

    def pwsh_cmd(self, cmd):
        print("[CMD]", cmd, flush=True)
        self.pwsh.write((cmd + "\n").encode('ascii'))

    def pwsh_get_timeout(self, timeout):
        buff = []
        start_time = time.time()
        while True:
            x = self.pwsh.read()
            buff.append(x)
            if time.time() - start_time >= float(timeout):
                return buff[29:]

    def pwsh_get(self):
        s = ""
        while True:
            x = self.pwsh.read()
            if len(x) < 1:
                return s.replace("\0", "").strip().replace("\r", "").replace("\n\n\n", "\n")
            s += str(x, encoding='ascii', errors='ignore')

    def pwsh_get_data(self):
        while True:
            x = self.pwsh.read(1)
            if len(x) < 1:
                return
            if self.acqComplete:
                return
            self.dataQueue.put(x)

    def pwsh_get_ok(self):
        r = ""
        count = 0
        while True:
            self.pwsh_cmd("status")
            s = self.pwsh_get()
            if s != "":
                print(s, flush=True)
            else:
                count += 1
                if count > 5:
                    return -1
            r += s
            if r.find("status ok") != -1:
                return r
            time.sleep(1)
