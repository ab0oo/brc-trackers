# Original header follows
#------------------------------------------
#--- Author: Pradeep Singh
#--- Date: 20th January 2017
#--- Version: 1.0
#--- Python Ver: 2.7
#--- Details At: https://iotbytes.wordpress.com/store-mqtt-data-from-sensors-into-sql-database/
#------------------------------------------
#
# Heavily modified by John Gorkos, AB0OO for BRC Tracker project


import http.client
import json
import paho.mqtt.client as mqtt
import psycopg2
import sys

# DB settings
dbcon  = None
dbhost = "localhost"
dbport = 5432
dbname = "locations"
dbuser = "www-data"
dbpass = "farkelfoo"
#  doc["rxId"] = chipId;
#  doc["txId"] = txID;
#  doc["lat"] = lat;
#  doc["lon"] = lon;
#  doc["alt"] = alt;
#  doc["hdop"] = hdop;
#  doc["vel"] = vel;
#  doc["txNum"] = txNum;
#  doc["rssi"] = rssi;
fields = ['receiver','source','rssi','lat','lon','alt','vel','cog']
db_ins_q = """ INSERT INTO locations  \
        ("receiver","transmitter","rssi","lat","lon","alt","vel","cog") \
               VALUES ( %s, %s, %s, %s, %s, %s, %s, %s )"""

# create table locations( receiver varchar(10), transmitter char(8), toi timestamp with time zone default now(), lat float, lon float, alt float, cog float, vel float, rssi int );
# {"receiver": "HOUSE", "toi": 1660743982.2000704, "rssi": -74, "lat": 38.66174532586382, "lon": -121.0834755306805, "source": "6edf7c", "alt": 0, "vel": 0.0, "cog": 268}
# Received message 'b'{"receiver": "HOUSE", "toi": 1660744103.117748, "rssi": -76, "lat": 38.66175845177278, "lon": -121.08351228322562, "source": "6edf7c", "alt": 0, "vel": 0.0, "cog": 268}'' 
# ' INSERT INTO locations("receiver","transmitter","lat","lon","alt","cog","vel","rssi") VALUES ( \'HOUSE\', \'6edf7c\',  -76, 38.66175845177278,  -121.08351228322562, 0, 0.0, 268 )'


# MQTT Settings 
broker = "localhost"
port = 1883
ka_interval = 45
topic = "brc/trackers"

#Subscribe to all Sensors at Base Topic
def on_connect(client,userdata,flags,rc):
    print("Connected with result code "+str(rc))
    client.subscribe(topic, 0)


def st2pgarray(alist):
    return '{' + ','.join(alist) + '}'


#Save Data into DB Table
def on_message(client, userdata, message):
    # This is the Master Call for saving MQTT Data into DB
    # For details of "sensor_Data_Handler" function please refer "sensor_data_to_db.py"
    print("Received message '" + str(message.payload) + "' on topic '"
        + message.topic + "' with QoS " + str(message.qos))
    pj = (json.loads(message.payload))
    try:
        dbcon = psycopg2.connect(database=dbname, user=dbuser, password=dbpass, port=dbport, host=dbhost)
        cursor = dbcon.cursor()
        insertion_record = []
        for val in fields:
            if val in pj:
                if isinstance(pj[val],list):
                    insertion_record.append([pj[val]])
                else:
                    insertion_record.append(pj[val])
            else:
                insertion_record.append(None)
        #print("Insertion record: "+str(insertion_record))
        print(cursor.mogrify(db_ins_q,insertion_record))
        cursor.execute(db_ins_q,insertion_record)
        dbcon.commit()

    except (psycopg2.DatabaseError,Exception) as e:
        print("Failed to insert location record", e)

    finally:
        cursor.close()
        #dbcon.disconnect()
        pass

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed to topic")


def write_data(payload):
    pass


print("Building client")
client = mqtt.Client()

# Assign event callbacks
client.on_message = on_message
client.on_connect = on_connect
client.on_subscribe = on_subscribe
client.username_pw_set("john","password5678")

# Connect
client.connect(broker, int(port), int(ka_interval))

# Continue the network loop
client.loop_forever()
