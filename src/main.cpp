/*
a90 - на 90 градусов
f120 - на 120 градусов с условием на фидбек
c - калибровка серво. запись значений в память
p - вывод таблицы калибровки
s - вывод текущего угла
*/

#include <Arduino.h>
#include <ServoSmooth.h>
#include <ESP32Servo.h>
#include <FeedbackServo.h>

byte servoPin = 27;
byte feedbackPin = 32;

ServoSmooth servo;
FeedbackServo feedbackServo(&servo, "/servo_1.dat", feedbackPin, 110, 180);

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

    feedbackServo.setTargetAngle(angle);
}

void calibrateServo(){
    feedbackServo.calibrate();
}

void printCalibrationData(){
    feedbackServo.printCalibrationData();
}

void getCurrentAngle(){
    int curFeedback = analogRead(feedbackPin);
    int angle = feedbackServo.getCurrentAngle();

    Serial.printf("feedback: %d angle: %d\n", curFeedback, angle);
}

void setup(){
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    Serial.printf("curAngle: %d\n",feedbackServo.getCurrentAngle());

    servo.attach(27);
    servo.setSpeed(130);
    servo.setAccel(0.3);
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
        else if (cmd == 's'){
            getCurrentAngle();
        }
    }

    static uint32_t tmr;
    if (millis() - tmr >= 20){
        tmr = millis();
        feedbackServo.tickManual();
    }
}