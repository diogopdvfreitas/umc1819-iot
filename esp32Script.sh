#!/bin/bash

echo "Script Start"
stty -F /dev/ttyESP32 cs8 115200 ignbrk -hupcl -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
echo "tk3" > /dev/ttyESP32
sleep 1
echo "#teamdarmstadt" > /dev/ttyESP32
echo "Script Ran Successfully"
$SHELL