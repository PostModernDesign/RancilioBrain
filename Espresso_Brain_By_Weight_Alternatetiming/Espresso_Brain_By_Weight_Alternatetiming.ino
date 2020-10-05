  //

//Built using bits from HX711 Library
//    FILE: HX_kitchen_scale.ino
//  AUTHOR: Rob Tillaart
// VERSION: 0.1.0
// PURPOSE: HX711 demo
//     URL: https://github.com/RobTillaart/HX711
//
// HISTORY:
// 0.1.0    2020-06-16 initial version
//

// to be tested 






//Screen Stuff
#include <Adafruit_SSD1306.h>
//#include <splash.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);




//stand in for ssr
#include <EasyNeoPixels.h>

#include <EEPROM.h> 

#include "HX711.h"


HX711 scale;
HX711 scale2;

uint8_t dataPin = 5;
uint8_t clockPin = 6;

uint8_t dataPin2 = 11;
uint8_t clockPin2 = 12;

float w1;
float w2;
float currentWeight;
float lastWeight;


bool weightReached = false;
float targetWeight;
float weightOffset = 1.2;

const unsigned long eventInterval = 100;
const unsigned long reachedInterval = 3000;
unsigned long previousTime = 0;
unsigned long startCounting;
float elapsedTime;
unsigned long currentTime;



int storedWeight;

float tV = 0.0;

bool started = false;

bool preStarted = false;


//--------------  encoder  //--------------
int buttonPin = 4;
int buttonPinState = 1;
int buttonPinLast = 1;
bool buttonPressed = 0;
int pinA =3; 
int pinB = 2;
int pinAStateCurrent = HIGH;
int pinAStateLast = pinAStateCurrent;
bool turnDetected = false;

//-------------- SSR  //--------------
int threeWay = 9;
int pump = 10;
 
void extractOn(){
  digitalWrite(pump, HIGH);
  digitalWrite(threeWay, HIGH);
  }

void extractOff(){
  digitalWrite(pump, LOW);
  digitalWrite(threeWay, LOW);
  }



//--------------


void preStartedDisplay(){
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);   
  display.print("Target Weight");
  display.setTextSize(3);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 14);   
  display.print(targetWeight, 1);
  
  if (tV !=0){   
    display.setTextSize(1);     
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 44);
    display.print("Previous Weight");
    display.setTextSize(1);     
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(98, 44);   
    display.print(lastWeight, 1);
    display.setTextSize(1);     
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 56);
    display.print("Previous Time");
    display.setTextSize(1);     
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(98, 56);   
    display.print(tV, 1);
  }
}

//--------------

void startedDisplay(){

  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);   
  display.print("Target Weight");
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(85, 0);   
  display.print(targetWeight, 1);
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,22);   
  display.print("Weight");
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(80,22);   
  display.print(currentWeight, 1);
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 46);   
  display.print("Time");
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(80, 46);   
  display.print(tV,1);


  }

//--------------

void justEndedDisplay(){
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);   
  display.print("Target Weight");
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(85, 0);   
  display.print(targetWeight, 1);
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,22);   
  display.print("Weight");
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(80,22);   
  display.print(lastWeight, 1);
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 46);   
  display.print("Time");
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(80, 46);   
  display.print(tV,1);

  }

//--------------



void startingDisplay(){
  display.clearDisplay();
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(16,18);   
  display.print("Starting");
  display.display();
  }


//--------------
void increment(){
    turnDetected = true;
    //Rotation
    pinAStateCurrent = digitalRead(pinA);
    if((pinAStateLast == LOW) && (pinAStateCurrent == HIGH)){
    if(digitalRead(pinB) == HIGH){
          Serial.println("decremented");
          targetWeight -= 0.1;
        delay(15);
      }
    else {
    targetWeight += 0.1; 
    delay(15);
    }
  }
  pinAStateLast = pinAStateCurrent;
}

//--------------
void buttonDetect(){
  buttonPinState=digitalRead(buttonPin);
  if (buttonPinState != buttonPinLast){
     if ((buttonPinState==1) && (!started)) /*I just started extracting*/{
      preStarted = false;
      startingDisplay();
      tareScales();
      delay(250);
      tV=0;
      currentWeight=0;
      elapsedTime=0;
      preStarted = false;
      started=true;
      extractOn();
      startCounting = millis();
      display.clearDisplay();
      }
     if ((elapsedTime>1.0) &&(buttonPinState==1) && (started)) /*I want to stop the extraction manually*/{
      weightReached = true;
      }
    buttonPinLast=buttonPinState;
  }
}
//--------------

void update(){
  if(preStarted) {
    increment();
  }
  else{
//    Serial.println("ignored input, coffee is brewing");
    }
}

//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------
//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------

void setup()
{
  Serial.begin(9600);
  
//display setup
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  
  }

  if (EEPROM.read(256) != 123){
    EEPROM.write(256, 123);
    storedWeight = 0;
  }
  else {
    EEPROM.get(0, storedWeight);  
  }
  targetWeight = storedWeight*0.1;

   pinMode (pump, OUTPUT);
   pinMode(threeWay, OUTPUT);
   extractOff();

  
  display.clearDisplay();
  preStartedDisplay();
  display.display();
   
  pinMode(buttonPin, INPUT_PULLUP); // initialize the button pin as a input
  pinMode (pinA, INPUT_PULLUP);
  pinMode (pinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinA), update, CHANGE);

  
  scale.begin(dataPin, clockPin);
  scale2.begin(dataPin2, clockPin2);

  scale.set_scale(1091); 
  scale2.set_scale(1105);

  scale.tare();
  scale2.tare();
  
  preStarted = true;
}

//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------
//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------

void loop(){

    if ((preStarted == false) && (!weightReached)){
        scaleFunc();
    }
    if ((preStarted == false) && (weightReached)){
        precisionScale();
    }

  currentTime = millis();
  buttonDetect();

//  ––––––––––––––––––––––––––––––––––––––––

  if(started){
      if(!weightReached){
          elapsedTime = ((currentTime - startCounting)*.001);
          tV = elapsedTime;
        previousTime = currentTime;
        display.clearDisplay();
        startedDisplay();
        display.display();      
        }
  }

//  ––––––––––––––––––––––––––––––––––––––––  

  if (weightReached && started){
    extractOff();
    display.clearDisplay();
    justEndedDisplay();
    display.display(); 
  if(currentWeight>lastWeight){
      lastWeight = currentWeight;
      }
    }
  
  if(!started){
    display.clearDisplay();
    preStartedDisplay();
    display.display();
  }

  if (tV>5){
    if ((currentWeight) > (targetWeight-weightOffset)){
       lastWeight=currentWeight;
       weightReached = true;
        if (storedWeight != round(targetWeight/0.1)) {
          storedWeight= round(targetWeight/0.1); 
          EEPROM.put(0, storedWeight);
          }
    }
  }
    if( currentTime - previousTime >= reachedInterval){
        started=0;
        weightReached = false;
        preStarted = true;
        currentWeight=0;
        }


//  Post pump shutoff / residual flow compensation (WIP) ––––––––––––––––––––––––––––––––––––––––        
//
// if (tV <= 20){
//  weightOffset = 3.0;
//  }   
// if ((tV >20) && (tV<= 22)){
//  weightOffset = 2;
//  }   
// if ((tV >22) && (tV<= 24)){
//  weightOffset = 1.8;
//  }   
// if ((tV >24) && (tV<= 26)){
//  weightOffset = 1.6;
//  }   
// if ((tV >26) && (tV<= 28)){
//  weightOffset = 1.4;
//  }   
// if ((tV >28) && (tV<= 30)){
//  weightOffset = 1.2;
//  }   
// if ((tV >30) && (tV<= 33)){
//  weightOffset = 1;
//  }   
// if (tV >33){
//  weightOffset = .75;
//  }   

    
}

//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------
//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------

void scaleFunc(){
  w1 = scale.get_units();
  w2 = scale2.get_units();
  if((w1+w2)>1){
    currentWeight=(w1+w2);
   }
}

void precisionScale(){
  w1 = scale.get_units(3);
  w2 = scale2.get_units(3);
  currentWeight= (w1+w2);
  }

void tareScales(){
  scale.tare();
  scale2.tare();
  }

//--------------
