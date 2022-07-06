#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <IRremote.h>

#define PHOTO_RESISTOR A0
#define TRIGGER_PIN 4
#define BUTTON_PIN 2 
#define RECEIVE_PIN 5
#define ECHO_PIN 3
#define RED_LED_PIN 11
#define YELLOW_LED_PIN 10
#define GREEN_LED_PIN 9
#define MAXIMUM_BLINK_DELAY 1600
#define MAXIMUM_DISTANCE 400
#define SPEED_OF_SOUND 340.0

#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
#define IR_BUTTON_LEFT 8
#define IR_BUTTON_RIGHT 90
#define IR_BUTTON_OK 28
#define IR_BUTTON_HASH 13
#define IR_BUTTON_STAR 22

unsigned long triggerWaitTime = 60;
unsigned long lockedBlinkDelay = 300;
unsigned long previousTriggerTime = millis();
unsigned long previousYellowLedBlinkTime = millis();
unsigned long pulseInTimeBegin;
unsigned long pulseOutTimeEnd;
unsigned long yellowLEDBlinkDelay = MAXIMUM_BLINK_DELAY;
unsigned long previousLockedTimeBlink;
unsigned long debouncedDelay = 35;
unsigned long previousDebouceTime = millis();
double previousDistance = 0;
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3, POSITIVE);

//state
bool yellowLEDState = false;
bool isRobotLocked = false;
bool lockedBlinkState = false;
bool memoryReset = false;

//screen state
int currentScreenInView = 1;
int photoResistorValue = 0;
int measurement = 0;

bool newDistanceAvailable = false;
double distance = 0;
double delayPercentage = 1;

void echoPinInterruptHandler(){
    if(digitalRead(ECHO_PIN) == HIGH){
        pulseInTimeBegin = micros();
    }else{
        pulseOutTimeEnd = micros();
        newDistanceAvailable = true;
    }
}
void changeMeasurement(){
    if(measurement == 1){
        measurement = 2;
    }else{
        measurement = 1;
    }
    EEPROM.write(34, measurement);
}

void handleButtonEvents(int command){
    Serial.println(command);
    switch(command){
        case IR_BUTTON_UP:
            if(currentScreenInView == 3){
                currentScreenInView = 1;
               
            }else{
                currentScreenInView++;
            }
             memoryReset = false;
            break;
        case IR_BUTTON_DOWN:
            if(currentScreenInView == 1){
                currentScreenInView = 3;
            }else{
                currentScreenInView--;
            }
             memoryReset = false;
            break;
        case IR_BUTTON_LEFT:
            if(currentScreenInView == 1){
                currentScreenInView = 3;
            }else{
                currentScreenInView--;
            }
             memoryReset = false;
            break;
        case IR_BUTTON_RIGHT:
            if(currentScreenInView == 3){
                currentScreenInView = 1;
            }else{
                currentScreenInView++;
            }
             memoryReset = false;
            break;
        case IR_BUTTON_OK:
             unlockRobot();
            break;
        case IR_BUTTON_HASH:
             changeMeasurement();
            break;
        case IR_BUTTON_STAR:
            if(currentScreenInView == 2){
                if(measurement > 1){
                    EEPROM.write(34,1);
                    measurement = 1;
                }
                memoryReset = true;
                
            }
            
    }
}

void calculateDistance(){
    //speed = d * t
    //distance = speed/t
    double duration = pulseOutTimeEnd - pulseInTimeBegin;
    distance = ((SPEED_OF_SOUND/10000.0) * duration)/ 2.0;
    // if(previousDistance != 0){
    //     distance = previousDistance * 0.7 + newDistance * 0.3;
    // }else{
    //     distance = newDistance;
    // }
    previousDistance = distance;
    if(distance >= MAXIMUM_DISTANCE){
        delayPercentage = 1.0;
    }else{
        delayPercentage = distance / MAXIMUM_DISTANCE;
    }
    yellowLEDBlinkDelay = (MAXIMUM_BLINK_DELAY * delayPercentage);
    if(distance < 10.0){
       lockRobot();
    }

}

void triggerUltrasensor(){
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
}

void checkPushButton(){
    if(digitalRead(BUTTON_PIN) == HIGH){
        unlockRobot();
    }
}

void luminosityHandler(){
    photoResistorValue = analogRead(PHOTO_RESISTOR);
    double percent = photoResistorValue/1023.0;
    double flipPercent = 1 - percent;
    int greeLedBrightness = flipPercent * 255;
    analogWrite(GREEN_LED_PIN,greeLedBrightness);
}

void lockRobot(){
    isRobotLocked = true;
    previousLockedTimeBlink = millis();
}

void unlockRobot(){
    isRobotLocked = false;
    digitalWrite(RED_LED_PIN, LOW);
}

void obstacleDetailsDisplay(){
    lcd.setCursor(0,0);
    String response = "Dist: ";
    response += String(measurement == 1 ? distance : (distance/2.54));
    response += measurement == 1 ? " cm" : " in";
    addedSpace(response);
    lcd.print(response);
    lcd.setCursor(0,1);
    if(distance < 15.0){
        response = "!! Warning !!";
    }else{
        response ="No obstacle.";
    }
    addedSpace(response);
    lcd.print(response);
}

void settingsDetailsDisplay(){
    lcd.setCursor(0,0);
    String response = "Press on OFF to";
    addedSpace(response);
    lcd.print(response);
    lcd.setCursor(0,1);
    response = "reset settings";
    addedSpace(response);
    lcd.print(response);
}

void resetMeasurementDisplay(){
    lcd.setCursor(0,0);
    String response = "Settings have";
    addedSpace(response);
    lcd.print(response);
    lcd.setCursor(0,1);
    response = "been reset.";
    addedSpace(response);
    lcd.print(response);
}

void luminosityDetailsDisplay(){
    lcd.setCursor(0,0);
    String response = "Luminosity: ";
    response += photoResistorValue;
    addedSpace(response);
    lcd.print(response);
    lcd.setCursor(0,1);
    response ="";
    addedSpace(response);
    lcd.print(response);
}

void handleRemoteControlInput(){
    if(IrReceiver.decode()){
        IrReceiver.resume();
        int command = IrReceiver.decodedIRData.command;
        handleButtonEvents(command);
    }
}

void printToLCD(){
    String msg = "";
        if(isRobotLocked){
            lcd.setCursor(0,0);
            msg = "!!! Obstacle !!!";
            addedSpace(msg);
             lcd.print(msg);
            lcd.setCursor(0,1);
            msg = "Press to unlock.";
            lcd.print(msg);
        }else{
           switch (currentScreenInView)
           {
            case 1:
                obstacleDetailsDisplay();
                break;
            case 2:
                if(memoryReset){
                    resetMeasurementDisplay();
                }else{
                    settingsDetailsDisplay();
                }
                break;
            case 3:
                luminosityDetailsDisplay();
                break;
           }
        }
       
}

void addedSpace(String& str){
    for(int i = str.length(); i < 16; i++){
        str += " ";
    }
}


void setup(){
    lcd.begin(16,2);
    lcd.clear();
    measurement = EEPROM.read(34);
    if(measurement != 1 && measurement != 2){
        measurement = 1;
    }
    Serial.begin(115200);
    IrReceiver.begin(RECEIVE_PIN);
    attachInterrupt(digitalPinToInterrupt(ECHO_PIN),echoPinInterruptHandler, CHANGE);
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
}

void loop(){
     unsigned long timeNow = millis();
     luminosityHandler();
     if(timeNow - previousDebouceTime > debouncedDelay){
        previousDebouceTime += debouncedDelay;
        handleRemoteControlInput();
     }
    if(!isRobotLocked){
        if(timeNow - previousTriggerTime > triggerWaitTime){
            previousTriggerTime += triggerWaitTime;
            triggerUltrasensor();
        }
        if(newDistanceAvailable){
            newDistanceAvailable = false;
            calculateDistance();
        }
        if(timeNow - previousYellowLedBlinkTime > yellowLEDBlinkDelay){
            previousYellowLedBlinkTime += yellowLEDBlinkDelay;
            if(yellowLEDState == false){
                digitalWrite(YELLOW_LED_PIN, HIGH);
            }else{
                digitalWrite(YELLOW_LED_PIN, LOW);
            }
            yellowLEDState = !yellowLEDState;
        
        } 
    }else{
        checkPushButton();
        if(timeNow - previousLockedTimeBlink > lockedBlinkDelay){
             previousLockedTimeBlink += lockedBlinkDelay;
            if(lockedBlinkDelay == false){
                digitalWrite(YELLOW_LED_PIN, HIGH);
                digitalWrite(RED_LED_PIN, HIGH);
            }else{
                digitalWrite(YELLOW_LED_PIN, LOW);
                digitalWrite(RED_LED_PIN, LOW);
            }
            lockedBlinkDelay = !lockedBlinkDelay;
        }
    }
    printToLCD();
}