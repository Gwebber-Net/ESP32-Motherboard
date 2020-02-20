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
        JSON.stringify({"setting": "btime"})
    );
};
)=====";

void ReceiveSettings()
{

  
  
  String input = server.arg("plain");

  int count = input.length();
  String cut = input.substring(1,count-1);
  Serial.println(cut);

  
  cut.replace("\\","");
  Serial.println(cut);

StaticJsonDocument<200> doc;
DeserializationError err = deserializeJson(doc, cut);
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
JsonObject object = doc.to<JsonObject>();
object["voltage"] = 4.2;

// serialize the object and send the result to Serial
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
  
  server.begin();
}
