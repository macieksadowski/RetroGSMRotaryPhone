/*NIE DZIAŁA:
 * -natychmiastowe rozłączanie
 * -wybieranie losowych cyfr
 * 
 */


/******************************************************/
/*    DECLARATIONS OF VARIABLES AND INPUTS/OUTPUTS    */
/******************************************************/

#include "rwt_sim.h"
#define PIN_FORK 4
#define PIN_T_KRAN 3
#define PIN_T_STYK 2
#define PIN_RING 8
#define PIN_SYGNALOWY 12


bool forkState = 0;
bool forkStateOld = 0;
bool dialingMode = false;
int dialState = 0;
bool callingProcessStarted = false;
bool callInProgress = false;
volatile word digit;
String number;
int state=2;
static unsigned long timeFromLastInput = 0;
static unsigned long timeForInput = 5000;
rwt_sim gsm;

/********************************************/
/*            INITIAL SETTINGS              */
/********************************************/

void setup()
{
  
  pinMode(PIN_T_STYK, INPUT_PULLUP);
  pinMode(PIN_T_KRAN, INPUT_PULLUP);
  pinMode(PIN_FORK, INPUT_PULLUP);
  pinMode(PIN_RING, OUTPUT);
  pinMode(13,OUTPUT);
  ring(0);
  
  attachInterrupt(digitalPinToInterrupt(PIN_T_STYK), dialerIsTurning, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_T_KRAN), wybierak_start, RISING);
  Serial.begin(9600);
  gsm.initialize();
 
}

void loop()
{
  forkState = digitalRead(PIN_FORK);
  unsigned long actualTime = millis();
  state = gsm.getStatus();
  Serial.print("STATE = ");
  Serial.print(state);
  Serial.print("  D.M. = ");
  Serial.print(dialingMode);
  
  Serial.print("  D.S. = ");
  Serial.print(dialState);
  Serial.print("  C.I.P. = ");
  Serial.print(callInProgress);
  Serial.print("  F.S = ");
  Serial.print(forkState);
  Serial.print("  F.S.O. = ");
  Serial.println(forkStateOld);
  Serial.print("DIGIT = ");
  Serial.print(digit);
  Serial.print("  NUMBER = ");
  Serial.println(number);

  /*IF USER HANGS UP - jezeli odlozono sluchawke*/
  if(forkState == 0 && forkStateOld ==1)
  {
	
    /*IF CALL WAS IN PROGRESS - jezeli trwala rozmowa*/
    if(callInProgress && (state ==0 || state ==4))
    {
      callInProgress = false;
      gsm.hangUp();
      number="";
      digit=0;
    }
    
  }
  
  /*IF USER PICKS UP - jezeli podniesiono sluchawke*/
  if(forkState == 1 && forkStateOld == 0)
  {
    /*IF INCOMING CALL - jezeli pol. przychodzace*/
    if(state == 3)
    {
      /*IF NOT DIALING MODE - jezeli nie wybierano numeru*/
      if(!dialingMode)
      {
		    gsm.pickUp();
        digit=0;
        ring(0);
        number="";
        callingProcessStarted = false;
        callInProgress = true;  
      }
      /*IF DIALING MODE - jezeli wybierano numer*/
      if(dialingMode)
      {
        ring(0);
        gsm.hangUp();
      }
    }    
  }
  
  /* IF HANG UP - jeżeli odlozona*/
  if(!forkState && !forkStateOld)
  {
    /*IF INCOMING CALL - jezeli pol. przychodzace*/
    if(state == 3)
    {
      ring(1);
    }
  }
   /* IF PICK UP - jeżeli podniesiona*/
  if(forkState && forkStateOld)
  {
     
    /*IF DIALING - jezeli wybierano numer*/
    if(state == 0)
    {
   
      dialState = dialing();
      if(dialState == 1)
      {
    
        gsm.writeCommand("AT+SIMTONE=0,425,200,0,5000");
        char charBuf[50];
        number.toCharArray(charBuf,50);
        Serial.print("Calling number ");
        Serial.println(charBuf);
        if(!callingProcessStarted)
        {
          number="";
          gsm.callNumber(charBuf);
          callingProcessStarted = true;
        }
      }
      else if(dialState == 2)
      {
        gsm.writeCommand("AT+SIMTONE=0,425,200,0,5000");
        if(!callingProcessStarted)
        {
          if(!gsm.speedDialing(number.toInt())) number="";  
          callingProcessStarted = true;
        }    
    }

    
    /*IF CALL IN PROGRESS - jezeli pol. trwa*/
    if(state == 4)
    {
      digit=0;
      dtmf();
      callingProcessStarted = false;
      callInProgress = true;
      
    }
  }
  } 
  forkStateOld = forkState;

}
/***************************************************/
/*            INTERRUPTIONS FUNCTIONS              */
/***************************************************/

void dialerIsTurning()
/* This function is triggered when rotary dialer is turning.*/
{
  if (forkState)
  {
  static unsigned long lastTime;
  unsigned long timeNow = millis();
  if (timeNow - lastTime < 130) return;
  digit++;
  lastTime = timeNow;
  }
}


void wybierak_start()
/*This function is trigerred when user inputs a digit on rotary dialer*/
{
  if (forkState)
  {
    static unsigned long lastTime;
    unsigned long timeNow = millis();
    if (timeNow - lastTime < 500) return;
    if (digit != 0)
    {
      if (digit == 10) digit = 0;

      number = number + String(digit);
    }

    
    digit = 0;
    lastTime = timeNow;
    timeFromLastInput = millis();
  }
}

int dtmf()
{
    if(millis() - timeFromLastInput > 2000 && timeFromLastInput != 0)
    {
      if(number.length() > 0)
      {
        
         int dtmf = number.toInt(); 
         String dtmfStr =""; 
         
        
         if(dtmf >= 0 && dtmf <9)
          {
          dtmfStr = String("AT+CLDTMF=7,\"") + String(dtmf) + String("\"");
          
          }
         else if(dtmf == 11) 
         {
          dtmfStr="AT+CLDTMF=7,\"*\"";
          
         }
         else if(dtmf == 22) 
         {
          dtmfStr="AT+CLDTMF=7,\"#\"";
          
         }
         gsm.writeCommand(dtmfStr);
         number="";
      }
    }
 
  
}


int dialing()
/*
Returns:
-1 = error dialing
0 = fork down
1 = user input number
2 = user used speed dialing
3 = speed dialing was programmed
*/
{
    if(dialingMode == false)
    {
      digit=0;
      number="";
    }
    if(number.length() < 1)    gsm.writeCommand("AT+SIMTONE=1,425,200,0,2000");
    else gsm.writeCommand("AT+SIMTONE=0,425,200,0,200");
    dialingMode = true;
    

    if(millis() - timeFromLastInput > timeForInput && timeFromLastInput != 0)
    {
      if(number.startsWith("00"))
      {
        Serial.println("International number!");
        number = number.substring(2);
        number = "+" + number;
        Serial.println("Calling number...");
        return 1;
      }
      else if(number.startsWith("0"))
      {
        int position = number.substring(1, 2).toInt();
        String speedDialNumber = number.substring(2);
        Serial.println("Speed Dialing Programming:");
        Serial.print("Digit= ");
        Serial.println(position);
        Serial.println("Number= ");
        Serial.println(speedDialNumber);
        if(position > -1 && position < 10 && speedDialNumber.length() == 9)
        {
          Serial.println("Number inputed correctly!");
          
          gsm.speedDialingProgramming(position,speedDialNumber);
        }
        else
        {
          gsm.writeCommand("AT+STTONE=1,6,2000");
          Serial.println("Error - wrong number!");
        }
        number = "";
        return 3;
      }
      else if(number.length() == 1)
      {
        Serial.print("Speed dialing, position ");
        Serial.println(number);
        return 2;
      }
      else if(number.length() < 10 && number.length() > 1)
      {
        number = "+48" + number;
        Serial.println("Calling number...");
        return 1;
      }
      else
      {
        Serial.println("Error dialing!");
        
        number = "";
        return -1;
      }
    
  }
  else
  {
    //gsm.writeCommand("AT+SIMTONE=0,425,200,0,5000");
    ring(0);
    dialingMode = false;
    timeFromLastInput = 0;
    digit=0;
    number = "";
    callingProcessStarted = false;
    
    return 0;
  }
}

void ring(bool state)
{
  if(state == 1)
  {
    
    digitalWrite(PIN_RING,LOW);
      for(int i=0;i<6;i++)
      {
        if(forkState == 1 && forkStateOld ==0) break; 
        delay(500);
      }
      digitalWrite(PIN_RING,HIGH);
      

  }
  else
  {
    
    digitalWrite(PIN_RING,HIGH);
  }
}
