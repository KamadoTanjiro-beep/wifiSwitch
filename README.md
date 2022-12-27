# WifiSwitch
Make a simple webserver with timer and a switch. <br>
It helps in operating anything with esp8266 and a relay. NOT TESTED ON ESP32<br>
I am using it to operate my router switch.<br>
One major flaw is that, since it is dependent on the Wifi, so once you turn the router off via webserver, there is no way to turn it on as the webserver would be gone.<br>
I have added the timer to turn it back on at a specified time.<br>
In certain cases, router turns on but internet doesn't works. At that time we turn off the router and turn it back on again. That facility is also provided in the code, just in case.<br>

You can add a RTC if required (for timer operation, as internet would be off if you use this to turn off router). Since, I used a readily available Relay Module which didn't had any RTC, I used one of my ESP in Aquarium project (I'll add link soon) to send time to this module via ESP NOW technology.

Module I used:
<img src="https://robu.in/wp-content/uploads/2019/03/314748__00.jpg" alt="chikne97 wifiswitch" width="200" height="200"> <br/>
ESP8266 10A DC 7-30V Network Relay WIFI Module <br>

Demo Preview:
<img src="https://github.com/chikne97/wifiSwitch/blob/main/demo1.png" alt="chikne97 wifiswitch" width="200" height="200"> <br/>
