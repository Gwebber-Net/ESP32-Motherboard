#include <ArduinoJson.h>



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
