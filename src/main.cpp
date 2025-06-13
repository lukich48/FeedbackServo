/*
a90 - на 90 градусов
f120 - на 120 градусов с условием на фидбек
c - калибровка серво. запись значений в память
p - вывод таблицы калибровки
*/

#include <Arduino.h>
#include <ServoSmooth.h>
#include <FeedbackServo.h>
#include "esp_adc_cal.h"

byte servoPin = 27;
byte feedbackPin = 32;

FeedbackServo servo("/servo_1.dat", feedbackPin, 0, 180);

void angleFromSerial(){
    
    int angle = Serial.parseInt();
    Serial.printf("\nReceived: %d\n", angle);

    servo.write(angle);

    int curFeedback = analogRead(feedbackPin);
    Serial.printf("feedback: %d\n", curFeedback);
}

void writeWithFeedbackFromSerial(){
    int angle = Serial.parseInt();
    Serial.printf("\nReceived: %d\n", angle);
    
    int curFeedback = analogRead(feedbackPin);
    Serial.printf("feedback: %d\n", curFeedback);

    servo.writeWithFeedback(angle);
}

void calibrateServo(){
    servo.calibrate();
}

void printCalibrationData(){
    servo.printCalibrationData();
}

void setup(){
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    servo.attach(servoPin);
    servo.setSpeed(130);
    servo.setAccel(0.3);
    // servo.setTargetDeg(100);
}

void loop(){
    if (Serial.available() > 0) {

        char cmd = Serial.read();

        if (cmd == 'a'){
            angleFromSerial();       
        }
        else if (cmd == 'f'){
            writeWithFeedbackFromSerial();
        }
        else if (cmd == 'c'){
            calibrateServo();
        }
        else if (cmd == 'p'){
            printCalibrationData();
        }
    }

    servo.tick();
}