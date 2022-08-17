# SPDX-FileCopyrightText: 2018 Brent Rubell for Adafruit Industries
#
# SPDX-License-Identifier: MIT

"""
Example for using the RFM9x Radio with Raspberry Pi.

Learn Guide: https://learn.adafruit.com/lora-and-lorawan-for-raspberry-pi
Author: Brent Rubell for Adafruit Industries
"""
# Import Python System Libraries
import time
# Import Blinka Libraries
import busio
from digitalio import DigitalInOut, Direction, Pull
import board
# Import the SSD1306 module.
import adafruit_ssd1306
# Import RFM9x
import adafruit_rfm9x
import aprslib
import logging
import json
import paho.mqtt.client as mqtt
import base91


broker = "localhost"
mqttPort = 1883
ka_interval = 45
topic = "brc/trackers"

# logging.basicConfig(level=logging.DEBUG) # enable this to see APRS parser debugging

rxName = "HOUSE"
mqttUser = "john"
mqttPass = "password5678"


client=mqtt.Client(rxName)
client.username_pw_set(mqttUser,mqttPass)
client.connect(broker, port=mqttPort, keepalive=ka_interval)
client.loop_start()

# Create the I2C interface.
i2c = busio.I2C(board.SCL, board.SDA)

# 128x32 OLED Display
reset_pin = DigitalInOut(board.D4)
display = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c, reset=reset_pin)
# Clear the display.
display.fill(0)
display.show()
width = display.width
height = display.height

# Configure LoRa Radio
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, 913.5, preamble_length=16, baudrate=1000000, crc=False)
rfm9x.tx_power = 23
rfm9x.coding_rate = 5
rfm9x.signal_bandwidth = 125E3
rfm9x.spreading_factor = 7
rfm9x.preamble_length = 16
rfm9x.destination = 0x13;
prev_packet = None
print("Starting now.")

lastAlt=0
lastCog=0
lastVel=0
lastPacket = time.time()

while True:
    packet = None
    # draw a box to clear the image
    display.fill(0)
    display.text('RasPi LoRa', 35, 0, 1)

    uplink = {}
    uplink["receiver"] = rxName
    # check for packet rx
    packet = rfm9x.receive(keep_listening=True,with_header=True,with_ack=False,timeout=None)
    uplink["toi"] = time.time()
    uplink['rssi'] = rfm9x.last_rssi
    if packet is None:
        display.show()
        timeSince = round(time.time() - lastPacket);
        text = f'{timeSince}s since last PKT -'
        display.text(text, 15, 20, 1)
        print(text, end='\r');
    else:
        # Display the packet text and rssi
        print(packet)
        display.fill(0)
        prev_packet = packet
        packet_text = str(prev_packet, "utf-8")
        display.text('RX: ', 0, 0, 1)
        display.text(packet_text, 25, 0, 1)
        try:
            aprs_packet = aprslib.parse(packet_text)
            uplink["lat"] = aprs_packet["latitude"]
            uplink["lon"] = aprs_packet["longitude"]
            uplink['source'] = aprs_packet['from']
            if 'altitude' in aprs_packet:
                uplink['alt'] = aprs_packet['altitude']
                lastAlt = aprs_packet['altitude']
            else:
                uplink['alt'] = lastAlt
            if 'speed' in aprs_packet:
                uplink["vel"] = aprs_packet["speed"]
                uplink["cog"] = aprs_packet["course"]
                lastCog = aprs_packet["course"]
                lastVel = aprs_packet["speed"]
            else:
                uplink["vel"] = lastVel
                uplink["cog"] = lastCog
        except Exception as ex:
            print("Unknown packet format:  ", ex)
            traceback.print_exception()
        print(json.dumps(uplink))
        client.publish(topic, payload=json.dumps(uplink), qos=0, retain=False)
        lastPacket=time.time();
        time.sleep(.5)

    display.show()
    time.sleep(0.1)
