//Library for 7 segment display
#include <SevSeg.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// Initialize software serial on pins 10 and 11 for DF player
SoftwareSerial mySoftwareSerial(10, 11);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;
String line;
char command;
int pause = 0;
int repeat = 0;

// Define the trigger (aka limit switch) and music button
#define trigger A1
#define musicButton A3
#define dialogueButton A5
#define killSwitch 4
int musicButtonState = 0;     // variable for reading the pushbutton status
int dialogueButtonState = 0;  // variable for reading the pushbutton status
int totalSongs = 3;
int currentSong = 0;

// Define the magazine size and shot burst
int shotCount = 36;
int shotBurst = 3;

// 7 segment display
//Create an instance of the object.
SevSeg myDisplay;

//Create global variables
unsigned long timer;
//int deciSecond = 0;

//Timing
unsigned long previousMillis = 0;
const long interval = 100;  // 100ms interval between decrementing shots
bool firing = false;        // Indicates whether the system is currently firing
int shotsToFire = 0;        // Keeps track of how many shots need to be decremented

//Reloading variables
bool reloadingState = true;  // Keeps track of whether we are in the reloading phase
unsigned long blinkPreviousMillis = 0;
const long blinkInterval = 500;  // Blinking interval in milliseconds (500ms on, 500ms off)
bool displayOn = true;           // Keeps track of whether the display is on or off

//Firing cooloff
unsigned long lastTriggerTime = 0;  // To store the time when the trigger was last pressed
const long coolOffInterval = 500;   // 250ms cool-off period

void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  // Check if the module is responding and if the SD card is found
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini"));
  Serial.println(F("Initializing DFPlayer module ... Wait!"));

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Not initialized:"));
    Serial.println(F("1. Check the DFPlayer Mini connections"));
    Serial.println(F("2. Insert an SD card"));
    while (true)
      ;
  }
  Serial.println();
  Serial.println(F("DFPlayer Mini module initialized!"));
  // Initial settings
  myDFPlayer.setTimeOut(500);  // Serial timeout 500ms
  myDFPlayer.volume(30);       // Volume 5
                               //  myDFPlayer.EQ(0);            // Normal equalization
                               //  myDFPlayer.EQ(DFPLAYER_EQ_POP);
                               //  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
                               //  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
                               //  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
  myDFPlayer.EQ(DFPLAYER_EQ_BASS);

  //Set trigger and music button pinmode
  pinMode(trigger, INPUT_PULLUP);
  pinMode(musicButton, INPUT);

  // 7 segment display
  int displayType = COMMON_CATHODE;  //Your display is either common cathode or common anode

  //This pinout is for a regular display
  //Declare what pins are connected to the digits
  int digit1 = 2;  //Pin 9 on my 2 digit display
  int digit2 = 3;  //Pin 6 on my 2 digit display
  int digit3;      //Pin 8 on my 2 digit display
  int digit4;      //Pin 6 on my 2 digit display

  //Declare what pins are connected to the segments
  int segA = 4;    //Pin 7 on my 2 digit display
  int segB = 5;    //Pin 8 on my 2 digit display
  int segC = 6;    //Pin 3 on my 2 digit display
  int segD = 7;    //Pin 2 on my 2 digit display
  int segE = 8;    //Pin 1 on my 2 digit display
  int segF = 9;    //Pin 10 on my 2 digit display
  int segG = 12;   //Pin 4 on my 2 digit display
  int segDP = 13;  //Pin 5 on my 2 digit display

  int numberOfDigits = 2;  //Do you have a 1, 2 or 4 digit display?

  myDisplay.Begin(displayType, numberOfDigits, digit1, digit2, digit3, digit4, segA, segB, segC, segD, segE, segF, segG, segDP);

  myDisplay.SetBrightness(100);  //Set the display to 100% brightness level

  timer = millis();

  // Play empty sound to make DF Play loading instant
  myDFPlayer.playFolder(2, 1);
}

void loop() {
  // Continuously display the current shot count on the 7-segment display
  displayNumber();

  unsigned long currentMillis = millis();  // Get the current time

  // Check if the trigger is pressed (LOW), we're not already firing, and the cool-off period has passed
  if (digitalRead(trigger) == LOW && !firing && (currentMillis - lastTriggerTime >= coolOffInterval)) {
    myDFPlayer.play(1);
    // Begin firing process
    shotsToFire = shotBurst;          // Set how many shots to decrement (3 shots)
    firing = true;                    // Indicate that the system is now firing
    previousMillis = millis();        // Reset the timer to the current time
    lastTriggerTime = currentMillis;  // Update the last trigger time to the current time
  }

  // If we're firing, handle the countdown
  if (firing) {
    shotsFired();  // Call the shotsFired function to handle shot countdown
  }


  musicButtonState = digitalRead(musicButton);
  if (musicButtonState == HIGH) {
    currentSong++;
    delay(800);
    if (currentSong == 1) {
      myDFPlayer.playFolder(1, 1);
      Serial.println("Song 1 playing");
      return;
    }
    if (currentSong == 2) {
      myDFPlayer.playFolder(1, 2);
      Serial.println("Song 2 playing");
      return;
    }
    if (currentSong == 3) {
      myDFPlayer.playFolder(1, 3);
      Serial.println("Song 3 playing");
      currentSong = 1;
      return;
    }
  }

  dialogueButtonState = digitalRead(dialogueButton);
  if (dialogueButtonState == HIGH) {
    Serial.println("Play Dialogue");
    delay(800);
    return;
    // if (currentSong == 1) {
    //   myDFPlayer.playFolder(1, 1);
    //   Serial.println("Song 1 playing");
    //   return;
    // }
    // if (currentSong == 2) {
    //   myDFPlayer.playFolder(1, 2);
    //   Serial.println("Song 2 playing");
    //   return;
    // }
    // if (currentSong == 3) {
    //   myDFPlayer.playFolder(1, 3);
    //   Serial.println("Song 3 playing");
    //   currentSong = 1;
    //   return;
    // }
  }
}

void shotsFired() {
  unsigned long currentMillis = millis();

  // Check if enough time has passed (100ms) since the last shot decrement
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Update previousMillis to the current time

    // Decrement the shot count and update display
    if (shotsToFire > 0 && shotCount > 0) {
      shotCount--;    // Decrease shot count by 1
      shotsToFire--;  // Decrease the number of shots left to fire in the burst
      Serial.println(shotCount);
      Serial.println("CLACK");
    }

    // If all shots in the burst have been fired, stop the firing process
    if (shotsToFire == 0 || shotCount == 0) {
      firing = false;
      if (shotCount == 0) {
        reloading();  // Call reloading function if the magazine is empty
      }
    }
  }
}

void reloading() {
  // Continuously display "0" on the 7-segment display while reloading
  if (reloadingState) {
    char tempString[10];
    sprintf(tempString, "%1d", 0);  // Display "0"
    myDisplay.DisplayString(tempString, 0);
    Serial.println("Display: 0");
  }

  // Wait for the trigger to be pulled to stop reloading and reset the shot count
  if (digitalRead(trigger) == LOW) {
    reloadingState = false;  // Exit the reloading phase
    shotCount = 36;          // Reset the shot count to 36
    Serial.println("Reloading complete, shotCount reset to 36");

    // Display the new shot count (36) but do not decrement yet
    displayNumber();
  }
}

void displayNumber() {
  char tempString[10];                     //Used for sprintf
  sprintf(tempString, "%2d", shotCount);   //Convert deciSecond into a string that is right adjusted
  myDisplay.DisplayString(tempString, 0);  //(numberToDisplay, decimal point location)
}

// void playMusic() {
//   myDFPlayer.playFolder(1, 1);  //play specific mp3 in SD:/folder/song.mp3; Folder Name(1~99); File Name(1~255)
//   if (currentSong = 1) {

//   } else if (currentSong = 2) {
//     myDFPlayer.next();
//   } else {
//     myDFPlayer.next();
//   }


//   // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
// }
