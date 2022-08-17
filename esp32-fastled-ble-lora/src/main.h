#pragma once

#include <TinyGPSPlus.h>
#include <Wire.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define DATA_PIN    18
//#define CLK_PIN     12
#define LED_TYPE SK6812
#define COLOR_ORDER GRB
#define NUM_STRIPS 1
#define NUM_LEDS_PER_STRIP 60
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS

// set up the pins for the GPS connection
#define TXD2 21
#define RXD2 36

#define BAND 915E6  //you can set band here directly,e.g. 868E6,915E6

#define MILLI_AMPS 3000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND 60

#define RF_FREQUENCY                                913500000 // Hz

#define TX_OUTPUT_POWER                             17        // dBm

#define SCK 5    // GPIO5  -- SX1278's SCK
#define MISO 19  // GPIO19 -- SX1278's MISO
#define MOSI 27  // GPIO27 -- SX1278's MOSI
#define SS 18    // GPIO18 -- SX1278's CS
#define RST 14   // GPIO14 -- SX1278's RESET
#define DI0 26   // GPIO26 -- SX1278's IRQ(Interrupt Request)


#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 29 // Define the payload size here
// the natural log of 1.002 is needed for the altitude encoding
#define LN_1_002 0.00199800266
#define LN_1_08 0.07696104113


// The TinyGPSPlus object
extern TinyGPSPlus gps;

extern uint8_t power;
extern uint8_t brightness;
extern CRGB solidColor;
extern bool colorChanged;

extern uint8_t currentPatternIndex;
extern uint8_t currentPaletteIndex;
extern uint8_t gHue;
extern uint8_t speed;
extern uint8_t cooling;
extern uint8_t sparking;

int fracPart(double val, int n);
void displayLocation();
void displayId();
void base91Encode(bool latitude, double value, unsigned char encoded[4]);
void DoubleToString(char *str, double double_num, unsigned int len);