SPS Tagged SSID Scanner

Use this with an ESP8266 module to feed into a trilateration library. This scanner will output SPS tagged SSIDs as hex
along with the channel and RSSI.
Note that if two or more access points have the same MAC address, the ESP8266 module will drop subsequent SSIDs. Changes have been made to allow Nodes to have the same MAC address, provided they are on different channels.
In that sense, each access point needs to have a unique MAC address to work with this ESP scanner.

Change the MAC address of ESP based APs with the following command:

AT+CIPAPMAC="00:00:00:01:01:01"
