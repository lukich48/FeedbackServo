#pragma once
#include "arduino.h"

#include <Servo.h>
#include <FileData.h>
#include <LittleFS.h>
// #include <SPIFFS.h>


struct FeedbackInterval{
    int minFeedback;
    int maxFeedback;
    int minAngle;
    int maxAngle;
};

struct Data {
    int anglePoints[10];
    int feedbackPoints[10];
    int maxIndex;
  };
Data points;
FileData fileData;

static const int delta = 0;

class FeedbackServo{
    public:
        byte feedbackPin;
        int minAngle;
        int maxAngle;
        int minAllowedAngle;
        int maxAllowedAngle;
        FeedbackServo(const char* dataPath, Servo* servo, byte feedbackPin, int minAngle = 0, int maxAngle = 180);
        void setAllowedAngles(int minAllowedAngle, int maxAllowedAngle);
        void calibrate();
        void printCalibrationData();
        void write(int angle);
    private:
        Servo *servo;
        FeedbackInterval getFeedbackInterval(int curFeedback);
        int getCurAngle();
};

FeedbackServo::FeedbackServo(const char* dataPath, Servo* servo, byte feedbackPin, int minAngle, int maxAngle)
{
    this->feedbackPin = feedbackPin;
    this->minAngle = minAngle;
    this->maxAngle = maxAngle;
    this->minAllowedAngle = minAngle;
    this->maxAllowedAngle = maxAngle;

    this->servo = servo;
    fileData.setFS(&LittleFS, dataPath);
    fileData.setData(&points, sizeof(points));
    LittleFS.begin();
    fileData.read();
}

void FeedbackServo::setAllowedAngles(int minAllowedAngle, int maxAllowedAngle){
    this->minAllowedAngle = minAllowedAngle;
    this->maxAllowedAngle = maxAllowedAngle;
}

void FeedbackServo::calibrate(){
    
    servo->write(minAngle);
    delay(3000);

    int curAngle = minAngle;
    int angleIncrement = 20;

    for (int i = 0; i < sizeof(points.anglePoints)/sizeof(*points.anglePoints); i++){

        servo->write(curAngle);
        delay(2000);
        int curFeedback = analogRead(feedbackPin);
        log_d("%d %d: %d", i, curAngle, curFeedback);

        points.anglePoints[i] = curAngle;
        points.feedbackPoints[i] = curFeedback;
        points.maxIndex = i;

        curAngle = constrain(curAngle + angleIncrement, minAngle, maxAngle);
    }

    fileData.updateNow();
}

void FeedbackServo::printCalibrationData(){
    
    for(int i = 0; i <= points.maxIndex; i++){
        Serial.printf("%d %d: %d\n", i, points.anglePoints[i], points.feedbackPoints[i]);
    }
}

void FeedbackServo::write(int angle){
    int curAngle = getCurAngle();
    servo->write(angle);

    // todo: ждем расчетное время
    delay(1500);   

    curAngle = getCurAngle();
    int curDelta = constrain(angle - curAngle, -delta, delta);
    log_d("write angle: %d", curAngle + curDelta);
    
    servo->write(curAngle + curDelta);
}

FeedbackInterval FeedbackServo::getFeedbackInterval(int curFeedback){
    FeedbackInterval res;

    if(curFeedback <= points.feedbackPoints[0]){
            res.minFeedback = curFeedback - 1;
            res.maxFeedback = curFeedback + 1;
            res.minAngle = points.anglePoints[0] - 1;
            res.maxAngle = points.anglePoints[0] + 1;
            return res;
    }

    if(curFeedback >= points.feedbackPoints[points.maxIndex]){
        res.minFeedback = curFeedback - 1;
        res.maxFeedback = curFeedback + 1;
        res.minAngle = points.anglePoints[points.maxIndex] - 1;
        res.maxAngle = points.anglePoints[points.maxIndex] + 1;
        return res;
    }

    for (int i = 0; i <= points.maxIndex; i++){
        if(curFeedback < points.feedbackPoints[i]){
            res.minFeedback = points.feedbackPoints[i-1];
            res.maxFeedback = points.feedbackPoints[i];
            res.minAngle = points.anglePoints[i-1];
            res.maxAngle = points.anglePoints[i];
            return res;
        }
    }
}

int FeedbackServo::getCurAngle(){
    int feedback = analogRead(feedbackPin);
    FeedbackInterval interval = getFeedbackInterval(feedback);
    int curAngle = map(feedback, interval.minFeedback, interval.maxFeedback, interval.minAngle, interval.maxAngle);
    log_d("feedback: %d angle: %d", feedback, curAngle);
    return curAngle;
}