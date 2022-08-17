This project builds heavily on Jason Coon's esp32_fastled_ble project.  Please see the accompanying README for info.

This tracker uses a Heltec ESP32 LORA devboard with an external GPS connected via the second serial port to provide compressed APRS packets via LORA at 125kHw:4/5:SF7.

The tracker also has the ability to drive a string of addressable RGB LEDs, using BLE as the control mechanism.

I am using this project for bicycle tracking (and, possibly, recovery) for a set of eBikes in Black Rock City for the annual Burning Man gathering.

