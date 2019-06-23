#!/bin/bash

sleep 3
echo "Script Start"
stty -F /dev/ttyESP32 cs8 115200 ignbrk -hupcl -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
#echo "blah1234" > /dev/ttyESP32
python3 /home/pi/tk3/readNFC.py
sleep 2
#echo "blah1234" > /dev/ttyESP32
echo "Script Ran Successfully"
$SHELL