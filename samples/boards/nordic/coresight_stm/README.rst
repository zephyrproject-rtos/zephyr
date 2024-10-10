.. _nrf-coresight-stm-sample:

Coresight STM demo
##################

Overview
********

This sample presents how to enable STM logging on nRF54H20 platform.
Also, it prints timing for different log messages.
Thus, performance of different loggers can be compared.

Sample has three configurations:
1. 'sample.boards.nrf.coresight_stm.local_uart'
This configuration doesn't use STM. Logs are printed on local console.
#. 'sample.boards.nrf.coresight_stm'
This configuration use STM. Application, Radio, PPR and FLPR cores send logs to an ETR buffer.
Proxy (Application core) gets logs from the ETR buffer, decodes STPv2 data
and prints human readable logs on UART.
#. 'sample.boards.nrf.coresight_stm.dict'
This sample uses STM logging in dictionary mode. Application, Radio, PPR and FLPR cores
send logs to the ETR buffer. Proxy (Application core) forwards data from the ETR to
the host using UART. Host tool is needed to decode the logs.

Requirements
************

This application uses nRF54H20 DK board for the demo.

Building and running
********************

To build the sample, use configuration setups from :file:`sample.yaml` using the ``-T`` option.
See the following examples:

nRF54H20 DK
  You can build the test for application and radio cores as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.boards.nrf.coresight_stm .

Sample Output
=============

.. code-block:: console

   *** Using Zephyr OS v3.6.99-5bb7bb0af17c ***
   [00:00:00.227,264] <inf> app/app: test no arguments
   [00:00:00.227,265] <inf> app/app: test no arguments
   [00:00:00.227,267] <inf> app/app: test no arguments
   [00:00:00.227,268] <inf> app/app: test no arguments
   [00:00:00.227,270] <inf> app/app: test no arguments
   [00:00:00.227,272] <inf> app/app: test no arguments
   [00:00:00.227,273] <inf> app/app: test no arguments
   [00:00:00.227,275] <inf> app/app: test no arguments
   [00:00:00.227,276] <inf> app/app: test no arguments
   [00:00:00.227,278] <inf> app/app: test no arguments
   [00:00:00.585,558] <inf> rad/app: test with one argument 100
   [00:00:00.585,569] <inf> rad/app: test with one argument 100
   [00:00:00.585,574] <inf> rad/app: test with one argument 100
   [00:00:00.585,579] <inf> rad/app: test with one argument 100
   [00:00:00.585,584] <inf> rad/app: test with one argument 100
   [00:00:00.585,588] <inf> rad/app: test with one argument 100
   [00:00:00.585,593] <inf> rad/app: test with one argument 100
   [00:00:00.585,598] <inf> rad/app: test with one argument 100
   [00:00:00.585,603] <inf> rad/app: test with one argument 100
   [00:00:00.585,608] <inf> rad/app: test with one argument 100
   [00:00:00.624,408] <inf> ppr/app: test with one argument 100
   [00:00:00.624,433] <inf> ppr/app: test with one argument 100
   [00:00:00.624,459] <inf> ppr/app: test with one argument 100
   [00:00:00.624,486] <inf> ppr/app: test with one argument 100
   [00:00:00.624,512] <inf> ppr/app: test with one argument 100
   [00:00:00.624,537] <inf> ppr/app: test with one argument 100
   [00:00:00.624,564] <inf> ppr/app: test with one argument 100
   [00:00:00.624,590] <inf> ppr/app: test with one argument 100
   [00:00:00.624,616] <inf> ppr/app: test with one argument 100
   [00:00:00.624,643] <inf> ppr/app: test with one argument 100
   [00:00:00.625,249] <inf> flpr/app: test with one argument 100
   [00:00:00.625,251] <inf> flpr/app: test with one argument 100
   [00:00:00.625,252] <inf> flpr/app: test with one argument 100
   [00:00:00.625,254] <inf> flpr/app: test with one argument 100
   [00:00:00.625,254] <inf> flpr/app: test with one argument 100
   [00:00:00.625,256] <inf> flpr/app: test with one argument 100
   [00:00:00.625,257] <inf> flpr/app: test with one argument 100
   [00:00:00.625,259] <inf> flpr/app: test with one argument 100
   [00:00:00.625,259] <inf> flpr/app: test with one argument 100
   [00:00:00.625,260] <inf> flpr/app: test with one argument 100
   [00:00:00.626,025] <inf> app/app: test with one argument 100
   [00:00:00.626,028] <inf> app/app: test with one argument 100
   [00:00:00.626,030] <inf> app/app: test with one argument 100
   [00:00:00.626,032] <inf> app/app: test with one argument 100
   [00:00:00.626,033] <inf> app/app: test with one argument 100
   [00:00:00.626,035] <inf> app/app: test with one argument 100
   [00:00:00.626,036] <inf> app/app: test with one argument 100
   [00:00:00.626,038] <inf> app/app: test with one argument 100
   [00:00:00.626,040] <inf> app/app: test with one argument 100
   [00:00:00.626,041] <inf> app/app: test with one argument 100
   [00:00:00.984,403] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,412] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,417] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,422] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,427] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,432] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,438] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,443] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,448] <inf> rad/app: test with two arguments 100 10
   [00:00:00.984,452] <inf> rad/app: test with two arguments 100 10
   [00:00:01.024,032] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,033] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,035] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,036] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,036] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,038] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,040] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,041] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,043] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,043] <inf> flpr/app: test with two arguments 100 10
   [00:00:01.024,153] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,180] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,208] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,235] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,260] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,288] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,315] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,342] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,368] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,395] <inf> ppr/app: test with two arguments 100 10
   [00:00:01.024,841] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,843] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,844] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,848] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,849] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,851] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,852] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,854] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,856] <inf> app/app: test with two arguments 100 10
   [00:00:01.024,857] <inf> app/app: test with two arguments 100 10
   [00:00:01.383,230] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,240] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,246] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,251] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,257] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,262] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,267] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,273] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,278] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.383,283] <inf> rad/app: test with three arguments 100 10 1
   [00:00:01.422,747] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,747] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,748] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,750] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,752] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,753] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,753] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,755] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,756] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.422,758] <inf> flpr/app: test with three arguments 100 10 1
   [00:00:01.423,585] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,588] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,590] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,592] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,593] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,595] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,596] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,600] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,601] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,603] <inf> app/app: test with three arguments 100 10 1
   [00:00:01.423,832] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.423,860] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.423,888] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.423,915] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.423,942] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.423,969] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.423,996] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.424,024] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.424,051] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.424,078] <inf> ppr/app: test with three arguments 100 10 1
   [00:00:01.781,960] <inf> rad/app: test with string test string
   [00:00:01.781,972] <inf> rad/app: test with string test string
   [00:00:01.781,977] <inf> rad/app: test with string test string
   [00:00:01.781,984] <inf> rad/app: test with string test string
   [00:00:01.781,990] <inf> rad/app: test with string test string
   [00:00:01.781,996] <inf> rad/app: test with string test string
   [00:00:01.782,001] <inf> rad/app: test with string test string
   [00:00:01.782,008] <inf> rad/app: test with string test string
   [00:00:01.782,014] <inf> rad/app: test with string test string
   [00:00:01.782,019] <inf> rad/app: test with string test string
   [00:00:01.821,363] <inf> flpr/app: test with string test string
   [00:00:01.821,366] <inf> flpr/app: test with string test string
   [00:00:01.821,368] <inf> flpr/app: test with string test string
   [00:00:01.821,371] <inf> flpr/app: test with string test string
   [00:00:01.821,374] <inf> flpr/app: test with string test string
   [00:00:01.821,377] <inf> flpr/app: test with string test string
   [00:00:01.821,380] <inf> flpr/app: test with string test string
   [00:00:01.821,384] <inf> flpr/app: test with string test string
   [00:00:01.821,385] <inf> flpr/app: test with string test string
   [00:00:01.821,388] <inf> flpr/app: test with string test string
   [00:00:01.822,235] <inf> app/app: test with string test string
   [00:00:01.822,243] <inf> app/app: test with string test string
   [00:00:01.822,246] <inf> app/app: test with string test string
   [00:00:01.822,251] <inf> app/app: test with string test string
   [00:00:01.822,254] <inf> app/app: test with string test string
   [00:00:01.822,257] <inf> app/app: test with string test string
   [00:00:01.822,260] <inf> app/app: test with string test string
   [00:00:01.822,265] <inf> app/app: test with string test string
   [00:00:01.822,268] <inf> app/app: test with string test string
   [00:00:01.822,272] <inf> app/app: test with string test string
   [00:00:01.823,420] <inf> ppr/app: test with string test string
   [00:00:01.823,486] <inf> ppr/app: test with string test string
   [00:00:01.823,550] <inf> ppr/app: test with string test string
   [00:00:01.823,614] <inf> ppr/app: test with string test string
   [00:00:01.823,680] <inf> ppr/app: test with string test string
   [00:00:01.823,744] <inf> ppr/app: test with string test string
   [00:00:01.823,808] <inf> ppr/app: test with string test string
   [00:00:01.823,873] <inf> ppr/app: test with string test string
   [00:00:01.823,937] <inf> ppr/app: test with string test string
   [00:00:01.824,001] <inf> ppr/app: test with string test string
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,627] rad/tp: 5
   [00:00:02.180,628] rad/tp: 5
   [00:00:02.180,628] rad/tp: 5
   [00:00:02.180,628] rad/tp: 5
   [00:00:02.180,628] rad/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.219,971] flpr/tp: 5
   [00:00:02.220,870] app/tp: 5
   [00:00:02.220,870] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.220,872] app/tp: 5
   [00:00:02.222,940] ppr/tp: 5
   [00:00:02.222,940] ppr/tp: 5
   [00:00:02.222,940] ppr/tp: 5
   [00:00:02.222,940] ppr/tp: 5
   [00:00:02.222,942] ppr/tp: 5
   [00:00:02.222,942] ppr/tp: 5
   [00:00:02.222,942] ppr/tp: 5
   [00:00:02.222,942] ppr/tp: 5
   [00:00:02.222,942] ppr/tp: 5
   [00:00:02.222,944] ppr/tp: 5
   [00:00:02.222,944] ppr/tp: 5
   [00:00:02.222,944] ppr/tp: 5
   [00:00:02.222,944] ppr/tp: 5
   [00:00:02.222,944] ppr/tp: 5
   [00:00:02.222,945] ppr/tp: 5
   [00:00:02.222,945] ppr/tp: 5
   [00:00:02.222,945] ppr/tp: 5
   [00:00:02.222,945] ppr/tp: 5
   [00:00:02.222,945] ppr/tp: 5
   [00:00:02.222,947] ppr/tp: 5
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,160] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.579,161] rad/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.618,481] flpr/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,382] app/tp: 6 0000000a
   [00:00:02.619,384] app/tp: 6 0000000a
   [00:00:02.619,384] app/tp: 6 0000000a
   [00:00:02.619,384] app/tp: 6 0000000a
   [00:00:02.619,384] app/tp: 6 0000000a
   [00:00:02.622,417] ppr/tp: 6 0000000a
   [00:00:02.622,417] ppr/tp: 6 0000000a
   [00:00:02.622,417] ppr/tp: 6 0000000a
   [00:00:02.622,417] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,419] ppr/tp: 6 0000000a
   [00:00:02.622,420] ppr/tp: 6 0000000a
   [00:00:02.622,420] ppr/tp: 6 0000000a
   [00:00:02.622,420] ppr/tp: 6 0000000a
   [00:00:02.622,420] ppr/tp: 6 0000000a
   [00:00:02.622,420] ppr/tp: 6 0000000a
   [00:00:02.622,420] ppr/tp: 6 0000000a
   [00:00:02.622,422] ppr/tp: 6 0000000a
   [00:00:02.622,422] ppr/tp: 6 0000000a
   [00:00:02.622,422] ppr/tp: 6 0000000a
   rad: Timing for log message with 0 arguments: 5.10us
   rad: Timing for log message with 1 argument: 6.10us
   rad: Timing for log message with 2 arguments: 6.0us
   rad: Timing for log message with 3 arguments: 6.40us
   rad: Timing for log_message with string: 7.10us
   rad: Timing for tracepoint: 0.5us
   rad: Timing for tracepoint_d32: 0.5us
   flpr: Timing for log message with 0 arguments: 1.20us
   flpr: Timing for log message with 1 argument: 1.20us
   flpr: Timing for log message with 2 arguments: 1.20us
   flpr: Timing for log message with 3 arguments: 1.30us
   flpr: Timing for log_message with string: 3.0us
   flpr: Timing for tracepoint: 0.0us
   flpr: Timing for tracepoint_d32: 0.0us
   app: Timing for log message with 0 arguments: 1.80us
   app: Timing for log message with 1 argument: 2.0us
   app: Timing for log message with 2 arguments: 2.0us
   app: Timing for log message with 3 arguments: 2.10us
   app: Timing for log_message with string: 4.40us
   app: Timing for tracepoint: 0.10us
   app: Timing for tracepoint_d32: 0.10us
   ppr: Timing for log message with 0 arguments: 25.20us
   ppr: Timing for log message with 1 argument: 26.20us
   ppr: Timing for log message with 2 arguments: 26.90us
   ppr: Timing for log message with 3 arguments: 27.40us
   ppr: Timing for log_message with string: 64.80us
   ppr: Timing for tracepoint: 0.30us
   ppr: Timing for tracepoint_d32: 0.25us

See OS Services » Logging » Multi-domain logging using ARM Coresight STM` for details.
