#include <Wire.h>
#include "Adafruit_TPA2016.h"
#include <OSCBundle.h>
#include <OSCMessage.h>
#include <WiFi101.h>  
#include <WiFiUdp.h>
#include <Servo.h>

#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

Servo servo;
int angle = 0;
int dir =1;

void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC3_Handler();

int timer = 0;// timer base 1ms
bool flag = true;

int isVibrate = 0;
int vibrateFreq = 0;
int vibrateTime = 0;

int recieve_msg = 0;

Adafruit_TPA2016 audioamp01 = Adafruit_TPA2016();

//wifi and osc setting
int status = WL_IDLE_STATUS;
char ssid[] = "art333w-nb";
char pass[] = "0928010029";
int keyIndex = 0;

IPAddress sendToUnityPC_Ip(192, 168, 137, 66);
unsigned int sendToUnityPC_Port = 8000;
unsigned int listenPort = 9000; 

char packetBuffer[255];
char ReplyBuffer[] = "acknowledged";

WiFiUDP Udp_send;
WiFiUDP Udp_listen;

const int sensorPin = A0;

void setup() {
  servo.attach(9);
  //pinMode setting
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, INPUT);    
  //interrupt setting
  startTimer(20000); //1ms  //1-> 1s //100->10ms
  
  //audioamp setting
  Serial.begin(9600);
  audioamp01.begin();
  audioamp01.enableChannel(true, true);
  audioamp01.setLimitLevelOff();
  audioamp01.setAGCCompression(TPA2016_AGC_OFF);
  Serial.print("hello!");
  //initial wifi
  initWifi();
}

void loop() {
   //tone(12, 440, 1000);

  // Arduino to Unity
  OSCMessage msg("/1/fader1");
  msg.add((int)analogRead(sensorPin));
  msg.add((int)analogRead(1));
 // msg.add((int)analogRead(sensorPin));
//    msg.add((int)analogRead(sensorPin1));
//    msg.add((int)analogRead(sensorPin2));
//    msg.add((int)analogRead(sensorPin3));
    
  Udp_send.beginPacket(sendToUnityPC_Ip, sendToUnityPC_Port);
  msg.send(Udp_send);
  Udp_send.endPacket();
  msg.empty();
  delay(10);
  recieve_msg = 0;

  // Unity to Arduino
  OSCMessage messageIn;
  int size;
  int stringLength;
  if((size = Udp_listen.parsePacket())>0)
  {
    while(size--)
      messageIn.fill(Udp_listen.read());

    if(!messageIn.hasError())
    {
      recieve_msg = 1;
      Serial.print(messageIn.getInt(0));Serial.print(" | ");
      Serial.print(messageIn.getInt(1));Serial.print(" | ");
      Serial.print(messageIn.getInt(2));Serial.print(" | ");
      Serial.print(messageIn.getInt(3));Serial.print(" | ");
      Serial.print(messageIn.getInt(4));Serial.print(" | ");
      Serial.print(messageIn.getInt(5));Serial.print(" | ");
      Serial.print(messageIn.getInt(6));Serial.print(" | ");
      Serial.print(messageIn.getInt(7));Serial.print("\n");
      isVibrate = messageIn.getInt(0);
      vibrateFreq = messageIn.getInt(1);
      //Serial.print("Freq");
      //Serial.print(vibrateFreq);
      //Serial.print("\n"); 
      int LEDonoff = messageIn.getInt(4);
      if(LEDonoff == 1 )
        digitalWrite(11, HIGH);
      else if(LEDonoff == 0)
        digitalWrite(11, LOW);

      if(messageIn.getInt(5) ==1 )
        tone(12,messageIn.getInt(6),messageIn.getInt(7));
      if(messageIn.getInt(3) ==1 ){
        if(angle>180) dir = -1;
        if(angle<0) dir = 1;
        angle += dir;
        servo.write(angle);
        delay(1);
        }

      vibrateTime = messageIn.getInt(2);
    }  
  }
 
  audioamp01.setGain(30);
  vibrate(12,vibrateFreq,vibrateTime);    
}

void setTimerFrequency(int frequencyHz) {
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  TcCount16* TC = (TcCount16*) TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  Serial.println(TC->COUNT.reg);
  Serial.println(TC->CC[0].reg);
  while (TC->STATUS.bit.SYNCBUSY == 1);
}

void startTimer(int frequencyHz) {
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3) ;
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

  TcCount16* TC = (TcCount16*) TC3;

  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Set prescaler to 1024
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  setTimerFrequency(frequencyHz);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  NVIC_EnableIRQ(TC3_IRQn);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
}

void TC3_Handler() {
  TcCount16* TC = (TcCount16*) TC3;
  // If this interrupt is due to the compare register matching the timer count
  // we toggle the LED.
  if (TC->INTFLAG.bit.MC0 == 1) {
    TC->INTFLAG.bit.MC0 = 1;
    // Write callback here!!!
    if(isVibrate == 1)
      timer ++;
    else
      timer =0;
    //if(timer>30000)
      //timer = 0;
  }
}

void vibrate(int pinNum, int half_wavelength,int vibrateTime){
  if(timer < vibrateTime && isVibrate == 1){
    if((timer/half_wavelength)%2 ==0)
      digitalWrite(pinNum,HIGH);
    else 
      digitalWrite(pinNum,LOW);
  }
  else{
    isVibrate = 0;
  }
}

void printWifiStatus() 
{
  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void initWifi()
{ 
  //Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) 
  {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) 
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  Udp_send.begin(sendToUnityPC_Port);
  Udp_listen.begin(listenPort);
}
