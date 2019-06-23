#!/usr/bin/env python
# -*- coding: utf8 -*-

import ndef
import time
import serial
import RPi.GPIO as GPIO
from mfrc522 import MFRC522

ONE_DIG = [ '0', '1', '2', '3',
            '4', '5', '6', '7', 
            '8', '9', 'a', 'b', 
            'c', 'd', 'e', 'f' ]

MIFARE_CLASSIC_1K_KEYS = [  [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF],
                            [0XD3, 0XF7, 0XD3, 0XF7, 0XD3, 0XF7] ]

cont = True
reader = MFRC522()
tokenData = ''

while cont:
    # Only start if NFC token is found
    (status, TagType) = reader.MFRC522_Request(reader.PICC_REQIDL)
    if status == reader.MI_OK:
        for sector in range(4, 16):
            print("Sector " + str(sector))
            for key in MIFARE_CLASSIC_1K_KEYS:
                # Scan for NFC tokens
                (status, TagType) = reader.MFRC522_Request(reader.PICC_REQIDL)
                # If a token is found, get the UID
                (status, uid) = reader.MFRC522_Anticoll()
                # If we have the UID, continue
                if status == reader.MI_OK:
                    # Select the scanned tag
                    reader.MFRC522_SelectTag(uid)
                    # Authenticate
                    status = reader.MFRC522_Auth(reader.PICC_AUTHENT1A, sector, key, uid)
                    # Check if authenticated
                    if status == reader.MI_OK:
                        data = reader.MFRC522_Read(sector)
                        # Print sector data
                        print("\tData: " + str(data))                    
                        conv = ''
                        # Convert data to text
                        # Data Sector 7 is trash mixed in
                        if sector != 7:
                            # Data Sector 4 should only start at the 4th byte
                            if sector == 4:
                                for d in range (4, 16):
                                    temp = hex(data[d]).split('x')[1]
                                    if temp in ONE_DIG:
                                        temp = '0' + temp
                                    conv += temp                            
                            else:    
                                for d in data:
                                    temp = hex(d).split('x')[1]
                                    if temp in ONE_DIG:
                                        temp = '0' + temp
                                    conv += temp
                        tokenData += conv
                        print("\tConv: " + conv)
                        print("\tKey: " + str(key))
                        reader.MFRC522_StopCrypto1()
        cont = False

# Decoding bytes to NDEF Record
octets = bytearray.fromhex(tokenData)
tokenRecords = ndef.message_decoder(octets)
wifiConfigRecord = list(tokenRecords)[0]

# Extracting Credentials from Record
wifiCredential = wifiConfigRecord.get_attribute('credential', 0)
ssid = wifiCredential.get_attribute('ssid').value
pwrd = wifiCredential.get_attribute('network-key').value

# Send credentials through serial
serConn = serial.Serial('/dev/ttyESP32', 115200)
serConn.write(ssid)
time.sleep(3)
serConn.write(pwrd)

serConn.close()
GPIO.cleanup()