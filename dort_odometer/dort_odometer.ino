#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>



#include <Wire.h>
#include <EEPROM.h>

Adafruit_8x8matrix eightMatrix = Adafruit_8x8matrix();
Adafruit_7segment odo1Display = Adafruit_7segment();
Adafruit_7segment odo2Display = Adafruit_7segment();

//Calibration Factor
//Number of pulses per 1/10 of a mile
int calibrationFactor = (4250/10);
int calibrationFactorPrev;

//Pin constants for Arduino
const byte buttonOdo1 = 8;
const byte buttonOdo2 = 7;
const byte buttonCalibration = 4;
const byte buttonBrightness = 12;

int brightness = 15;

unsigned long buttonsOffTime; //Value for debounce

boolean buttonOdo1State;
boolean buttonOdo2State;
boolean buttonCalibrationState;
boolean buttonBrightnessState;
boolean buttonPressed = false;
boolean buttonPrevPressed = false;

int sensorCount = 0; //Number of interruptions!
int TimerCount = 0; //Timer2 overflow
int Freq; //Copy of sensorCount for nonblocking purposes
unsigned long totalCount = 0;  //Total sensorCounts
boolean trigger = false;
int odo1 = 0;
int odo2 = 0;
int odo2Distance = 0;

/*ISR(TIMER2_OVF_vect) {                    // This is the Interrupt Service Routine, called when Timer2 overflows

  TimerCount +=8;                         // Increment TimerCount by 8 because 250 kHz is 1/8 of 10 MHz
  if (TimerCount > calibrationFactor)  {  // Check to see if it's time to display the count
    Freq = sensorCount;                   // Copies the interrupt 0 count into variable "Freq"
    trigger = true;                       // sets "Trigger" to let the main loop know it's time to print data
    sensorCount = 0;                      // Reset the interrupt 0 count
    TimerCount -= calibrationFactor;               // Reset Timer2 count, but keep the excess for next time around
  }

}*/

static uint8_t PROGMEM
  smile_bmp[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10100101,
    B10011001,
    B01000010,
    B00111100 },
  neutral_bmp[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10111101,
    B10000001,
    B01000010,
    B00111100 },
  frown_bmp[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10011001,
    B10100101,
    B01000010,
    B00111100 };

void setup() {
  Serial.begin(9600);
  Serial.println("Lichtwald -- Rally Odo Mk. 1");
  
  odo1Display.begin(0x74);
  odo1Display.setBrightness(brightness);
  
  odo2Display.begin(0x76);
  odo2Display.setBrightness(brightness);
  
  eightMatrix.begin(0x70);
  eightMatrix.setBrightness(brightness);
  eightMatrix.setRotation(3);
  
  pinMode(buttonOdo1, INPUT);
  pinMode(buttonOdo2, INPUT);
  pinMode(buttonBrightness, INPUT);
  pinMode(buttonCalibration, INPUT);
  digitalWrite(buttonOdo1, HIGH);
  digitalWrite(buttonOdo2, HIGH);
  digitalWrite(buttonBrightness, HIGH);
  digitalWrite(buttonCalibration, HIGH);
  
  attachInterrupt(0, AddSensorCount, RISING);  // Interrupt 0 is on digital pin 2
  /*
  //  -------------------------------------------------------------------------------------
  //  This bit of code is adapted from an article by dfowler at uchobby.com
  //  http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/
  
  //Timer2 Settings: Timer Prescaler / 64, mode 0
  //Timmer clock = 16 MHz / 64 = 250 KHz or 0.5us
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 0<<CS21 | 0<<CS20;      // Set Timer2 frequency to 250 KHz
                                             // Used to be 010 for 2 MHz clock

  //Timer2 Overflow Interrupt Enable   
  TIMSK2 = 1<<TOIE2;
  
  //  -------------------------------------------------------------------------------------
  */
  buttonsOffTime = millis();                 // Set ButtonStateTime to current time
  
  odo1Display.print(0xDEAD, HEX);
  odo1Display.writeDisplay();
  
  odo2Display.print(0xBEEF, HEX);
  odo2Display.writeDisplay();
  
  /*eightMatrix.clear();
  eightMatrix.setTextSize(1);
  eightMatrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  eightMatrix.setTextColor(LED_ON);
  for (int8_t x=0; x>=-72; x--) {
    eightMatrix.clear();
    eightMatrix.setCursor(x,0);
    eightMatrix.print("Starting...");
    eightMatrix.writeDisplay();
    delay(100);
  }*/
  eightMatrix.clear();
  eightMatrix.drawBitmap(0, 0, neutral_bmp, 8, 8, LED_ON);
  eightMatrix.writeDisplay();
  
  
  //delay(2000);
}

void loop() {
  
  if (sensorCount>0) {
    //It's update time!
    Freq = sensorCount;
    sensorCount = 0;
    totalCount += Freq;
    odo1 += Freq;
    odo2 += Freq;
    while (odo2 > (calibrationFactor/10)){
      //eightMatrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
      odo2 -= (calibrationFactor/10);
      odo2Distance += 1;
    }
      
    
    int odo2Temp = odo2;
    
    eightMatrix.clear();
    eightMatrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
    eightMatrix.writeDisplay();
    
    odo1Display.print(odo1, DEC);
    odo1Display.writeDisplay();
    //odo2Temp = (odo2*100) / (calibrationFactor);
    
    odo2Display.writeDigitNum(0, odo2Temp/1000, false);
    odo2Display.writeDigitNum(1, (odo2Temp/100) % 10, true);
    odo2Display.writeDigitNum(3, (odo2Temp/10) % 10, false);
    odo2Display.writeDigitNum(4, (odo2Temp % 10), false);
    odo2Display.print(odo2Distance, DEC); 
    
    odo2Display.writeDisplay();  
    trigger=false;
  }
  else{
    eightMatrix.clear();
    eightMatrix.drawBitmap(0, 0, neutral_bmp, 8, 8, LED_ON);
    eightMatrix.writeDisplay();
    
    //sensorCount += 123;
  }
  
  buttonBrightnessState = digitalRead(buttonBrightness);
  buttonOdo1State = digitalRead(buttonOdo1);
  buttonOdo2State = digitalRead(buttonOdo2);
  buttonCalibrationState = digitalRead(buttonCalibration);
  
  buttonPressed = !(buttonBrightnessState && buttonOdo1State && buttonOdo2State && buttonCalibrationState);
  
  if (!buttonPressed && buttonPrevPressed)
  {
    buttonsOffTime = millis();
  }
  
   // test to see if it's been at over 50 mS since last button was released
  // and that a button is being pressed for the first time around
  if (((millis() - buttonsOffTime) > 50) && (buttonPressed == true) && (buttonPrevPressed == false))  {
    if (buttonBrightnessState == LOW){
      if (brightness == 15){
        brightness = 1;
      }
      else if (brightness==1){
        brightness = 7;
      }
      else{
        brightness = 15;
      }
      odo1Display.setBrightness(brightness);
      odo2Display.setBrightness(brightness);
      eightMatrix.setBrightness(brightness);
    }
    if (buttonOdo1State == LOW){
      odo1 = 0;
      odo1Display.print(0, DEC);
      odo1Display.writeDisplay();
    }
    if (buttonOdo2State == LOW){
      odo2 = 0;
      odo2Display.print(0, DEC);
      odo2Display.writeDisplay();
    }
  }
  
  delay(250);
  
}

void AddSensorCount()  {                  // This is the subroutine that is called when interrupt 0 goes high
  sensorCount++;                          // Increment SensorCount by 1
}

void calibrationRouting() {
  
  //Enter this loop for the calibration routine
  
  //Check states at the beginning
  buttonBrightnessState = digitalRead(buttonBrightness);
  buttonOdo1State = digitalRead(buttonOdo1);
  buttonOdo2State = digitalRead(buttonOdo2);
  buttonCalibrationState = digitalRead(buttonCalibration);
  
  buttonPressed = !(buttonBrightnessState && buttonOdo1State && buttonOdo2State && buttonCalibrationState);
  
  
  if (!buttonPressed && buttonPrevPressed)
  {
    buttonsOffTime = millis();
  }
  
   // test to see if it's been at over 50 mS since last button was released
  // and that a button is being pressed for the first time around
  if (((millis() - buttonsOffTime) > 50) && (buttonPressed == true) && (buttonPrevPressed == false))  {
    if (buttonBrightnessState == LOW){
      
    }
    if (buttonOdo1State == LOW){
      
    }
    if (buttonOdo2State == LOW){
      
    }
    
    if (buttonCalibrationState == LOW){
      
    }
  }
  
}
