//FYI I used this hx711 library 
//     URL: https://github.com/RobTillaart/HX711


//Screen Stuff
#include <Adafruit_SSD1306.h>
//#include <splash.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <EEPROM.h> //This is being used to store the target weight so when arduino power cycles it retains last used weight

//this is the little board that comes with each load cell
#include "HX711.h"
HX711 scale;
HX711 scale2;

// pins for the first load cell
uint8_t dataPin = 5;
uint8_t clockPin = 6;

// pins for the second load cell
uint8_t dataPin2 = 11;
uint8_t clockPin2 = 12;

//the variable to hold a value for each load cell (float because it is a decimal)
float w1;
float w2;

//Obviously we use this stuff for weight
float currentWeight;
float lastWeight;
bool weightReached = false;
float targetWeight;
float weightOffset = 1.2; // This value is how many grams before the target should the pump shut off, you need to test

//This stuff is all related to timing
const unsigned long eventInterval = 100; 
const unsigned long reachedInterval = 3000; // how long to keep reading and updating the weight after the target weight is reached (btw 1000 = 1s)
unsigned long previousTime = 0;
unsigned long startCounting; 
float elapsedTime;
unsigned long currentTime;
float tV = 0.0;

//This is the weight that arduino will default to when powered on
int storedWeight;


//This is rudamentary state logic
bool started = false;
bool preStarted = false;


//-------------- the rotary encoder  //--------------
int buttonPin = 4;
int buttonPinState = 1;
int buttonPinLast = 1;
bool buttonPressed = 0;
int pinA =3; 
int pinB = 2;
int pinAStateCurrent = HIGH;
int pinAStateLast = pinAStateCurrent;
bool turnDetected = false;


//-------------- The Relays //--------------
int threeWay = 9;
int pump = 10;

//Helper functions that turn the relays on and off 
void extractOn(){
  digitalWrite(pump, HIGH);
  digitalWrite(threeWay, HIGH);
  }

void extractOff(){
  digitalWrite(pump, LOW);
  digitalWrite(threeWay, LOW);
  }



//-------------- These are the different layouts for each screen that we will use, helps keep our code more readable to break them out like this//-------------- 


//When you first turn the machine on
void preStartedDisplay(){
  display.setTextSize(1);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);   
  display.print("Target Weight");
  display.setTextSize(3);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 14);   
  display.print(targetWeight, 1);


//if you've pulled a shot already put the info on screen (if TV isnt 0 you've pulled a shot)  
  if (tV !=0){   
    display.setTextSize(1); // this is size..  1 is 8 pixels tall, 2 is 16, and so on
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 44); // this is position in (x,y)
    display.print("Previous Weight"); //display.print sends this to buffer
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

//-------------- when the shot is being pulled this is what is shown
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

//-------------- This is identical to when a shot is pulled only we switch to a more precise method of reading the scale (called lastWeight)

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


//-------------- This is just that text "starting" that appears after you click the encoder but before the pump is on.


void startingDisplay(){
  display.clearDisplay();
  display.setTextSize(2);     
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(15,18);   
  display.print("Starting");
  display.display();
  }


//-------------- This function adds or subtracts .1 to the target weight if you turn left or right
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

//--------------  This function checks to see if the button was pressed
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
//-------------- This is the interupt that we create in our setup, the conditional statement means if you rotate the encoder while a shot is extracting it wont run any code and cause lag

void update(){
  if(preStarted) {
    increment();
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

//This is seeing if this is the first time you are writing to the eeprom and if it is just put a 0 for now
  if (EEPROM.read(256) != 123){
    EEPROM.write(256, 123);
    storedWeight = 0;
  }

//This code runs every time the arduino powers back on... it sets the target weight to the last weight used so that it "remmbers"
  else {
    EEPROM.get(0, storedWeight);  
  }
//  This is a trick to convert our int back into a float, eeprom floats cant go very high because you only have like a byte so we store the weigth as an int and convert it back 
  targetWeight = storedWeight*0.1;

//setting up the relays
   pinMode (pump, OUTPUT);
   pinMode(threeWay, OUTPUT);

//making sure the pump and valve are off, probably dont need but just in case   
   extractOff();

// inital display 
  display.clearDisplay();
  preStartedDisplay();
  display.display();

//  This is our rotary encoder setup
  pinMode(buttonPin, INPUT_PULLUP); // initialize the button pin as a input
  pinMode (pinA, INPUT_PULLUP);
  pinMode (pinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinA), update, CHANGE);


//  This initiallizes the scales
  scale.begin(dataPin, clockPin);
  scale2.begin(dataPin2, clockPin2);

//This is the callibration value for each load cell, you need to calibrate 
//them with a known weight The good thing is that the value is linear so if 
//you know somethign weights 100g you can calculate what the value should be. 
  scale.set_scale(1091); 
  scale2.set_scale(1105);

//set the scales to 0
  scale.tare();
  scale2.tare();

//Set the state of the system
  preStarted = true;
}

//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------
//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------

void loop(){

//check to see if extraction is running and perform a get weigth if it is
    if ((preStarted == false) && (!weightReached)){
        scaleFunc();
    }
// check to see if the shot just ended and switch to doing multiple averaged readings for more accuracy
    if ((preStarted == false) && (weightReached)){
        precisionScale();
    }


  currentTime = millis();
//check if button is pressed and run the button detect code we defined earlier 
  buttonDetect();

//  ––––––––––––––––––––––––––––––––––––––––

//This is the code that updates the time and weight value while the extraction is happening
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

//This is the code that updates the weight for a few more moments just after the shot has stopped extracting and timer has stopped
  if (weightReached && started){
    extractOff();
    display.clearDisplay();
    justEndedDisplay();
    display.display(); 
  if(currentWeight>lastWeight){
      lastWeight = currentWeight;
      }
    }
  //This is the code that goes back to the inial standby screen when you are finished extracting so you can set another weight
  if(!started){
    display.clearDisplay();
    preStartedDisplay();
    display.display();
  }


//This is the code to stop the extraction once the target weight is reached. 
//I added the conditional to only runs 5s after extraction starts because 
//sometimes you need to reposition your cup and that would trip 
//the weight and stop extraction and ruin your shot :(

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

//  this code runs after your weight is reached AND a certain extra 
//amount of time has passed, you know for the last few drips... 
//it resets the states so you are ready to brew again... 
// e.g you can change it to 5s if you update your reached interval value to 5000 at the top.
    if( currentTime - previousTime >= reachedInterval){
        started=0;
        weightReached = false;
        preStarted = true;
        currentWeight=0;
        }


//  Post pump shutoff / residual flow compensation (WIP) ––––––––––––––––––––––––––––––––––––––––        

//the idea behind this is if your shot is reaching weight quickly, cut the pump more grams before the target weight
//and the other way around, if your shot is coming out slow, only cut the pump just before the target. 
// There is a smarter way to do this, I haven't discoverd it yet.

 if (tV <= 20){
  weightOffset = 3.0;
  }   
 if ((tV >20) && (tV<= 22)){
  weightOffset = 1.9;
  }   
 if ((tV >22) && (tV<= 24)){
  weightOffset = 1.5;
  }   
 if ((tV >24) && (tV<= 26)){
  weightOffset = 1.2;
  }   
 if ((tV >26) && (tV<= 28)){
  weightOffset = 1;
  }   
 if ((tV >28) && (tV<= 30)){
  weightOffset = .9;
  }   
 if ((tV >30) && (tV<= 33)){
  weightOffset = 1;
  }   
 if (tV >33){
  weightOffset = .75;
  }   

    
}//This is the end of our loop

//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------
//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------//--------------

//A few helper functions

//This asks the scales to get the weight and add them up 
void scaleFunc(){
  w1 = scale.get_units();
  w2 = scale2.get_units();
  if((w1+w2)>1){
    currentWeight=(w1+w2);
   }
}
//This asks the scales to check the weight twice so in theory its more accurate, this is how the shot is weighed just after the pump is off
void precisionScale(){
  w1 = scale.get_units(2);
  w2 = scale2.get_units(2);
  currentWeight= (w1+w2);
  }

//We tare the cells just after the rotary knob is clicked, this helper function is almost not worth writing, but whatevs
void tareScales(){
  scale.tare();
  scale2.tare();
  }

//--------------
