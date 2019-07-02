#!/usr/bin/env python
# -*- coding: utf8 -*-

import time
import socket
import paho.mqtt.client as mqttClient
from zeroconf import *

### CONNECTION VARIABLES & MDNS LISTENER CLASS ###

mqttAddress = ""
mqttPort = 0
mqttUser = "tk3_brok"
mqttPass = "brokmqtt"

class MyListener:
	def __init__(self):
		self.zc = Zeroconf()

	def remove_service(self, zeroconf, type, name):
		print("Service %s removed" % (name,))

	def add_service(self, zeroconf, type, name):
		info = self.zc.get_service_info(type, name)
		mqttAddress = socket.inet_ntoa(info.addresses[0])
		mqttPort = info.port
		print(mqttAddress, mqttPort)

### MQTT CALLBACK METHODS ###

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected                #Use global variable
        Connected = True                #Signal connection
    else:
        print("Connection failed")

def on_message(client, userdata, message):
    print("Message received: " + message.payload)

### MQTT CONNECTION ###

zc = Zeroconf()
listener = MyListener()
browser = ServiceBrowser(zc, "_mqtt._tcp.local.", listener)
time.sleep(1)
zc.close()

client = mqttClient.Client("tk3python")
client.username_pw_set(mqttUser, password=mqttPass)
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqttAddress, mqttPort)
client.loop_start()

while Connected != True:
    time.sleep(0.1)

    