/*
 * 
 * v2
 * smart home system
 * - Removed the delay(2000) and used time diff delay.
 * 
 * 
 * v3
 * - fixed payload length
 * - properly calibrate energy reading...
 * - concatenate the energy reading into a 2 dp char array...
 * 
 * 
*/

#include <SoftwareSerial.h>

#define SAMPLES 100
#define ACS_Pin A0
#define BLUE 4
#define lon     digitalWrite(BLUE, HIGH)
#define loff    digitalWrite(BLUE, LOW)

SoftwareSerial Fieneryc(8,9);

char f_read = ' ';
char s_read = ' ';

char enUnit = 'u'; // k = kWh | u = Wh

char charCurrEnergy[7] = {' ',' ',' ',' ',' ',' ',' '};

String stringSRead = "";
String stringFRead = "";
String THIS_DEV_ADDR    = "AT+ADDRESS=8\r\n";
String OTHER_DEV_ADDR   = "AT+ADDRESS=13\r\n"; 
String PARAMETER        = "AT+PARAMETER=7,3,4,5\r\n";
String sendCommand      = "";

int isSRead = 0, isFRead = 0;
byte isS1=0, isS2=0, isL=0, S1 = 6, S2 = 7, L = 5, isBlink = 0;
int randEnergy = 0;

double currPower = 0.0, currEnergy = 0.0;
float Amps_Peak_Peak, Amps_RMS;
const float Volt_RMS = 169.7056;

unsigned long energyTime = millis(), 
              sendTime = millis(), 
              serialUpdateTime = millis(), 
              fienerycUpdateTime = millis(),
              blinkTime = 0;
unsigned int updateTime = 1000;

void switchState(byte pin, int state){
  if (state) {
    Serial.print("Setting pin "); Serial.print(pin); Serial.println("to HIGH");
    digitalWrite(pin, LOW);
  }
  else {
    Serial.print("Setting pin "); Serial.print(pin); Serial.println(" to LOW");
    digitalWrite(pin, HIGH);
  }
}

void switchAll(){
  switchState(S1, isS1);
  switchState(S2, isS2);
  switchState(L, isL);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Fieneryc");
  
  Fieneryc.begin(9600);
  randomSeed(analogRead(0));

  //Setup comm
  while(!Fieneryc);
  Fieneryc.print("AT\r\n");
  delay(1);
  Fieneryc.print(OTHER_DEV_ADDR);
  delay(1);
  Fieneryc.print("AT+PARAMETER=7,3,4,5\r\n");
  delay(1);

  randEnergy = random(176);

  pinMode(S1,OUTPUT);  //l
  pinMode(S2,OUTPUT);  //s1
  pinMode(L,OUTPUT);  //s2
  pinMode(BLUE,OUTPUT);

  switchAll();

  pinMode(ACS_Pin, INPUT);  //current reader...
}

void readSerial(){
  while(Serial.available()>0){    
    s_read = Serial.read();         
    if (!((s_read <= 0) || (s_read == '\r') || (s_read == '\n'))){
      stringSRead += s_read; 
      isSRead++;
    }    
   }
}

void readFieneryc(){
  while(Fieneryc.available()){
    f_read = Fieneryc.read();
    stringFRead += f_read;
    isFRead++;
  }
}

void readCurrent(){
  
  int cnt;   
  float High_peak = 0, Low_peak = 1024;

  for(cnt=0 ; cnt<SAMPLES ; cnt++){

    float ACS_Value = analogRead(ACS_Pin); 
    if(ACS_Value > High_peak){
       High_peak = ACS_Value; 
    }
        
    if(ACS_Value < Low_peak){
       Low_peak = ACS_Value;  
    }
  }      

  Amps_Peak_Peak = High_peak - Low_peak;  

}

void calcEnergy(){
   readCurrent();
   Amps_RMS   = Amps_Peak_Peak*0.3536*0.06;
   currPower  = Amps_RMS*Volt_RMS;
   currPower /= 3600;
   if (enUnit == 'u'){
      currEnergy += currPower;

      if (currEnergy > 1000.00){
         currEnergy /= 1000.00;
         enUnit = 'k';
      }
   }

   if (enUnit == 'k'){
      currEnergy += currPower/1000.00;  

      if (currEnergy > 1000.00){
         currEnergy /= 1000.00;
         enUnit = 'm';
      }
   }

   if (enUnit == 'm'){
      currEnergy += currPower/1000.00;  

      if (currEnergy > 1000.00){
         currEnergy = 0.00;
         enUnit = 'u';
      }
   }

   dtostrf(currEnergy, 6, 2, charCurrEnergy);
   
}

void loop() {

   if(millis() - serialUpdateTime>=updateTime){
    readSerial();
    serialUpdateTime = millis();
   }   

   if(isSRead) {
    Serial.print("Serial:|(");
    Serial.print(isSRead); Serial.print(" bytes read)|");
    Serial.println(stringSRead);
    stringSRead += "\r\n";
    Fieneryc.print(stringSRead);
    
    isSRead = 0; stringSRead = "";    
   }


   if(millis() - fienerycUpdateTime>=updateTime){
    readFieneryc();
    fienerycUpdateTime = millis();
   }

   if (isFRead > 0){
    Serial.print("Fieneryc:|(");
    Serial.print(isFRead); Serial.print(" bytes read)|");//Sent: "); Serial.print(charCurrEnergy); 
    Serial.print("Response:");
    Serial.print(stringFRead);

    //if (stringFRead.equals("+OK\r\n")){
    //  randEnergy = random(176);
    //}

    if (stringFRead.length() > 5){
       switch(stringFRead[9]){
          case 'A': 
                    if(stringFRead[10] == '1') isS1 = 1;
                    else isS1 = 0;
                    switchState(S1, isS1);
                    break;
                    
          case 'B': 
                    if(stringFRead[10] == '1') isS2 = 1;
                    else isS1 = 0;
                    switchState(S2, isS2);
                    break;
                    
          case 'L': 
                    if(stringFRead[10] == '1') isL = 1;
                    else isL = 0;
                    switchState(L, isL);
                    break;
       }

       isBlink = 2;
    }
    
    isBlink = 1;
    lon;
    blinkTime = millis();
    isFRead = 0; stringFRead = "";
   }

  
  
  if (millis() - sendTime >= 10000){
    //send the energy reading...
    sendCommand  = "AT+SEND=13,15,";
    sendCommand += charCurrEnergy; sendCommand += ","; sendCommand += enUnit; 
    sendCommand += ","; sendCommand += isS1; //S1 switch state...
    sendCommand += ","; sendCommand += isS2; //S2 switch state...
    sendCommand += ","; sendCommand  += isL;  //L  switch state...
    sendCommand += ",\r\n";
    Fieneryc.print(sendCommand);
    Serial.println(sendCommand);
    sendTime = millis();
  }

  if (millis() - energyTime >= 1000){
    calcEnergy();
    energyTime = millis();
  }
  
  if (isBlink){
    if (millis() - blinkTime > 300) {
       loff;

       if(isBlink == 2) {
          if (millis() - blinkTime >= 400){
            lon;
            isBlink++;
            blinkTime = millis();
          }
       }

       if (isBlink == 3)
            isBlink = 0;
    }
  }
  
}
