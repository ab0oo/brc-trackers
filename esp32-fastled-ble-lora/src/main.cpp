/*
   ESP32 FastLED BLE: https://github.com/jasoncoon/esp32-fastled-ble
   Copyright (C) 2017 Jason Coon

   Built upon the amazing FastLED work of Daniel Garcia and Mark Kriegsman:
   https://github.com/FastLED/FastLED

   ESP32 support provided by the hard work of Sam Guyer:
   https://github.com/samguyer/FastLED

   Arduino BLE support based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   updated by chegewara

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <EEPROM.h>
#include <FastLED.h>

#include <SPI.h>
#include <heltec.h>
#include "main.h"

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001008)
#warning "Requires FastLED 3.1.8 or later; check github for latest code."
#endif

TinyGPSPlus gps;

uint8_t power = 1;
uint8_t brightness = 128;
CRGB solidColor = CRGB::Green;
CRGB leds[NUM_LEDS];

uint8_t currentPatternIndex = 0;
uint8_t currentPaletteIndex = 0;
uint8_t gHue = 0;  // rotating "base color" used by many of the patterns
uint8_t speed = 30;
uint8_t cooling = 50;
uint8_t sparking = 120;
bool colorChanged = false;

#include "patterns.h"
#include "ble.h"

char txpacket[BUFFER_SIZE];
unsigned char foo[4];
int txNumber;
int16_t rssi, rxSize;
uint32_t chipID = 0;
uint32_t nextTx = 0;
char sourceId[6];

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
	for(int i=0; i<17; i=i+8) {
	  chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
    sprintf(sourceId, "%06x", chipID);
    Serial.printf("Initializing device %s\n", sourceId);

    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);

    Heltec.display->init();
    Heltec.display->clear();
    Heltec.display->display();

    displayId();

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, 0, NUM_LEDS_PER_STRIP);

    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);

    // set master brightness control
    FastLED.setBrightness(brightness);
    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DI0);
    if (!LoRa.begin((long)RF_FREQUENCY, true)) {
        Serial.println("Starting LoRa failed!");
        while (1)
            ;
    }
    setupBLE(sourceId);
}

void loop() {
    uint32_t starttime = millis();

    // We want to display GPS data and Device ID, alternating.
    if ( (starttime / 1000) % 10 > 5) {
        displayId();
    } else {
        displayLocation();
    }

    // notify changed value
    //  if (deviceConnected && colorChanged) {
    //    pCharacteristic->setValue((uint8_t*)&value, 4);
    //    pCharacteristic->notify();
    //    value++;
    //    delay(6); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    //  }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);                   // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }

    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
        Serial.println("connecting");
    }

    if (power == 0) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else {
        // Call the current pattern function once, updating the 'leds' array
        patterns[currentPatternIndex].pattern();

        EVERY_N_MILLISECONDS(40) {
            // slowly blend the current palette to the next
            nblendPaletteTowardPalette(currentPalette, targetPalette, 8);
            gHue++;  // slowly cycle the "base color" through the rainbow
        }
    }

    // // send the 'leds' array out to the actual LED strip
    FastLED.show();

    // insert a delay to keep the framerate modest
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    // delay(1000 / FRAMES_PER_SECOND);

    while (Serial2.available() > 0) gps.encode(Serial2.read());

    if (gps.location.age() < 2000) {
        if (starttime > nextTx) {
//            Heltec.display->drawString(120, 0, "A");
            txNumber += 1;
            signed int lat = gps.location.lat() * 100000;
            signed int lon = gps.location.lng() * 100000;
            signed int alt = gps.altitude.feet();
            unsigned int vel = gps.speed.knots();
            unsigned int cog = (int)round(gps.course.deg());
            strncpy(txpacket, sourceId, 6);
            strncpy(txpacket + 6, ">APZ001:", 8);
            // As this is an APRS format, we must start with an indicator
            // that we are unable to receive messages.
            int i = 14;
            txpacket[i++] = '!';
            txpacket[i++] = '\\';
            base91Encode(true, gps.location.lat(), foo);
            txpacket[i++] = foo[0];
            txpacket[i++] = foo[1];
            txpacket[i++] = foo[2];
            txpacket[i++] = foo[3];
            base91Encode(false, gps.location.lng(), foo);
            txpacket[i++] = foo[0];
            txpacket[i++] = foo[1];
            txpacket[i++] = foo[2];
            txpacket[i++] = foo[3];
            txpacket[i++] = '^';
            // Encoding COURSE and SPEED into the MSG body
            // ln(0) == -infinity, which doesn't translate well to ascii....
            txpacket[i++] = (char)((cog + 1) / 4 + 33);
            if (vel == 0) {
                txpacket[i++] = (char)(33);
            } else {
                txpacket[i++] = (char)(log(vel) / LN_1_08 + 33);
            }
            // Set compression type byte to 58 (00111010)
            txpacket[i++] = (char)(58 + 33);
            // add the TX packet counter to the end of the message
            txpacket[i++] = 
            // the Type Byte is encoded as 00110110 (0x36 , 54d)
            Serial.printf("[%d]Encoded value %d:  %s\t\t -- ", txNumber, i, txpacket);
            Serial.printf("Lat: %d, lon %d, alt %d, vel: %d, cog: %d, chipId %6x\n",
                          lat, lon, alt, vel, cog, chipID);
            LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
            LoRa.setSignalBandwidth(125E3);
            LoRa.setCodingRate4(LORA_CODINGRATE+4);
            LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
            LoRa.setSyncWord(0x13);
            LoRa.beginPacket();
            LoRa.write( (uint8_t *)txpacket, i); //send the package out
            LoRa.endPacket();
            LoRa.idle();
            nextTx = starttime + 9950 + random(250);
        }
    } else {
//        Heltec.display->drawString(120, 0, "V");
    }
}

void displayLocation() {
    char str[30];
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_10);
    int index = sprintf(str, "%02d-%02d-%02d", gps.date.year(), gps.date.day(), gps.date.month());
    str[index] = 0;
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->drawString(0, 0, str);

    index = sprintf(str, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
    str[index] = 0;
    Heltec.display->drawString(60, 0, str);

    index = sprintf(str, "alt: %d.%d", (int)gps.altitude.meters(), fracPart(gps.altitude.meters(), 2));
    str[index] = 0;
    Heltec.display->drawString(0, 16, str);

    index = sprintf(str, "hdop: %d.%d", (int)gps.hdop.hdop(), fracPart(gps.hdop.hdop(), 2));
    str[index] = 0;
    Heltec.display->drawString(0, 32, str);

    index = sprintf(str, "lat :  %d.%d", (int)gps.location.lat(), fracPart(gps.location.lat(), 4));
    str[index] = 0;
    Heltec.display->drawString(60, 16, str);

    index = sprintf(str, "lon:%d.%d", (int)gps.location.lng(), fracPart(gps.location.lng(), 4));
    str[index] = 0;
    Heltec.display->drawString(60, 32, str);

    index = sprintf(str, "%d.%d km/h @ %d°", (int)gps.speed.kmph(), fracPart(gps.speed.kmph(), 3), gps.course.deg());
    str[index] = 0;
    Heltec.display->drawString(0, 48, str);
    Heltec.display->display();
}

int fracPart(double val, int n) {
    return (int)((val - (int)(val)) * pow(10, n));
}

void base91Encode(bool latitude, double value, unsigned char encoded[4]) {
    int a = 0;
    if (latitude) {
        a = 380926 * (90 - value);
    } else {
        a = 190463 * (180 + value);
    }
    encoded[0] = (char)(a / pow(91, 3) + 33);
    a = a % (int)(pow(91, 3));
    encoded[1] = (char)(a / pow(91, 2) + 33);
    a = a % (int)(pow(91, 2));
    encoded[2] = (char)(a / 91 + 33);
    encoded[3] = (char)(a % 91 + 33);
}

void displayId() {
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(64, 0, "Device ID");
    Heltec.display->drawString(64, 32, sourceId);
    Heltec.display->display();
}