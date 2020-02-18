#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer server;

char* ssid = "H220RK";
char* password = "jfmamjjasond";

// debug Variables
byte debug = 1;
// debug Variables


// Module Get Count
byte moduleGetCount = 0;
byte moduleCountFound = 0;
byte moduleCountError = 0;
byte moduleCount = 0;
// Module Get Count

// Module Get Data
byte moduleGetData = 0;
byte moduleGetDataPoint = 0;
byte moduleGetDataCount = 0; // How many times the data from all the modules has been requested.
// Module Get Data

// Module Write Settings
byte moduleWriteSettings = 0;
byte moduleWriteSettingsPoint = 0;
byte moduleWriteSettingsCount = 0; // How many times the data from all the modules has been requested.
// Module Write Settings


// Overflow Variabelen
long moduleGetCountOverflow = 0;
long moduleGetDataOverflow = 0;
long moduleWriteSettingsOverflow = 0;
long debugging = 0;
long dataReceivedErrorOverflow = 0;
// Overflow Variabelen


// Shunt board Variables
double referenceVoltage;
// Shunt board Variables


// Module Settings
byte settingbTime;
byte settingbThreshold;

int settingcutoffVoltage;
// Module Settings






void setup()
{
  // Initialise the I2C.
  Wire.begin();
  
  pinMode(2,OUTPUT); // RelayControl1
  pinMode(4,OUTPUT); // RelayControl2

  ////////////////////////////////
  //  Here starts the module count checking process
  //  The watchmon needs to know how many modules there are connected.
  ////////////////////////////////
  // Start the timer that will check if there was a request from the slave modules.
  moduleGetCount = 1;

  // Send the GetCount message to the module(s).
  Put_Header(0,0); // Adres 0 Cmd 0
  Put_Data(3); // Sub_Cmd 0
  Put_Data(0);
  Put_Teal();
  Senddata();


  
  Serial.begin(115200);
  Serial.println("Connecting");
  WiFi.begin(ssid,password);
  
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",[](){server.send(200,"text/plain","Hello World!");});
  server.on("/voltage",SendVoltage);
  
  server.begin();



  
}

void loop()
{
  server.handleClient();

  if(Serial1.available()) // Is there a new Byte?
  {
    ReadBuffer(); // Read a byte.
    
  }
  ///////////////////////////
//  Module Count routine
//
///////////////////////////
  if(moduleGetCount)
  {
    if(moduleGetCountOverflow > 1000)
    {     
      moduleGetCountOverflow= 0;
      // The timer is still active, and the time has passed.
      if(moduleCountFound)
      {
        moduleCountFound = 0;
        // Module_Count is found, so now we can move on into the software.
        moduleGetCount = 0;
        //Module_Get_Data = 1;
        moduleWriteSettings = 1;
        
        moduleCountError = 0;
      }
      else // Time has passed but no Module_Count_Found.
      {
       moduleCountError = 1;
       moduleGetCount = 0;
       //Module_Get_Data = 1;
        
      }
     }
   }
///////////////////////////
// End Module Count routine.
//
///////////////////////////


///////////////////////////
// Start Module Get Data routine.
//
///////////////////////////
if(moduleGetDataOverflow > 200)
{
  moduleGetDataOverflow = 0; // Reset the overflow
  if(moduleGetData) // Is the data sequence enabled ?
  {
    Put_Header(moduleGetDataPoint,0);
    Put_Data(1); // Subcommand 1 is request voltages.
    Put_Teal();
    Senddata();

    moduleGetDataPoint++;
    //Serial.print("Module_Get_Data_Point = ");
    //Serial.println(Module_Get_Data_Point);
    if(moduleGetDataPoint == moduleCount)
    {
      //Serial.println("Module_Get_Data_Point = Module_Count_Current");
      moduleGetDataPoint = 0;
      moduleGetDataCount++;
      if(moduleGetDataCount == 5)
      {
        moduleGetDataCount = 0;
        moduleGetData = 0; // Disable the sequence.
        moduleGetCount = 1; // Enable the Module_Get_Count sequence.
        moduleGetCountOverflow = 0; // Reset the Module_Get_Count_Overflow, so that it will take some time before the data is being checked again.

        Put_Header(0,0);
        Put_Data(3);
        Put_Data(0);
        Put_Teal();
        Senddata();
      }
    }
  }
}
      
      
      

///////////////////////////
// End Module Get Data routine.
//
///////////////////////////


///////////////////////////
// Start Module Write Settings routine.
//
///////////////////////////

  if(moduleWriteSettingsOverflow> 500)
  {
    moduleWriteSettingsOverflow = 0;
    if(moduleWriteSettings)
    {
      
       // Write settings command.
      Put_Header(moduleWriteSettingsPoint,0); // Address = the pointer,  Command = 1, it comes from the main module. meant for a slave.
      Put_Data(2); // Sub_cmd = 2, write settings.
      //Put_Data(Balancing_Rate);
      Put_Data(settingbTime);
      Put_Data(settingbThreshold);
      Put_Data(settingcutoffVoltage & 255);
      Put_Data((settingcutoffVoltage >> 8) & 255); 
      Put_Teal();
      Senddata();

      moduleWriteSettingsPoint++;

      if(moduleWriteSettingsPoint == moduleCount) // Did it run the same number of times as there are modules ?
      {
       moduleWriteSettingsPoint = 0; // Reset the pointer 
       moduleWriteSettingsCount++; // Count the counter + 1
       if(moduleWriteSettingsCount == 1) // Did it run 1 complete loop ?
       {
        moduleWriteSettingsCount = 0; // Reset the counter 
        moduleWriteSettings = 0; // Disable the Module_Write_Settings sequence 
        moduleGetData = 1; // Enable the Module_Get_Data Sequence
        moduleGetDataOverflow = 0;  // Reset the Module_Get_Data Overflow.
       }
      }   
    }
  }
///////////////////////////
// End Module Write Settings routine.
//
///////////////////////////
  
  
  
}
