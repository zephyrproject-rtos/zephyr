#!/bin/sh

HEX_NAME=${O}/${KERNEL_HEX_NAME}

command -v nrfjprog >/dev/null 2>&1 || { echo >&2 "Can't flash nRF board,"\
						  "nrfjprog is not installed."\
						  "Aborting."; exit 1; }

CONNECTED_BOARDS=`nrfjprog --ids`
read -s -a BOARD_LIST <<< $CONNECTED_BOARDS

BOARDS_NUM=`echo "$CONNECTED_BOARDS" | wc -l`
if [ $BOARDS_NUM == 1 ]
then
	BOARD_SNR=$BOARD_LIST
else
	echo "There are multiple boards connected."
	for i in $(seq 1 1 $BOARDS_NUM)
	do
		echo $i. ${BOARD_LIST[$i - 1]}
	done

	prompt="Please select one with desired serial number (1-$BOARDS_NUM):"
	while true; do
		read -p "$prompt" ANS
		if [ 1 -le $ANS -a $ANS -le $BOARDS_NUM ]
		then
			break;
		else
			echo -n
		fi
	done

	BOARD_SNR=${BOARD_LIST[$ANS - 1]}
fi

echo "Flashing file: "${HEX_NAME}

nrfjprog --eraseall -f $NRF_FAMILY --snr $BOARD_SNR &&
nrfjprog --program $HEX_NAME -f $NRF_FAMILY --snr $BOARD_SNR &&
if [ $NRF_FAMILY == NRF52 ]
then
	# Set reset pin
	nrfjprog --memwr 0x10001200 --val 0x00000015 \
		 -f $NRF_FAMILY --snr $BOARD_SNR
	nrfjprog --memwr 0x10001204 --val 0x00000015 \
		 -f $NRF_FAMILY --snr $BOARD_SNR
	nrfjprog --reset -f $NRF_FAMILY --snr $BOARD_SNR
fi
nrfjprog --pinreset -f $NRF_FAMILY --snr $BOARD_SNR

if [ $? -eq 0 ]; then
    echo "${BOARD} Serial Number $BOARD_SNR flashed with success."
else
    echo "Flashing ${BOARD} failed."
    exit 2;
fi

