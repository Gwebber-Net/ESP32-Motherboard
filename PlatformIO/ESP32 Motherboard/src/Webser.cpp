#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>

extern float moduleVoltages[10][8];
extern byte cellCount;

extern WebServer server;
extern String PackInfo();
extern String Summary();
extern String Settings();


char webpage[] PROGMEM = R"=====(
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<script type="text/javascript" src="/js/nrg.js"></script>
</head>
<body>
<p> Voltage <span id="voltage">__</span> </p>
<button onclick="GetVoltage()"> Get Voltage </button>
<br>
<br>
<p> Result  <span id="result"></span> </p>
<button onclick="PostSettings()"> Send Settings</button>
</body>
</html>
)=====";


char www_js_nrg[] PROGMEM = R"=====(

function ajax(method, url, ctype="", on_success, on_failure, post_data="")
{
  var xhr = new XMLHttpRequest();
  xhr.onload = function()
  {
    if(xhr.status == 200)
    {
        on_success(xhr);
    } else 
    {
        on_fail(xhr);
    }
  };
    xhr.open(method, url);
    
    if ( method == "POST" ) 
    {
        xhr.setRequestHeader('Content-Type', ctype);
        xhr.send(JSON.stringify(post_data));
        console.log(post_data);
    }
    else
    {
      xhr.send();
    }
};
function GetVoltage()
{
  ajax("GET",
       "/voltage", 
       "application/json", 
       // success callback
       function(result) 
       {
            var obj = JSON.parse(result.responseText);
            document.getElementById("voltage").innerHTML = obj.voltage;
            console.log(result.status);
            console.log(result.responseText);
       },
       // error callback
       function(result) 
       {
            console.log(result.status);
            console.log(result.responseText);            
        }
        
  );
};

function PostSettings()
{
    ajax("POST",
       "/settings", 
       "application/json", 
       // success callback
       function(result) 
       {
            //var obj = JSON.parse(result.responseText);
            //document.getElementById("voltage").innerHTML = obj.voltage;
            console.log(result.status);
            console.log(result.responseText);
       },
       // error callback
       function(result) 
       {
            console.log(result.status);
            console.log(result.responseText);            
        },
        {setting: "btime"}
    );
};
)=====";



void ReceiveSettings()
{
  String input = server.arg("plain");
  Serial.println(input);
  
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, input);
  if (err) 
  {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(err.c_str());
  }
  else
  {
    String s = doc["setting"];
    Serial.println(s);
  }  
}




void SendVoltage()
{

StaticJsonDocument<100> doc;

// create an object
//JsonObject object = doc.to<JsonObject>();
//object["voltage"] = 4.2;
doc["voltage"] = 4.2;
// serialize the object and send the result to Serial
String output;
serializeJson(doc, output);
server.send(200,"application/json",output);
}


void SendPackInfo()
{
  String output = PackInfo();
  server.send(200,"application/json",output);

  
}

void SendSummary()
{
  String summary = Summary();
  server.send(200,"application/json",summary);
}


void SendConfig()
{
  String settings = Settings();
  server.send(200,"application/json",settings);
}


void InitialiseServer()
{
  server.on("/",[](){
    Serial.println("Service: /index.html");
    server.send_P(200, "text/html", webpage);}
  );
  server.on("/js/nrg.js",[](){
    Serial.println("Service: /js/nrg.js");
    server.send_P(200, "application/x-javascript", www_js_nrg);}
  );
  server.on("/voltage", SendVoltage);
  server.on("/settings", ReceiveSettings);
  server.on("/api/packinfo", SendPackInfo);
  server.on("/api/summary", SendSummary);
  server.on("/api/config",SendConfig);
  
  server.begin();

  moduleVoltages[0][0] = (float)3.0;
  moduleVoltages[0][1] = (float)3.1;
  moduleVoltages[0][2] = (float)3.2;
  moduleVoltages[0][3] = (float)3.3;
  moduleVoltages[0][4] = (float)3.4;
  moduleVoltages[0][5] = (float)3.5;
  moduleVoltages[0][6] = (float)3.6;
  moduleVoltages[0][7] = (float)3.7;
  moduleVoltages[1][1] = (float)3.8;
  moduleVoltages[1][2] = (float)3.9;
  moduleVoltages[1][3] = (float)3.9;
  moduleVoltages[1][4] = (float)3.8;
  moduleVoltages[1][5] = (float)3.7;
  moduleVoltages[1][6] = (float)3.6;
  moduleVoltages[1][7] = (float)3.5;
  moduleVoltages[2][1] = (float)3.4;
  moduleVoltages[2][2] = (float)3.3;
  moduleVoltages[2][3] = (float)3.2;
  moduleVoltages[2][4] = (float)3.1;
  moduleVoltages[2][5] = (float)3.0;
  moduleVoltages[2][6] = (float)2.9;
  moduleVoltages[2][7] = (float)2.8;
  moduleVoltages[3][1] = (float)2.7;
  cellCount = 23;
}