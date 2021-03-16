#!/bin/bash -e

if [ ! $1 ]; then
    BOARDS='stm32f103_mini'
elif [ "$1" = "-k" ]; then
    BOARDS='stm32f103_mini'
else
    BOARDS=$1
fi;
if [ ! $2 ]; then
        PROJECT="./samples/MyselfProject/SnakeGame/"
else
        PROJECT=$2
fi;

west build -b ${BOARDS}  ${PROJECT} -p auto


# west build -t menuconfig
# 或者
# west build -t guiconfig