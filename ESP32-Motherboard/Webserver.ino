#include <ArduinoJson.h>

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
const size_t CAPACITY = JSON_OBJECT_SIZE(1);
StaticJsonDocument<CAPACITY> doc;

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
  
//  for(int i = 0; i < 8; i++)
//  {
//    moduleStore[0][i] = (float)4.2;
//  }
//  cellCount = 7;
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.to<JsonArray>(); // Convert the document to an array.
  
  for(int i = 0; i < cellCount; i++)
  {
    JsonObject arr = array.createNestedObject(); // Create a Nested Object
    arr["cell"] = i + 1;
    byte module = 0;
    if(i > 7 && i < 16)
    {
      module = 1;
    }
    if(i > 15 && i < 24)
    {
      module = 2;
    }
    if(i > 23 && i < 32)
    {
      module = 3;
    }
    arr["voltage"] = moduleStore[module][i - (module * 8)];

    arr["bstate"] = "none"; 
    
//    if(i == 1)
//    {
//      arr["bstate"] = "charging"; 
//    }
//    if(i == 7)
//    {
//      arr["bstate"] = "discharging"; 
//    }
    
  }

  String output;
  serializeJson(doc, output);
  server.send(200,"application/json",output);

  
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
  
  server.begin();
}
