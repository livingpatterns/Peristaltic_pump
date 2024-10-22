/*
  Project: 3D-printed peristaltic pump kit
  Author: https://www.sciencedirect.com/science/article/pii/S2468067221000316 
  Modified by: Romain Defferrard, Living Patterns Laboratory EPFL
  
  Date: 25.03.2024

  Description: Code to drive the peristaltic pump.

  Additional notes: Might need to change lines 104-108 depending your needs. If you need more precise flow rate control, you can change line 108 to increase
  the flow rate by 1rpm instead of 5rpm. 
*/

//include the following
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>

#define OLED_RESET 4
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//change this to fit the number of steps per revolution as per specification given by the motor.
//The micro geared stepper motor: 20steps/rev and gear ratio of 1:300.
//This value is further divided by 100 because the library does not take rpm with 2 decimal places. As such, we divide this value by 100 and subsequently multiple 'motorSpeed' by 100.
const int stepsPerRevolution = 20 * 100 / 100;
int motorSpeed; //Do take note this value will be multiplied by 100 later in the code.


//Declare pin for stepper motor driver
Stepper myStepper(stepsPerRevolution, 12, 11, 10, 9);

//Declare pins for all the pushbuttons
const int forbut = 6; //for forward
const int revbut = 7; //for reverse
const int but1 = A1;
const int but2 = A2;
const int but3 = A3;

// Declare chip enable pin
const int chipenable = 8;

//initialise initial state of push button (red and green)
int revbuttonState = HIGH;
int buttonState = HIGH;
int digit1 = 0;   // ones place
float digit2 = 5; // tenths place
float digit3 = 0; // hundreth place

//This code uses a state machine. Initial starting state is '99'
int state = 99;
float desiredSpeed = 0.0;
float displayedSpeed = 0.0;

void setup() {
  // put your setup code here, to run once:

  // for display
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // Setup all pushbuttons
  // usually use the built in pull up function to help make buttons less jittery
  pinMode(revbut, INPUT_PULLUP);
  pinMode(forbut, INPUT_PULLUP);
  pinMode(but1, INPUT_PULLUP);
  pinMode(but2, INPUT_PULLUP);
  pinMode(but3, INPUT_PULLUP);

  // Chip enable state
  pinMode(chipenable, OUTPUT);
  digitalWrite(chipenable, LOW);
}

void displayspeed() {
  displayedSpeed = float(desiredSpeed);
  //This part is to display the speed reading:
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1); //you can change font size here
  display.setCursor(0, 8); //you can adjust the position of the cursor here
  display.print("Setting");
  display.setTextSize(2); //you can change font size here
  display.setCursor(50, 5);
  display.print("Speed:");
  display.setTextSize(3);
  display.setCursor(0, 30);
  display.print(displayedSpeed);
  display.setTextSize(2);
  display.setCursor(75, 38);
  display.print(" rpm");
  //display.print(" \t");
}

void loop() {
  // put your main code here, to run repeatedly:

  //state 0 is for you to manually adjust the speed.
  if (state == 0) {
    int new1 = digitalRead(but1);
    int new2 = digitalRead(but2);
    int new3 = digitalRead(but3);
    if (new1 == LOW) {
      if (digit1 >= 20) { // Max allowable rpm: 20rpm
        digit1 = digit1 - 20;
      }
      else {
        digit1 = digit1 + 5; // increase by 5rpm
      }
      desiredSpeed = digit1 + (digit2 / 10) + (digit3 / 100);
      displayspeed(); //refresh screen to display edited speed
      display.display();
      delay(100);
    }

    if (new2 == LOW) {
      if (digit2 >= 9) {
        digit2 = digit2 - 9;
      }
      else {
        digit2 = digit2 + 1;
      }
      desiredSpeed = digit1 + (digit2 / 10) + (digit3 / 100);
      displayspeed(); //refresh screen to display edited speed
      display.display();
      delay(100);
    }
    if (new3 == LOW) {
      if (digit3 >= 9) {
        digit3 = digit3 - 9;
      }
      else {
        digit3 = digit3 + 1;
      }
      desiredSpeed = digit1 + (digit2 / 10) + (digit3 / 100);
      displayspeed(); //refresh screen to display edited speed
      display.display();
      delay(100);
    }
    //We multiple the motorspeed here by 100 because we divided the 'stepsPerRevolution' by 100
    motorSpeed = int(desiredSpeed * 100);

    // IF FORWARD button is pressed, go to state 11
    if (digitalRead(forbut) == LOW)      //if button pull down
    {
      state = 11;
      delay(200);
    }

    //IF REV BUTTON PRESSED, GO TO STATE 22
    if (digitalRead(revbut) == LOW) {
      state = 22;
      delay(200);
    }
  }

  //state 11 is simply a passing state meant to toggle the display to tell that you are in a "forward' mode
  if (state == 11) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 3);
    display.print("moving ");
    display.setTextSize(2);
    display.print("Forward");
    display.setCursor(0, 25);
    display.setTextSize(1);
    display.print("Speed: ");
    display.setCursor(0, 40);
    display.setTextSize(2);
    display.print(displayedSpeed);
    display.print(" rpm");
    display.display();
    state = 1;
    delay(500);
  }

  //state 1 will run the motor with the preset speed (as determined in state 0) until the buttons toggled to off.
  if (state == 1) {
    buttonState = LOW;
    digitalWrite(chipenable, HIGH);
    while (buttonState == LOW) { //if button is not toggled (i.e. HIGH), motor will keep spinning
      // step 1/100 of a revolution:
      myStepper.setSpeed(motorSpeed);//set speed of the motor to 'motorSpeed' which translates to 'desiredSpeed'
      myStepper.step(stepsPerRevolution / 10); //This function is blocking; that is, it will wait until the motor has finished moving to pass control to the next line in your sketch. We divide this by 20 to make motor more responsive. See library for more details.
      buttonState = digitalRead(forbut);
    }
    digitalWrite(chipenable, LOW);
    state = 99;
    delay(500);
  }

  //state 22 is simply a passing state meant toggle the display to tell u r in a "reverse' mode
  if (state == 22) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 3);
    display.print("moving ");
    display.setTextSize(2);
    display.print("Reverse");
    display.setCursor(0, 25);
    display.setTextSize(1);
    display.print("Speed: ");
    display.setCursor(0, 40);
    display.setTextSize(2);
    display.print(displayedSpeed);
    display.print(" rpm");
    display.display();
    state = 2;
    delay(500);
  }

  //This does the same thing as state 1 except that it is in reverse
  if (state == 2) {
    revbuttonState = LOW;
    digitalWrite(chipenable, HIGH);
    while (revbuttonState == LOW) { //if button is not pressed (i.e. HIGH), motor will keep spinning
      // step 1/100 of a revolution:
      myStepper.setSpeed(motorSpeed);
      myStepper.step(-stepsPerRevolution / 10);
      revbuttonState = digitalRead(revbut);
    }
    digitalWrite(chipenable, HIGH);
    state = 99;
    delay(500);
  }

  //This is a passing state, just to display "Set speed" to prompt user to set speed.
  if (state == 99) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 32);
    display.print("Set Speed");
    display.display();
    state = 0;
    delay(1000);
    displayspeed();
    display.display();
  }

}
