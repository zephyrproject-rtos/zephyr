package main

// to run this, type: go run . <serial port> in this directory

import (
	"encoding/binary"
	"log"
	"os"
	"time"

	"go.bug.st/serial"
)

func main() {
	if len(os.Args) < 2 {
		log.Fatal("Usage: usb-cdc-perf <serial port>")
	}

	mode := &serial.Mode{
		BaudRate: 115200,
	}

	port, err := serial.Open(os.Args[1], mode)
	if err != nil {
		log.Fatal("Error opening port: ", err)
	}

	rb := make([]byte, 1024)
	var value uint32 = 0
	blockCount := 0
	lastPerfCalc := time.Now()
	perfBytes := 0
	errorCnt := 0
	leftCnt := 0

	for {
		c, err := port.Read(rb)
		if err != nil {
			log.Fatal("Error reading serial port: ", err)
		}
		if c > 0 {
			perfBytes += c

			for i := 0; i < c; i += 4 {
				rxValue := binary.LittleEndian.Uint32(rb[i : i+4])
				if value != rxValue {
					log.Printf("bad data, exp: 0x%x, got 0x%x\n", value, rxValue)
					errorCnt++
					value = rxValue + 1
				} else {
					value++
				}
			}
		}

		blockCount++
		now := time.Now()
		diff := now.Sub(lastPerfCalc)
		if diff > 950*time.Millisecond {
			rate := (float64(perfBytes) / diff.Seconds()) * 8
			modifier := ""
			if rate > 1000*1000 {
				rate /= 1000 * 1000
				modifier = "M"
			} else if rate > 1000 {
				rate /= 1000
				modifier = "K"
			}
			log.Printf("Read %v bytes, Data rate: %.2f %vb/sec, error cnt: %v, leftover cnt: %v\n",
				perfBytes, rate, modifier, errorCnt, leftCnt)
			lastPerfCalc = now
			perfBytes = 0
		}
	}
}
