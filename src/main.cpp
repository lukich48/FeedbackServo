#include <Arduino.h>
#include <Servo.h>
#include <FeedbackServo.h>
#include "esp_adc_cal.h"

byte servoPin = 27;
byte feedbackPin = 32;

Servo servo;
FeedbackServo feedbackServo(&servo, feedbackPin, 230, 3640, 0, 180);

void writeRange(int start, int end, bool stopable)
{
    servo.write(start);
    delay(2000);
    int feedback = analogRead(feedbackPin);
    Serial.printf("%d: %d\n", start, feedback);

    if (start < end){
        for (int i = start+1; i<=end; i += 1){
            servo.write(i);
            delay(200);
            int curFeedback = analogRead(feedbackPin);
            Serial.printf("%d: %d\n", i, curFeedback);
            if (stopable && curFeedback < feedback){
                i+= 2;
                servo.write(i);
                delay (300);
                curFeedback = analogRead(feedbackPin);
                Serial.printf("%d: %d\n", i, curFeedback);
                if(curFeedback < feedback){
                    servo.write(i-3);
                    break;
                }
            }
            feedback = curFeedback;
        }
    }
    else {
        for (int i = start-4; i>=end; i -=4){
            servo.write(i);
            delay(100);
            int curFeedback = analogRead(feedbackPin);
            Serial.printf("%d: %d\n", i, curFeedback);
            if (stopable && curFeedback > feedback){
                delay (200);
                if(curFeedback > feedback){
                    servo.write(i+1);
                    break;
                }
            }
            feedback = curFeedback;
        }
    }
}

void angleFromSerial(){
    
    int angle = Serial.parseInt();
    Serial.printf("\nReceived: %d\n", angle);
    
    int curFeedback = analogRead(feedbackPin);
    Serial.printf("feedback: %d\n", curFeedback);

    servo.write(angle);
}

void rangeFromSerial(){
    
    char buf[32];
    int amount = Serial.readBytesUntil('\n', buf, sizeof(buf));
    Serial.printf("\nReceived: %s\n", buf);

    int start, end, stopable;
    sscanf(buf, "%d-%d %d", &start, &end, &stopable);
    Serial.printf("start: %d end: %d stopable: %d\n", start, end, stopable);

    if (stopable > 1)
        stopable = 0;

    writeRange(start, end, stopable);
}

void writeWithFeedbackFromSerial()
{
    int angle = Serial.parseInt();
    Serial.printf("\nReceived: %d\n", angle);
    
    feedbackServo.write(angle);
}

void setup(){
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    servo.attach(servoPin);
    // servoR.attach(_servoRPin, Servo::CHANNEL_NOT_ATTACHED, 0, 180, 500, 2360);
    feedbackServo.addPoint(30, 750);
    feedbackServo.addPoint(90, 1830);
    feedbackServo.addPoint(163, 3250);
}

void loop(){
    if (Serial.available() > 0) {

        char cmd = Serial.read();


        if (cmd == 'a'){
            angleFromSerial();       
        }
        else if (cmd =='r'){
            rangeFromSerial();
        }
        else if (cmd == 'f'){
            writeWithFeedbackFromSerial();
        }
        
        // if (cmd == 1){
        //     writeFromStartToEnd(0, 180);
        // }
        // else if (cmd == 2){
        //     writeFromEndToStart(0, 180);
        // }
    } 
}