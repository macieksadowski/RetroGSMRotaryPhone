#include "rwt_sim.h"

SoftwareSerial Sim(5, 6);

 String rwt_sim::writeCommand(String const &command)
 {
    String toSend = command + "\r\n";
    Sim.print(toSend);
    buffer = readSerial();
    Serial.println(buffer);
    return buffer;
 }


String rwt_sim::readSerial(){
  timeout=0;
  while  (!Sim.available() && timeout < 12000  ) 
  {
    delay(13);
    timeout++;


  }
  if (Sim.available()) {
 	return Sim.readString();
  }

}

String rwt_sim::updateSerial()
{
  String answer = "";
  boolean tmp=false;
  delay(500);
  while (Serial.available()) 
  {
    Sim.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(Sim.available()) 
  {
    if(!tmp) Serial.write("User: ");
    tmp=true;
    Serial.write(Sim.read());//Forward what Software Serial received to Serial Port
  }
  answer = Serial.readString();
  return answer;
}

bool rwt_sim::initialize()
{
  String tmp;
  Sim.begin(9600);
  buffer.reserve(255);
  //Sim.print(F("ATE1\r\n"));
  writeCommand("ATE1");
  delay(500);
  //Sim.print(F("AT+CBC\r\n"));
  tmp = writeCommand("AT+CBC");
  //buffer = readSerial();
  //Serial.println((tmp.substring(tmp.indexOf("+CBC: ")+10,tmp.indexOf("+CBC: ")+15)));
  delay(500);
  int voltage = (tmp.substring(tmp.indexOf("+CBC: ")+11,tmp.indexOf("+CBC: ")+15)).toInt();
  //Serial.println(voltage);
  if(voltage>3100 && voltage<4100)
  {
    Serial.println("Voltage OK!");

    //Sim.print(F("AT+CSQ\r\n"));
    tmp = writeCommand("AT+CSQ");
    //buffer = readSerial();
    //Serial.println(buffer);
    int signal = tmp.substring(tmp.indexOf("+CSQ: ")+6,tmp.indexOf("+CSQ: ")+8).toInt();
    if(signal>5) 
    {
      Serial.print("Signal OK! (");
      Serial.print(signal);
      Serial.println(" dBm)");

      //Sim.print(F("AT+CFUN=1\r\n")); //Sets full phone functionality
      writeCommand("AT+CFUN=1");
      //readSerial();
      //Sim.print(F("AT+CRSL=0\r\n")); //Sets ringer sound level to 0
      writeCommand("AT+CRSL=0");
      //readSerial();
      //Sim.print(F("AT+CLVL=60\r\n")); //Sets loudspeaker volume level
      writeCommand("AT+CLVL=60");
      //readSerial();
      //Sim.print(F("AT+ECHO=0,65535,65535,30000,30000,1\r\n"));
      writeCommand("AT+ECHO=0,65535,65535,30000,30000,1");

      writeCommand("AT+CMIC=0,4");


      //readSerial();
      
    }
    else 
    {
      Serial.println("No signal!");
    }
  }
  else
  {
    Serial.println("BAD VOLTAGE!");
  } 
  return false;
} 


bool rwt_sim::pickUp()
{
  Sim.print (F("ATA\r\n"));
   buffer= readSerial();
   //Response in case of data call, if successfully connected 
   if ( (buffer.indexOf("OK") )!=-1 ) return true;  
   else return false;
}

void rwt_sim::callNumber(char* number)
{
  Sim.print (F("ATD+ "));
  Sim.print (number);
  Sim.print (";");
  Sim.print (F("\r\n"));
  Sim.print (F("AT+STTONE=1,4,2000\r\n"));
  //Sim.print("ATH");
  //Sim.print (F("\r\n"));
}

bool rwt_sim::hangUp()
{
  Sim.print (F("ATH\r\n"));
  buffer = readSerial();
  if ( (buffer.indexOf("OK") ) != -1) return true;
  else return false;
}

bool rwt_sim::speedDialing(int position)
{
  int memoryPosition = position+20;
  Sim.print (F("ATD> "));
  Sim.print (memoryPosition);
  Sim.print (";");
  Sim.print (F("\r\n"));
  buffer=readSerial();
  if ( (buffer.indexOf("OK") )!=-1 )
  {
    Sim.print (F("AT+STTONE=1,4,2000\r\n"));
    return true;
  }
  else 
  {
    Sim.print (F("AT+STTONE=1,6,3900\r\n"));
    return false;
  }
}

bool rwt_sim::speedDialingProgramming(int position, String number)
{
      int memoryPosition = position+20;
      Sim.print("AT+CPBW=");
      Sim.print(memoryPosition);
      Sim.print(",\"");
      Sim.print(number);
      Sim.print("\",129,\"SWyb ");
      Sim.print(position);
      Sim.print("\"\r\n");
      buffer = readSerial();
      Serial.println(buffer);
      if ( (buffer.indexOf("OK") )!=-1 )
      {
        writeCommand("AT+STTONE=1,17,2000");
        Serial.println("Number saved correctly!");
        return true;
      }
      else 
      {
        writeCommand("AT+STTONE=1,6,3900");
        Serial.println("Error saving number");
        return false;
      }
}

int rwt_sim::getStatus()
{
  // 0: module ready
  // 3: incoming call
  // 4: call in progress
    Sim.print (F("AT+CPAS\r\n"));
    buffer=readSerial();  
    return buffer.substring(buffer.indexOf("+CPAS: ")+7,
      buffer.indexOf("+CPAS: ")+9).toInt();
      //return 0;
}
