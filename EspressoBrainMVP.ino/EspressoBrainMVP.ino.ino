
/*
––––––––––––––––––––––
Espresso Timer Intention
––––––––––––––––––––––


___  Current Functions ___ 
– Set a brew time- turn on pump, open solenoid, count down on OLED, Turn off pump close solenoid.

___  Future Functions ___ 
– Menu to change modes and change parameters 
– Optize code becuase running out of space
– Set a preinfusion time and delay time – Open Solenoid, count, close solenoid, count, turn on pump, open solenoid, count down, etc..
– Backflush Mode - 15sec on, 15 sec rest - 5 times
– Manual Mode - Start pump and count up
– Wire up Load Cells
– Target weight mode - set weight and brew up until that weight.
*/

//1st order variables and functions
bool setupDone=0;
bool timerRunning = 0;
#include <EEPROM.h> 


/*–––––––––––––––––––––––––––––
Display Setup
–––––––––––––––––––––––––––––*/
#include <Adafruit_SSD1306.h>
#include <splash.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//LayoutFunctions
void topLeftLabel(String j) {
  display.setTextSize(2);      // scale 2
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corn
  display.print(j);
  }

void middleLeftTime(float j) {
  display.setTextSize(5);      // scale 5
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 24);     // these are coordinates(x, y)
  display.print(j, 1); // in this sketch this parameter is the time value aka (tV)
  }


/*–––––––––––––––––––––––––––––
Encoder Setup
–––––––––––––––––––––––––––––*/


//Tactile Switch Setup
int buttonPin = 4;
int buttonPinState = 1;
int buttonPinLast = 1;
bool buttonPressed = 0;


//Tactile Switch Detection---------------
void buttonDetect(){
  buttonPinState=digitalRead(buttonPin);
  if (buttonPinState != buttonPinLast){
     if ((buttonPinState==1) && (timerRunning==0)){
      buttonPressed=1;
      extractOn();
      }
    buttonPinLast=buttonPinState;
  }
}

//Rotary Encoder Turn Detection
//you need to use these pins because the support interupt
int pinA = 3;
int pinB = 2;
int pinAStateCurrent = LOW;
int pinAStateLast = pinAStateCurrent;
bool turnDetected = false;
 
/*–––––––––––––––––––––––––––––
Timer Setup
–––––––––––––––––––––––––––––*/
//millis Function

const unsigned long eventInterval = 100;
unsigned long previousTime =0;
int storedTime;
float tV;
float countDownDefault;



/*–––––––––––––––––––––––––––––
Menu Setup
–––––––––––––––––––––––––––––*/
//to do one day


/*–––––––––––––––––––––––––––––
Relay Setup
–––––––––––––––––––––––––––––*/

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



//update encoder function
void rotaryIncrement(){
    turnDetected = true;
    //Rotation
    pinAStateCurrent = digitalRead(pinA);
    if((pinAStateLast == LOW) && (pinAStateCurrent == HIGH)){
    if(digitalRead(pinB) == HIGH){
        if(tV>=.1){
          tV-= 0.1;
        }
        countDownDefault = tV;
//        storedTime=countDownDefault/0.1; 
        delay(15);
      }
    else {
    tV += 0.1;
    countDownDefault = tV;
//    storedTime=countDownDefault/0.1; 
    delay(15);
    }
  }
  pinAStateLast = pinAStateCurrent;
}

void update(){
  if((!timerRunning) && (setupDone)) {
    rotaryIncrement();
  }
  else{
//    Serial.println("ignored input, timer is running");
    }
}





void setup() {
  Serial.begin(9600);
//display setup
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }


if (EEPROM.read(256) != 123){
  EEPROM.write(256, 123);
  storedTime = 0;
  }
 else {
  EEPROM.get(0, storedTime);
 }
  tV = storedTime*0.1;

//  tactile setup
  pinMode (buttonPin, INPUT_PULLUP);
  display.clearDisplay();
  topLeftLabel("Shot Time");
  middleLeftTime(tV);
  display.display();



//Rotary Setup
  pinMode (pinA, INPUT_PULLUP);
  pinMode (pinB, INPUT_PULLUP);
  pinMode (buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinA), update, CHANGE);
  countDownDefault = tV;
 
 //SSR Setup
 pinMode (pump, OUTPUT);
 pinMode(threeWay, OUTPUT);
 extractOff();
  setupDone=1;

}




void loop() {

if (turnDetected){
  turnDetected=false;
  display.clearDisplay();
  topLeftLabel("Shot Time");
  middleLeftTime(tV);
  display.display();
  }

//buttonDetection
  buttonDetect();
  /* Updates frequently */
  unsigned long currentTime = millis();
if(buttonPressed){
  timerRunning=1;
  
  if( currentTime - previousTime >= eventInterval){
    if(tV >= 0.1){
      tV -= 0.1;
      previousTime = currentTime;
      display.clearDisplay();
       topLeftLabel("Extracting");
      middleLeftTime(tV);
      display.display();
    }
    else  {
      display.clearDisplay();
      topLeftLabel("Extracting");
      middleLeftTime(0.0);
      display.display();
      extractOff();
//      Serial.println("End countDownDefault: ");
//      Serial.println(countDownDefault);
//      Serial.println("storedTime: ");
//      Serial.println(storedTime);
      timerRunning=0;
      tV=countDownDefault; 
      if (storedTime != round(countDownDefault/0.1)) {
        storedTime = round(countDownDefault/0.1); 
        EEPROM.put(0, storedTime);
//        Serial.println("yes EEPROM: ");
//        Serial.println(storedTime);
        }
      delay(1000);
      buttonPressed=0;
      display.clearDisplay();
      topLeftLabel("Shot Time");
      middleLeftTime(tV);
      display.display();
     }
  }
}
}
