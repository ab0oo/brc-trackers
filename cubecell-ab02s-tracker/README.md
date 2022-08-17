# CubeCell APRS Tracker
Highly optimized LORA tracking system for localized tracking of mobile stations.  Uses APRS Compressed position packets transmitted at 125kHz:4/5:SF7 on a single frequency.

This code uses the onboard LED to indicate GPS lock:  green is locked, red is not.  Position transmissions begin with the GPS locks, and stops when lock is lost.

This project is presented as a PlatformIO build.  Get PlatformIO here:  https://platformio.org/platformio-ide.  Edits will probably be needed in the platformio.ini file to set up the upload and monitor ports for your particular build.

Additional docs for the Python code can be found in the Python directory.


