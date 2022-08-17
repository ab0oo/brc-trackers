# brc-trackers

This is a collection of code used for creating small, battery-powered LORA trackers for use at Burning Man (or wherever, really).
The basic idea is to send APRS-formatted compressed position packets over LORA frequencies, have a receiver hear them, publish them
on MQTT, and then commit them to a DB.  On the other side, a web front-end reads the DB and displays the tracked stations in real time.

Amateur radio operators have been doing this for about 35 years, this just uses part 15 (non-licensed) equipment to make it easily
availble to the masses.

It is VERY MUCH a work in progress, and the crucible will be Burning Man 2022.

de AB0OO
John Gorkos

