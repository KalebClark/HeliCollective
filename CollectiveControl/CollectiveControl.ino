#include <ClickEncoder.h>
#include <TimerOne.h>

#include <Joystick.h>

#include "ACE128.h"
#include "ACE128map12345678.h"
#include <Wire.h>

#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 mcp;
#define ACE128_ARDUINO_PINS

//ACE128 encoder(uint8_t{4,5,6,7,12,13,10,11}, (uint8_t*)encoderMap_87654321);
ACE128 myACE(4,5,6,7,12,13,10,11, (uint8_t*)encoderMap_12345678);
//int16_t multiturn_encoder_value;
int16_t axisVal;

//Joystick_ Joystick;
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK,
  14,     // Number of Buttons
  1,      // Number of Hat Switches
  true,   // includeXAxis
  false,  // includeYAxis
  false,  // includeXAxis
  false,  // includeRxAxis
  false,  // includeRyAxis
  false,  // includeRzAxis
  false,  // includeRudder
  false,  // includeThrottle
  false,  // includeBrake
  false  // includeSteering
);
// Handle Encoder
ClickEncoder *handleEnc;
int16_t last, value;
void timerIsr() { handleEnc->service(); }

// Toggle Switch Array
int toggleState[] = {0,0,0,0,0,0};
int togglePins[] = {0,1,2,3,4,5};
int toggleBtn[] =  {5,6,7,8,9,10};
int toggleLimit = 10;

int debugCount = 100;

void setup() {
  // Setup Serial output
  Serial.begin(19200);
  Serial.println("Collective controller startup");
  
  // Setup MCP23017
  mcp.begin();

  // Set Toggle Switch Pin Modes
  mcp.pinMode(togglePins[0], INPUT); mcp.pullUp(togglePins[0], HIGH);
  mcp.pinMode(togglePins[1], INPUT); mcp.pullUp(togglePins[1], HIGH);
  mcp.pinMode(togglePins[2], INPUT); mcp.pullUp(togglePins[2], HIGH);
  mcp.pinMode(togglePins[3], INPUT); mcp.pullUp(togglePins[3], HIGH);
  mcp.pinMode(togglePins[4], INPUT); mcp.pullUp(togglePins[4], HIGH);
  mcp.pinMode(togglePins[5], INPUT); mcp.pullUp(togglePins[5], HIGH);


  // Push Button Pin Modes
  mcp.pinMode(15, INPUT); mcp.pullUp(15, HIGH); // Button 1
  mcp.pinMode(14, INPUT); mcp.pullUp(14, HIGH); // Button 2
  mcp.pinMode(13, INPUT); mcp.pullUp(13, HIGH); // Button 3
  mcp.pinMode(12, INPUT); mcp.pullUp(12, HIGH); // Button 4

  // Setup Axis encoder
  myACE.begin();

  // Setup Handle Encoder
  handleEnc = new ClickEncoder(8, 9, A5, 4);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  last = -1;

  // Hat Switch
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);

  // Setup Joystick
  Joystick.setXAxisRange(0, 1024);

  Joystick.begin();
  
}

void toggle(int idx) {
  int pin = togglePins[idx];
    
  if ( mcp.digitalRead(pin) == LOW) {
    toggleState[idx] == 1;
    if(toggleState[idx] == 1) { 
      Joystick.setButton(toggleBtn[idx], 1);
    }
    if(toggleState[idx] <= toggleLimit) {
      toggleState[idx]++;
      Serial.print("TOGBTN("); Serial.print(toggleBtn[idx]); Serial.println(") ON");
    }

    if(toggleState[idx] >= toggleLimit) {
      Joystick.setButton(toggleBtn[idx], 0);
    }
  } else if ( mcp.digitalRead(pin) == HIGH) {
    toggleState[idx] = 0;
  }
}

void setHatSwitch(int switchNum, int dir) {
  Joystick.setHatSwitch(switchNum, dir);
  Serial.print("HAT: "); Serial.print(switchNum); Serial.print(" = "); Serial.println(dir);
}

int hEncTimer = 0;
int hEncTimerLimit = 15;

void loop() {

  // Read Hat Switch : 0, 45, 90, 135, 180, 225, 270, 315 
  if ((digitalRead(A2) == LOW) && (digitalRead(A0) == LOW)) { setHatSwitch(0, 315); } else // upLeft
  if ((digitalRead(A2) == LOW) && (digitalRead(A1) == LOW)) { setHatSwitch(0, 45); } else // upRight
  if ((digitalRead(A3) == LOW) && (digitalRead(A0) == LOW)) { setHatSwitch(0, 225); } else // downLeft
  if ((digitalRead(A3) == LOW) && (digitalRead(A1) == LOW)) { setHatSwitch(0, 135);} else // downRight
  if ( digitalRead(A2) == LOW) { setHatSwitch(0, 0); } else // UP
  if ( digitalRead(A3) == LOW) { setHatSwitch(0, 180); } else // DOWN
  if ( digitalRead(A0) == LOW) { setHatSwitch(0, 270); } else // LEFT
  if ( digitalRead(A1) == LOW) { setHatSwitch(0, 90); } else // RIGHT
  if ( digitalRead(A4) == LOW) { Joystick.setButton(4, 1); } else
  { Joystick.setHatSwitch(0, JOYSTICK_HATSWITCH_RELEASE); Joystick.setButton(4, 0); }

  // Toggle Switches
  toggle(0); toggle(1); toggle(2);
  toggle(3); toggle(4); toggle(5);

  // Push buttons
  if(mcp.digitalRead(15) == LOW) { Joystick.setButton(0, 1); Serial.println("BTN 1");} else { Joystick.setButton(0, 0); }
  if(mcp.digitalRead(14) == LOW) { Joystick.setButton(1, 1); Serial.println("BTN 2"); } else { Joystick.setButton(1, 0); }
  if(mcp.digitalRead(12) == LOW) { Joystick.setButton(2, 1); Serial.println("BTN 3"); } else { Joystick.setButton(2, 0); }
  if(mcp.digitalRead(13) == LOW) { Joystick.setButton(3, 1); Serial.println("BTN 4"); } else { Joystick.setButton(3, 0); }

  // Handle Encoder
  value += handleEnc->getValue();
  if (value != last) {
    // Deal with fast clicks
    if(hEncTimer >= 2) { 
      Serial.println("RESET");
      Joystick.setButton(11, 0); Joystick.setButton(12, 0);
      hEncTimer = 0;
    }
    
    if(value > last) { // GOING UP!
      Serial.println("UP");
      Joystick.setButton(11, 1);
      hEncTimer = 1;
    } else if (value < last) { // GOING DOWN!
      Serial.println("DOWN");
      Joystick.setButton(12, 1);
      hEncTimer = 1;
    }
    last = value;
  }
  if(hEncTimer != 0) {
    if(hEncTimer <= hEncTimerLimit) {
      // WAIT
      hEncTimer++;
      Serial.print("WAIT: ");
      Serial.println(hEncTimer);
    }
  
    if(hEncTimer >= hEncTimerLimit) {
      // BUTTONS DOWN
      hEncTimer = 0;
      Serial.println("STOP");
      Joystick.setButton(11, 0); Joystick.setButton(12, 0);
    }
  }

  // Handle Encoder Button
  ClickEncoder::Button b = handleEnc->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    Serial.println(b);
    // TODO & NOTES FOR TOMORROW
    /*
     * Create states for double click & hold.
     * 
     * Problem to fix: press and release needs to be longer
     * but do not want to delay the whole program for this.
     */
    if(b == 5) { Joystick.setButton(13, 1); delay(5); Joystick.setButton(13, 0); }
  }
       
  
//  if (mcp.digitalRead(6) == LOW) {
//    toggleState[0] == 1;
//    if(toggleState[0] == 1) { Serial.println("PIN 6 FIRE"); }
//    if(toggleState[0] <= toggleLimit) {
//      toggleState[0]++;
//    }
//    if(toggleState[0] >= toggleLimit) { 
//      Serial.println("SWITCH 6 OFF"); 
//    }
//  } else if (mcp.digitalRead(6) == HIGH) {
//    toggleState[0] = 0;
//    Serial.println("Toggle OFF and Reset");
//  }

//  if (mcp.digitalRead(7) == LOW) {
//    Serial.println("PIN 7!!!");
//  }

  // Read Absolute Encoder (Axis)
  axisVal = map(myACE.mpos(), 0, -88, 0, 1024);
  Joystick.setXAxis(axisVal);
  //Serial.print("Axis Value: "); Serial.println(myACE.mpos());


//  mcp.digitalWrite(7, HIGH);
//  delay(dly);
//  mcp.digitalWrite(7, LOW);
//  delay(dly);
    //delay(125);
}
