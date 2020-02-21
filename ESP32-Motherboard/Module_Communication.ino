// Communication Vartiables Serial-1
byte Tx_Buffer[80];
byte Rx_State = 0;
byte DLE = 16;
byte STX = 2;
byte ETX = 3;
byte Rx_Buffer[100];
byte Rx_Byte = 0;
byte New_Cmd = 0;
int Tx_Put_Point = 0;
int Rx_Count = 0;
int Rx_Point = 0;
char state = 0;

float moduleVoltages[10][8]; // 10 Modules maximum, and 8 voltages per module
byte  moduleCellToDump[10];
byte  moduleCellToReceive[10];
void Check_String()
{
  byte Slave_Address = Rx_Buffer[0];
  byte Slave_Cmd = Rx_Buffer[1];
  byte Slave_Sub_Cmd = Rx_Buffer[2];

if(Slave_Cmd == 0 && Slave_Sub_Cmd == 3)
  {
    //Serial.println("Command 3 received from module");
    //Serial.println (Rx_Buffer[3]);
      byte count = Rx_Buffer[3];  
      // Module have been adding this number up,
      //so the amount of times this number has pased a module = the amount of modules present on the bus.
      
      moduleCountFound = 1;
      moduleCount = count;
  }

  if(Slave_Cmd == 1)
  {
      switch(Slave_Sub_Cmd)
    {
      case 0:
      {
        break;
      }
    
      

      case 1: // Get_Data
      {
        // Get battery voltages.
        // Cell1 is the lower cell.
        // Cell2 is the higher cell.
        for(int i = 0; i < 8; i++)
        {
          byte low = Rx_Buffer[i+3];
          byte high = Rx_Buffer[i+4];
          int cell = cell | low;
          cell = cell | (high << 8);
          float Cell = cell;
          Cell = Cell / 100;
          moduleVoltages[Slave_Address][i] = Cell;
        }

        cellCount = 0;
        for(int i = 0; i < 10; i++)
        {
          for(int j = 0; j < 8; j++)
          {
            if(moduleVoltages[i,j]) // If there is a voltage.
            {
              cellCount++;
            }
            else
            {
              return;
            }
          }
        } 
        moduleCellToDump[Slave_Address] = Rx_Buffer[12];
        moduleCellToReceive[Slave_Address] = Rx_Buffer[13];
        

        // Balance Current will be in this next part. 
        // No code for this has been written at the other end yet.

        break;
        }
      case 2: // Write Settings
      {
        break;
      }

      
    }
  }
    Rx_Point = 0;
    Rx_Count = 0;
}


////////////////////////////
////////////////////////////
void Put_Data(byte data) // Functie die je bytes in op eenvolgende index doet
 {
   Tx_Buffer[Tx_Put_Point] = data;
   Tx_Put_Point++;
   
   if (data == DLE)
   {
     Tx_Buffer[Tx_Put_Point] = DLE;
     Tx_Put_Point++;
   }// if data == dle endd
 }// void put_data endd

 //////////////////////////////////////////////////////////////////////////////

 void Put_Header(byte Address,byte Cmd)

 {
   Tx_Buffer[0] = DLE;
   Tx_Buffer[1] = STX;
   Tx_Buffer[2] = Address;
   Tx_Put_Point = 3;
   Put_Data(Cmd);
 }// void put_header endd

 ////////////////////////////////////////////////////////////////////////////

 void Put_Teal()
 {
   Tx_Buffer[Tx_Put_Point] = DLE;
   Tx_Put_Point = Tx_Put_Point + 1;
   Tx_Buffer[Tx_Put_Point] = ETX;
   Tx_Put_Point = Tx_Put_Point + 1;
 } // void put_teal endd


 /////////////////////////////////////////////////////////////////////////////

 void Senddata()
 {
   int i;
   for(i = 0; i < Tx_Put_Point; i++) // for loop die je index laat oplopen
   {
     Serial1.write(Tx_Buffer[i]); // je hele array serieel versturen
   }
   Tx_Put_Point = 0;
 } // void senddata endd

 /////////////////////////////////////////////////////////////////////////////

 void ReadBuffer()
 {
   
   

   Rx_Byte = Serial1.read();
   
   
   //Uartwrite(Rx_Byte);
   switch (Rx_State)
   {
     case 0 :
     Rx_Point = 0;
     Rx_Count=0;
     
     if (Rx_Byte==DLE) Rx_State++;        // search DLE+STX for start of string

     break;
     case 1 :
     if (Rx_Byte==STX) Rx_State++; else Rx_State=0;
     break;
     case 2 :
     if (Rx_Byte!=DLE)
     { // If Received != DLE Begin
       Rx_Buffer[Rx_Point]=Rx_Byte;
       Rx_Count++;
       Rx_Point++;            // count number of characters received
       
     } // If Received != Eind
     if (Rx_Byte==DLE) Rx_State++;
     break;
     
     case 3 :
     if (Rx_Byte==DLE)
     {  // If Received == DLE Begin           // expanded DLE received
       Rx_Buffer[Rx_Point]=Rx_Byte;
       Rx_Count++;
       Rx_Point++;            // count number of characters received
       Rx_State=2;
     } // If Received == DLE Eind
     
     if (Rx_Byte==ETX)
     { // If received == ETX Begin
       Check_String();
  

       Rx_State=0;
       
       
     } // If received == ETX Eind
     
     
     
     if ((Rx_Byte!=DLE) & (Rx_Byte!=ETX)) // string error
     { // If Stringerror Begin
       New_Cmd = 0;
       Rx_State=0;
       
     } // If Stringerror Eind
     break;
     
   }//switch endd

 }
