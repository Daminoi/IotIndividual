#  _      _______ ____  _____ _____
# | |    |  ____/ ____|/ ____|_   _|
# | |    | |__ | |  __| |  __  | |  
# | |    |  __|| | |_ | | |_ | | |  
# | |____| |___| |__| | |__| |_| |_ 
# |______|______\_____|\_____|_____|
#
# venv IOT_project
# VA RIATTIVATO IL VENV OGNI VOLTA!
# attiva il venv così: 
# source IOT_project/bin/activate
#
# Su un'altra console avvia mosquitto:
# sudo systemctl restart mosquitto
# 
# To see messages received by the broker on the laptop (testing):
# mosquitto_sub -h localhost -t "#" -v
#
# Per avviare questo script:
# cd /home/damino/Documenti/PlatformIO/Projects/IotIndividual/
# python3 rttTesterHelper.py

import paho.mqtt.client as mqtt
import time

BROKER = "localhost"
TOPIC_PING = "rttTesting/ping"
TOPIC_PONG = "rttTesting/pong"

def on_connect(client, userdata, flags, rc):
    print("Connected to the local broker")
    client.subscribe(TOPIC_PING)

def on_message(client, userdata, msg):
    if msg.topic == TOPIC_PING:
        client.publish(TOPIC_PONG, 67)
        print(f"Ping received, pong sent: [{time.strftime('%H:%M:%S')}]")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, 1883, 60)
client.loop_forever()