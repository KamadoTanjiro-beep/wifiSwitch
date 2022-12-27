/*********
ORIGINAL IDEA:
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-async-web-server-espasyncwebserver-library/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

Modified:
Aniket Patra
V0.0.2
* It can receive time data from another ESP Node using ESP NOW protocol (since this project isn't using inbuilt RTC)
* Receiving time was introduced as when the router is off it cannot get it's time using timeclient, hence this project uses another ESP already present in the area with a RTC like DS3231
V0.0.3 20/12/2022
*New UI
*Added SPIFFS for offline user settings storage
V0.0.31 27/12/2022
*bug Tuesday Spelling
*********/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <FS.h>  //Include File System Headers
//OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "global.h"  //remove this line before using
//ESPNOW
#include <espnow.h>

#ifndef STASSID
#define STASSID "YOUR_SSID"     // WIFI NAME/SSID
#define STAPSK "YOUR_PASSWORD"  // WIFI PASSWORD
#endif

const char* ssid = pssid;      // Replace "pssid" with write "STASSID"
const char* password = ppass;  // Replace "ppass" with write "STAPSK"


const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

const char* PARAM_INPUT_3 = "time";
const char* PARAM_INPUT_4 = "settime";
const char* PARAM_INPUT_5 = "hour";
const char* PARAM_INPUT_6 = "min";

const String newHostname = "WiFiNode"; //Give any name that you want (would show up in router table)
char week[7][20] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

int onTime = 0, offTime = 0;
bool onFlag = 0, offFlag = 0;

const int relay1 = 4;
// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int time[8];
} struct_message;

// Create a struct_message called myData
struct_message myData;

//For timer checking
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;  //1 sec

//For reseting
unsigned long lastTime1 = 0;
unsigned long timerDelay1 = 600000;  //10 mins

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Set your Static IP address
IPAddress local_IP(192, 168, 29, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 29, 1);

IPAddress subnet(255, 255, 255, 0);

IPAddress primaryDNS(8, 8, 8, 8);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

void fileWriter(int data, int count) {
  /*The data in integer is a time, ex 06:00 is 600, count is 1 for on time and 2 for off time*/
  String rem = String(data);
  rem = rem + '\n';
  File file;

  if (count == 1)
    file = SPIFFS.open("/onData.txt", "w");  //Open File for writing
  else if (count == 2)
    file = SPIFFS.open("/offData.txt", "w");  //Open File for writing

  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }

  int bytesWritten = file.print(rem);
  int len = rem.length();

  if (bytesWritten == len) {
    Serial.println("Data Saved: ");
    Serial.println(bytesWritten);
    Serial.println(rem);

  } else {
    Serial.println("Data Save Failed");
    Serial.println(bytesWritten);
    Serial.println(len);
  }
  file.close();
}

void fileReader(int count) {
  File file;
  String rem;
  if (count == 1)
    file = SPIFFS.open("/onData.txt", "r");  //Open File for reading
  else if (count == 2)
    file = SPIFFS.open("/offData.txt", "r");  //Open File for reading

  if (!file) {
    Serial.println("Error opening file for reading");
    return;
  }
  int line = 0;
  while (file.available()) {
    if (line == 0) {
      rem = file.readStringUntil('\n');
    }
  }

  if (count == 1)
    onTime = rem.toInt();  //initialize global timer variables
  else if (count == 2)
    offTime = rem.toInt(); //initialize global timer variables
  file.close();
}

//converts integer time into string
String intToTimeString(int x) {
  String time;
  if (x == 0)
    time = "00:00";
  else if (x < 100)  //00:12 = 12
  {
    time += "00:";
    if (x < 10) {
      time += "0";
      time += String(x);
    } else
      time += String(x);
  } else if (x < 1000)  //08:45 = 845
  {
    time += "0";
    int temp = x / 100;
    time += String(temp) + ":";
    x = x % 100;
    if (x < 10) {
      time += "0";
      time += String(x);
    } else
      time += String(x);
  } else if (x >= 1000) {  //22:45 = 2245
    int temp = x / 1000;
    time += String(temp);
    x = x % 1000;
    temp = x / 100;
    time += String(temp) + ":";
    x = x % 100;
    if (x < 10) {
      time += "0";
      time += String(x);
    } else
      time += String(x);
  }
  return time;
}

// Replaces placeholder with button section in your web page
String processor(const String& var) {
  //Serial.println(var);
  if (var == "BUTTONPLACEHOLDER") {
    String buttons = "";
    buttons += "<div class=\"mb-1 mt-5 row d-flex justify-content-center\"><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(relay1) + "\" " + outputState(relay1) + "><span class=\"slider\"></span></label></div>";
    return buttons;
  }
  if (var == "TIMEDAY") {
    String timeData = "";
    timeData += "<h3 id='time' class=\"display-5 text-light bg-dark rounded\">" + String(myData.time[0]) + ":" + String(myData.time[1]) + ", " + String(myData.time[4]) + "/" + String(myData.time[5]) + "/20" + String(myData.time[6]) + ", " + week[myData.time[7]] + ", " + myData.time[3] + "&deg;C</h3>";
    return timeData;
  }
  if (var == "ONTIME") {
    String timeData = intToTimeString(onTime);
    return timeData;
  }
  if (var == "OFFTIME") {
    String timeData = intToTimeString(offTime);
    return timeData;
  }
  return String();
}

String outputState(int output) {
  if (digitalRead(output)) {
    return "";
  } else {
    return "checked";
  }
}

//Web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
<meta charset="UTF-8">
  <title>WiFi Switch</title>
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">  
  <link rel="icon" type="image/jpg" href="https://thumbs.dreamstime.com/b/switch-icon-vector-sign-symbol-isolated-white-background-logo-concept-your-web-mobile-app-design-134155804.jpg" />
  <link href="https:\//cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css" rel="stylesheet"
    integrity="sha384-EVSTQN3/azprG1Anm3QDgpJLIm9Nao0Yz1ztcQTwFspd3yD65VohhpuuCOmLASjC" crossorigin="anonymous">
    <style> @import url('https://fonts.googleapis.com/css2?family=Rubik+Gemstones&display=swap'); </style>
  
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #eb380c}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body class="bg-warning">
<div class="container">
  <img src="https://telecomtalk.info/wp-content/uploads/2022/11/jiofiber-300-mbps-prepaid-plan-can-meet.jpeg" alt="" height="180px" style="border: 2px solid rgb(243, 201, 12);">
    <!--<h2 class="display-2">Jio Fiber Router</h2>-->
  %TIMEDAY%
  %BUTTONPLACEHOLDER%  
 
 <div class="border border-5 p-4 mt-3">
      <div class="col-auto">
        <br><br><span style="font-family: 'Rubik Gemstones', cursive; font-size:xx-large;">TURN ON:</span>
        <input type="time" id="onTime" class="form-control" value="%ONTIME%">
  <button onclick="myFunction();" class="btn btn-primary mt-3">Save</button>

  </div>
      <div class="col-auto">
        <br><br><span style="font-family: 'Rubik Gemstones', cursive; font-size:xx-large;">TURN OFF:</span>
         <input type="time" id="offTime" class="form-control" value="%OFFTIME%">
</div>
  <button onclick="myFunction2();" class="btn btn-primary mt-3">Save</button>
  </div>
  </div>
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/js/bootstrap.bundle.min.js" integrity="sha384-MrcW6ZMFYlzcLA8Nl+NtUVF0sA7MsXsP1UyJoMp4YLEuNSfAP+JcXn/tWtIaxVXM" crossorigin="anonymous"></script>
<script>
    function myFunction() {
      
      var x = document.getElementById("onTime").value;   
      var myArray = x.split(":");
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/api1?settime=1&hour="+myArray[0]+"&min="+myArray[1], true);
      xhr.send();
      xhr.onload = function () {
        if (xhr.status != 200) {
          alert(`Error ${xhr.status}: ${xhr.statusText}`);
        } else {
          alert(`Done, ON time set at ${xhr.responseText} `);
        }
      };
      xhr.onerror = function () {
        console.log("Request failed");
      };   
           
    }
    function myFunction2() {
      
      var y = document.getElementById("offTime").value;
      var myArray = y.split(":");
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/api1?settime=2&hour="+myArray[0]+"&min="+myArray[1], true);
      xhr.send();
      xhr.onload = function () {
        if (xhr.status != 200) {
          alert(`Error ${xhr.status}: ${xhr.statusText}`);
        } else {
          alert(`Done, OFF time set at ${xhr.responseText} `);
        }
      };
      xhr.onerror = function () {
        console.log("Request failed");
      };    
          
    }
  </script>

<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  xhr.send();
}
var week=["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
function everyTime() {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/api?time=1", true);            
            xhr.send();
            xhr.onload = function () {
                if (xhr.status != 200) {
                    alert(`Error ${xhr.status}: ${xhr.statusText}`);
                    
                } else {
                  var x = xhr.responseText;
                    var myArray = x.split(":");
                    if(myArray[0].length==1)
                    myArray[0]="0"+myArray[0];

                    if(myArray[1].length==1)
                    myArray[1]="0"+myArray[1];

                    if(myArray[3].length==1)
                    myArray[3]="0"+myArray[3];

                    if(myArray[4].length==1)
                    myArray[4]="0"+myArray[4];

                    let word = myArray[0]+':'+myArray[1]+', '+myArray[3]+'/'+myArray[4]+'/20'+myArray[5]+', '+week[parseInt(myArray[6])]+', '+myArray[2]+'&deg;C';
                    document.getElementById('time').innerHTML=word;
                    //console.log(`Done, got ${xhr.responseText} `);
                }
            };
            xhr.onerror = function () {
                alert("Request failed");
            }; }

            var myInterval = setInterval(everyTime, 5000);
</script>
</body>
</html>
)rawliteral";

byte flag = 0; //for router reset

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
}


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, LOW);

  bool success = SPIFFS.begin();
  SPIFFS.gc();  //garbage collection
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }

  fileReader(1);
  fileReader(2);

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname(newHostname.c_str());  // Set new hostname
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    break;
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    } else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    /*Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);*/
    request->send(200, "text/plain", "OK");
  });

  // Send a TIME request to <ESP_IP>/api?time=<inputMessage1>
  server.on("/api", HTTP_GET, [](AsyncWebServerRequest* request) {
    String inputMessage1;
    // GET input1 value on <ESP_IP>/api?time=<inputMessage1>
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage1 = request->getParam(PARAM_INPUT_3)->value();
      int x = inputMessage1.toInt();
      if (x == 1) {
        String y = String(myData.time[0]) + ":" + String(myData.time[1]) + ":" + String(myData.time[3]) + ":" + String(myData.time[4]) + ":" + String(myData.time[5]) + ":" + String(myData.time[6]) + ":" + String(myData.time[7]);
        request->send(200, "text/plain", y);
      }
    } else
      inputMessage1 = "No message sent";
    /*Serial.println("Request: ");
    Serial.print(inputMessage1); */
  });

  // Send a AUTO ON OFF request to <ESP_IP>/api?settime=<inputMessage1>&hour=<inputMessage2>&min=<inputMessage3>
  server.on("/api1", HTTP_GET, [](AsyncWebServerRequest* request) {
    String inputMessage1, inputMessage2, inputMessage3, timeString;
    int x, y;
    // GET input1 value on <ESP_IP>/api?settime=<inputMessage1>&hour=<inputMessage2>&min=<inputMessage3>
    if (request->hasParam(PARAM_INPUT_4) && request->hasParam(PARAM_INPUT_5) && request->hasParam(PARAM_INPUT_6)) {
      inputMessage1 = request->getParam(PARAM_INPUT_4)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_5)->value();
      inputMessage3 = request->getParam(PARAM_INPUT_6)->value();
      timeString = inputMessage2 + inputMessage3;
      x = inputMessage1.toInt();
      y = timeString.toInt();
      onFlag = 1;
      offFlag = 1;
    }
    if (x == 1) {
      onTime = y;
      fileWriter(onTime, 1);
    } else if (x == 2) {
      offTime = y;
      fileWriter(offTime, 2);
    }


    request->send(200, "text/plain", inputMessage2 + ":" + inputMessage3);
  });

  // Start server
  server.begin();

  //OTA update
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  ArduinoOTA.handle();

  if ((millis() - lastTime) > timerDelay) {
    int hour = myData.time[0];
    hour *= 100;
    int min = myData.time[1];
    hour += min;
    int timeString = hour;

    if ((onFlag == 1) && (onTime == timeString)) {
      onFlag = 0;
      offFlag = 1;
      digitalWrite(relay1, LOW);
    }
    if ((offFlag == 1) && (offTime == timeString)) {
      onFlag = 1;
      offFlag = 0;
      digitalWrite(relay1, HIGH);
    }
    lastTime = millis();
  }

//This checks for no internet, automatically reboots the router. Also, the flag protects from fluctuations. Say, if for a moment internet
//is not there and flag is set to 1. No, suddenly the wifi comes back and the flag would be set to 0 by else, hence it won't let the relay to shut down.
  if (WiFi.status() != WL_CONNECTED) {
    if (flag == 0) {
      lastTime1 = millis();
      flag = 1;
    }
    if (((millis() - lastTime1) > timerDelay1) && flag == 1) {
      digitalWrite(relay1, HIGH);  //OFF
      delay(10000);                //wait for 10 sec
      digitalWrite(relay1, LOW);   //ON
      lastTime1 = millis();
    }
  } else
    flag = 0;
}