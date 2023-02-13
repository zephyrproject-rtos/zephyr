Title: Microkernel Object Performance

Description:

AppKernel is used to measure the performance of microkernel events, mutexes,
semaphores, FIFOs, mailboxes, pipes, memory maps, and memory pools.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console. It can be built and executed
on QEMU as follows:

    make run

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

|-----------------------------------------------------------------------------|
|          S I M P L E   S E R V I C E    M E A S U R E M E N T S  |  nsec    |
|-----------------------------------------------------------------------------|
|-----------------------------------------------------------------------------|
| enqueue 1 byte msg in FIFO                                       |    NNNNNN|
| dequeue 1 byte msg in FIFO                                       |    NNNNNN|
| enqueue 4 bytes msg in FIFO                                      |    NNNNNN|
| dequeue 4 bytes msg in FIFO                                      |    NNNNNN|
| enqueue 1 byte msg in FIFO to a waiting higher priority task     |    NNNNNN|
| enqueue 4 bytes in FIFO to a waiting higher priority task        |    NNNNNN|
|-----------------------------------------------------------------------------|
| signal semaphore                                                 |    NNNNNN|
| signal to waiting high pri task                                  |    NNNNNN|
| signal to waiting high pri task, with timeout                    |    NNNNNN|
| signal to waitm (2)                                              |    NNNNNN|
| signal to waitm (2), with timeout                                |    NNNNNN|
| signal to waitm (3)                                              |    NNNNNN|
| signal to waitm (3), with timeout                                |   NNNNNNN|
| signal to waitm (4)                                              |   NNNNNNN|
| signal to waitm (4), with timeout                                |   NNNNNNN|
|-----------------------------------------------------------------------------|
| average lock and unlock mutex                                    |    NNNNNN|
|-----------------------------------------------------------------------------|
| average alloc and dealloc memory page                            |    NNNNNN|
|-----------------------------------------------------------------------------|
| average alloc and dealloc memory pool block                      |    NNNNNN|
|-----------------------------------------------------------------------------|
| Signal enabled event                                             |    NNNNNN|
| Signal event & Test event                                        |    NNNNNN|
| Signal event & TestW event                                       |    NNNNNN|
| Signal event with installed handler                                         |
|    Handler responds OK                                                      |
|-----------------------------------------------------------------------------|
|                M A I L B O X   M E A S U R E M E N T S                      |
|-----------------------------------------------------------------------------|
| Send mailbox message to waiting high priority task and wait                 |
| repeat for  128 times and take the average                                  |
|-----------------------------------------------------------------------------|
|   size(B) |       time/packet (nsec)       |          KB/sec                |
|-----------------------------------------------------------------------------|
|          N|                          NNNNNN|                               N|
|          N|                          NNNNNN|                              NN|
|         NN|                          NNNNNN|                              NN|
|         NN|                          NNNNNN|                              NN|
|         NN|                          NNNNNN|                              NN|
|        NNN|                          NNNNNN|                             NNN|
|        NNN|                          NNNNNN|                             NNN|
|        NNN|                          NNNNNN|                             NNN|
|       NNNN|                         NNNNNNN|                             NNN|
|       NNNN|                         NNNNNNN|                            NNNN|
|       NNNN|                         NNNNNNN|                            NNNN|
|       NNNN|                         NNNNNNN|                            NNNN|
|-----------------------------------------------------------------------------|
| message overhead:      NNNNNN     nsec/packet                               |
| raw transfer rate:           NNNN KB/sec (without overhead)                 |
|-----------------------------------------------------------------------------|
|                   P I P E   M E A S U R E M E N T S                         |
|-----------------------------------------------------------------------------|
| Send data into a pipe towards a receiving high priority task and wait       |
|-----------------------------------------------------------------------------|
|                          matching sizes (_ALL_N)                            |
|-----------------------------------------------------------------------------|
|   size(B) |       time/packet (nsec)       |          KB/sec                |
|-----------------------------------------------------------------------------|
| put | get |  no buf  | small buf| big buf  |  no buf  | small buf| big buf  |
|-----------------------------------------------------------------------------|
|    N|    N|   NNNNNNN|   NNNNNNN|   NNNNNNN|         N|         N|         N|
|   NN|   NN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|   NN|   NN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|   NN|   NN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|       NNN|       NNN|       NNN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|       NNN|       NNN|       NNN|
| NNNN| NNNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|       NNN|       NNN|       NNN|
| NNNN| NNNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|       NNN|       NNN|       NNN|
| NNNN| NNNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|      NNNN|      NNNN|      NNNN|
|-----------------------------------------------------------------------------|
|                      non-matching sizes (1_TO_N) to higher priority         |
|-----------------------------------------------------------------------------|
|   size(B) |       time/packet (nsec)       |          KB/sec                |
|-----------------------------------------------------------------------------|
| put | get |  no buf  | small buf| big buf  |  no buf  | small buf| big buf  |
|-----------------------------------------------------------------------------|
|    N| NNNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|         N|         N|         N|
|   NN| NNNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|   NN| NNNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|   NN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
|  NNN|   NN|  NNNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|        NN|
| NNNN|   NN|  NNNNNNNN|  NNNNNNNN|  NNNNNNNN|        NN|        NN|        NN|
| NNNN|   NN| NNNNNNNNN| NNNNNNNNN| NNNNNNNNN|        NN|        NN|        NN|
| NNNN|    N| NNNNNNNNN| NNNNNNNNN| NNNNNNNNN|         N|         N|         N|
|-----------------------------------------------------------------------------|
|                      non-matching sizes (1_TO_N) to lower priority          |
|-----------------------------------------------------------------------------|
|   size(B) |       time/packet (nsec)       |          KB/sec                |
|-----------------------------------------------------------------------------|
| put | get |  no buf  | small buf| big buf  |  no buf  | small buf| big buf  |
|-----------------------------------------------------------------------------|
|    N| NNNN|   NNNNNNN|    NNNNNN|    NNNNNN|         N|         N|         N|
|   NN| NNNN|   NNNNNNN|   NNNNNNN|    NNNNNN|        NN|        NN|        NN|
|   NN| NNNN|   NNNNNNN|   NNNNNNN|    NNNNNN|        NN|        NN|        NN|
|   NN|  NNN|   NNNNNNN|   NNNNNNN|    NNNNNN|        NN|        NN|        NN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|       NNN|
|  NNN|  NNN|   NNNNNNN|   NNNNNNN|   NNNNNNN|        NN|        NN|       NNN|
|  NNN|   NN|  NNNNNNNN|  NNNNNNNN|   NNNNNNN|        NN|        NN|       NNN|
| NNNN|   NN|  NNNNNNNN|  NNNNNNNN|   NNNNNNN|        NN|        NN|       NNN|
| NNNN|   NN| NNNNNNNNN| NNNNNNNNN|   NNNNNNN|        NN|         N|       NNN|
| NNNN|    N| NNNNNNNNN|NNNNNNNNNN|   NNNNNNN|         N|         N|      NNNN|
|-----------------------------------------------------------------------------|
|         END OF TESTS                                                        |
|-----------------------------------------------------------------------------|
PROJECT EXECUTION SUCCESSFUL
