# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT

# Example using Interrupts to send a message and then wait indefinitely for messages
# to be received. Interrupts are used only for receive. sending is done with polling.
# This example is for systems that support interrupts like the Raspberry Pi with "blinka"
# CircuitPython does not support interrupts so it will not work on  Circutpython boards
# Author: Tony DiCola, Jerry Needell
import time
import board
import busio
import digitalio
from digitalio import DigitalInOut, Direction, Pull
# Import the SSD1306 module.
import adafruit_ssd1306
import RPi.GPIO as io
import adafruit_rfm9x
# this is all for decoding and sending packets to MQTT
import aprslib
import logging
import json
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import base91

broker = "localhost"
mqttPort = 1883
ka_interval = 45
topic = "brc/trackers"
# logging.basicConfig(level=logging.DEBUG) # enable this to see APRS parser debugging
rxName = "5:00&A"
mqttAuth = {'username':"john", 'password':"password5678"}
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

# setup interrupt callback function
def rfm9x_callback(rfm9x_irq):
    global packet_received  # pylint: disable=global-statement
    global lastPacket
    global lastCog
    global lastVel
    global lastAlt
    # check to see if this was a rx interrupt - ignore tx
    if rfm9x.rx_done:
        packet = rfm9x.receive(with_header=True, timeout=None)
        if packet is not None:
            packet_received = True
            # Received a packet!
            # Print out the raw bytes of the packet:
            uplink = {}
            uplink["receiver"] = rxName
            uplink["toi"] = time.time()
            uplink['rssi'] = rfm9x.last_rssi
            try:
                display.fill(0)
                prev_packet = packet
                packet_text = str(prev_packet, "utf-8")
                display.fill(0)
                display.text('RX: ', 0, 0, 1)
                display.text(packet_text, 25, 0, 1)
                display.show()
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
                print(json.dumps(uplink))
                publish.single(topic, payload=json.dumps(uplink), hostname=broker, client_id=rxName, auth=mqttAuth, port=mqttPort)
                #client.publish(topic, payload=json.dumps(uplink), qos=0, retain=False)
            except Exception as ex:
                print("Unknown packet format:  ", ex)
            lastPacket=time.time();
            time.sleep(.1)

# Define radio parameters.
RADIO_FREQ_MHZ = 913.5  # Frequency of the radio in Mhz. Must match your
# module! Can be a value like 915.0, 433.0, etc.

# Define pins connected to the chip, use these if wiring up the breakout according to the guide:
CS = digitalio.DigitalInOut(board.CE1)
RESET = digitalio.DigitalInOut(board.D25)

# Initialize SPI bus.
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)

# Initialze RFM radio
#rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, RADIO_FREQ_MHZ)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, 913.5, preamble_length=8, baudrate=1000000, crc=False)

# Note that the radio is configured in LoRa mode so you can't control sync
# word, encryption, frequency deviation, or other settings!

# You can however adjust the transmit power (in dB).  The default is 13 dB but
# high power radios like the RFM95 can go up to 23 dB:
rfm9x.tx_power = 13
rfm9x.coding_rate = 5
rfm9x.signal_bandwidth = 125E3
rfm9x.spreading_factor = 7
rfm9x.preamble_length = 8
rfm9x.destination = 0x13;

# configure the interrupt pin and event handling.
RFM9X_G0 = 22
io.setmode(io.BCM)
io.setup(RFM9X_G0, io.IN, pull_up_down=io.PUD_DOWN)  # activate input
io.add_event_detect(RFM9X_G0, io.RISING)
io.add_event_callback(RFM9X_G0, rfm9x_callback)

packet_received = False
lastAlt=0
lastCog=0
lastVel=0
lastPacket = time.time()

# this seems to be necessary to enable reception.  Beats me...
rfm9x.send(bytes("Hello world!\r\n", "utf-8"), keep_listening=True)
print("Sent Hello World message!")

# Wait to receive packets.  Note that this library can't receive data at a fast
# rate, in fact it can only receive and process one 252 byte packet at a time.
# This means you should only use this for low bandwidth scenarios, like sending
# and receiving a single message at a time.
lastPacketTime = int(round(time.time()));
print("Waiting for packets...")
while True:
    now = int(round(time.time()))
    secondsSincePacket = now - lastPacketTime
    display.fill(0)
    display.text(f"{secondsSincePacket}s since last RX",25, 0, 1)
    display.show()
    time.sleep(0.5)
    if packet_received:
        # the message is processed in the callback, and we reset for another go-round
        packet_received = False
        lastPacketTime = now;
