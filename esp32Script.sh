#!/bin/bash

echo "Start Script"
stty -F /dev/ttyESP32 cs8 115200 ignbrk -hupcl -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
echo "TP-Link_DDA8" > /dev/ttyESP32
sleep 1
echo "Xtd6YU42cgfL" > /dev/ttyESP32
echo "Script Ran Successfully"
$SHELL