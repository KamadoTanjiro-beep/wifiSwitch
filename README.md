# WifiSwitch
Make a simple webserver with timer and a switch.
It helps in operating anything with esp8266 and a relay. NOT TESTED ON ESP32
I am using it to operate my router switch.
One major flaw is that, since it is dependent on the Wifi, so once you turn the router off via webserver, there is no way to turn it on as the webserver would be gone.
I have added the timer to turn it back on at a specified time.
In certain cases, router turns on but internet doesn't works. At that time we turn off the router and turn it back on again. That facility is also provided in the code, just in case.

<img src="https://github.com/chikne97/wifiSwitch/blob/main/demo1.png" alt="chikne97 wifiswitch" width="200" height="200"> <br/>
