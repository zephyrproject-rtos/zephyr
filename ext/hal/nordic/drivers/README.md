# nRF 802.15.4 radio driver.

This driver implements only __non-beacon mode__ of operation.
It supports following __features__:
 * reception of unicast and broadcast frames (with filtering),
 * automatic sending ACK frames,
 * setting pending bit in ACK frame according to pending data for given 
   destination,
 * transmission of unicast and broadcast frames,
 * automatic CCA procedure before transmission,
 * automatic receiving ACK frames,
 * low power mode (sleep),
 * energy detection,
 * promiscuous mode.

## Implementation details

The driver is a FSM. From API perspective it has 4 states. Most of those states
contains sub-states in implementation.

### FSM description

```
          receive()       transmit()
          -------->       -------->
     Sleep         Receive         Transmit
          <-------- |  /|\<--------
           sleep()  |   |   receive() / transmitted() / busy_channel()
                    |   |
 energy_detection() |   | energy_detected()
                   \|/  |
               Energy detection
```

#### Transitions

The driver is initialized in the Sleep state. The higher layer should call 
the receive() function to make the driver enter the Receive state and start 
radio operations.

In basic applications radio should be most time in a Receive state. In this 
state the radio receives incoming frames. Changing to any other state should be
performed from Receive state.

When a frame is received in Receive state the driver notifies the higher layer 
by calling received() function. This function is called after reception of a 
broadcast frame or after sending an ACK to a unicast frame.
In the promiscuous mode the higher layer is notified about all of the received
frames. Even if the frame was not destined to the receiving node.

To transmit a frame the higher layer should call the transmit() function. If 
channel is busy the driver goes back to the Receive state and notifies the 
higher layer by calling the busy_channel() function. If a broadcast frame was 
transmitted the driver goes back to the Receive state and notifies the higher 
layer by calling the transmitted() function. If a unicast frame was transmitted 
and an ACK was received the driver goes back to the Receive state and notifies 
the higher layer by calling the transmitted() function. If a unicast frame was 
transmitted and there was no expected ACK received the higher layer shall call 
the receive() function after the ACK timeout to make the driver go back to the
Receive state.

To perform an Energy Detection procedure the higher layer should call the
energy_detection() function. When the procedure is completed the driver goes
automatically back to the Receive state and notifies the higher layer with the
energy_detected() function.

#### States

##### Sleep
In this state the radio is in low power mode. It cannot transmit or receive any
frame.

##### Receive
In this state the radio receives 802.15.4 frames. It filters out frames with
invalid CRC, length, type, destination address. 
If the driver receives unicast frame destined to the receiving node it 
automatically transmits an ACK frame. According to 802.15.4 standard, an ACK
frame should be delayed aTurnaroundTime (192 uS) after reception of the ACKed
frame. To perform this delay the driver uses the TIFS timer in the radio
peripheral. This timer requires 3 shorts to work correctly:
1. END -> DISABLE
2. DISABLE -> TXEN
3. READY -> START
The driver has limited time after receiving of a frame to decide if an ACK 
should be transmitted. If ACK should not be transmitted the driver must abort
sending ACK by disabling those shorts and triggering DISABLE task.
To use this limited time most effective the driver uses the Bit Counter feature
of the radio peripheral to get notification when the Frame Control field is 
received and when the destination address is received. Those fields are used to
filter the frame before whole frame is received.
If all 3 shorts used to send ACK automatically are enabled the radio peripheral
sends ACK frames in loop. To prevent this during debugging process there are 
only 2 shorts enabled when waiting for frame (1. and 2.) and 2 other shorts are
enabled in DISABLED event handler (1. and 3.). The first short is still enabled
to automatically disable transmitter after transmission of the ACK frame.

##### Transmit
In this state the radio performs the CCA procedure. If channel is free the radio 
transmits requested frame. If an ACK was requested in the transmitted frame
the driver automatically receives the ACK frame in this state.
To prevent the TXIDLE peripheral state the driver uses 2 shorts in Transmit
state:
1. READY -> START
2. END -> DISABLE
Those shorts automatically start transmission of the frame when transmitter is
ready and disable transmitter when the frame was transmitted.

##### Energy detection
In this state the radio performs the Energy Detection procedure. During this
procedure the radio is busy and cannot change state to any other. The end of
this procedure is notified to the higher layer by a function call.

### Mutex and critical sections.

State transitions in the FSM can be requested simultaneously by the higher layer
and the IRQ handler. To prevent race conditions in the driver there is a mutex.
The mutex is unlocked only in the *Receive* state (*WaitingRxFrame* substate). 
If there is requested state transition, the procedure shall lock the mutex 
before state is changed. If mutex cannot be locked, another procedure has locked
it and is going to change the state.
The mutex is unlocked when the driver enters *Receive* state.

A race condition could also occur during handle of a requests from the higher 
layer. Even if the receiver is stopped (TASK STOP) the END or DISABLED event can 
be raised for a few uS after triggering the task. To prevent interrupt of the
higher layer request handler by IRQ handler, the higher layer request handlers
are performend in critical sections. The critical sections are implemented as
software interrupt requests with priority equal to the RADIO IRQ.
