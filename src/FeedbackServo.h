#pragma once
#include "arduino.h"

#include <Servo.h>

struct FeedbackInterval{
    int minFeedback;
    int maxFeedback;
    int minAngle;
    int maxAngle;
};

class FeedbackServo{
    private:
        Servo *servo;
        void writeRange(int start, int end);
        int anglePoints[10]; 
        int feedbackPoints[10];
        int pointsCount;
        void sortPoints();
        FeedbackInterval getFeedbackInterval(int curFeedback);
        int getCurAngle();
    public:
        byte servoPin; 
        byte feedbackPin;
        int minFeedback;
        int maxFeedback; 
        int minAngle;
        int maxAngle;
        int minAllowedAngle;
        int maxAllowedAngle;
        int stopable = true;
        FeedbackServo(Servo* servo, byte feedbackPin, int minFeedback, int maxFeedback, int minAngle = 0, int maxAngle = 180);
        void addPoint(int angle, int feedback);
        void setAllowedAngles(int minAllowedAngle, int maxAllowedAngle);
        void goToAngle(int angle);
        void write(int angle);
};

static const int delta = 3;

FeedbackServo::FeedbackServo(Servo* servo, byte feedbackPin, int minFeedback, int maxFeedback, int minAngle, int maxAngle)
{
    this->servoPin = servoPin;
    this->feedbackPin = feedbackPin;
    this->minFeedback = minFeedback;
    this->maxFeedback = maxFeedback;
    this->minAngle = minAngle;
    this->maxAngle = maxAngle;
    this->minAllowedAngle = minAngle;
    this->maxAllowedAngle = maxAngle;

    this->servo = servo;

    anglePoints[pointsCount] = minAngle;
    feedbackPoints[pointsCount] = minFeedback;
    pointsCount++;

    anglePoints[pointsCount] = maxAngle;
    feedbackPoints[pointsCount] = maxFeedback;
    pointsCount++;
}

void FeedbackServo::addPoint(int angle, int feedback){
    anglePoints[pointsCount] = angle;
    feedbackPoints[pointsCount] = feedback;
    pointsCount++;

    sortPoints();
}

void FeedbackServo::setAllowedAngles(int minAllowedAngle, int maxAllowedAngle){
    this->minAllowedAngle = minAllowedAngle;
    this->maxAllowedAngle = maxAllowedAngle;
}

void FeedbackServo::write(int angle){
    int curAngle = getCurAngle();
    servo->write(angle);

    // todo: ждем рассчетное время
    delay(1500);   

    curAngle = getCurAngle();
    int curDelta = constrain(angle - curAngle, -delta, delta);
    log_d("write angle: %d", curAngle + curDelta);
    
    servo->write(curAngle + curDelta);
}

void FeedbackServo::goToAngle(int angle){
    int curAngle = getCurAngle();
    log_d("curAngle: %d", curAngle);

    writeRange(curAngle, angle);
}

void FeedbackServo::writeRange(int start, int end)
{
    servo->write(start);
    delay(300);
    int feedback = analogRead(feedbackPin);
    log_d("%d: %d", start, feedback);

    if (start < end){
        for (int i = start+2; i<=end; i += 2){
            servo->write(i);
            delay(100);
            int curFeedback = analogRead(feedbackPin);
            log_d("%d: %d", i, curFeedback);
            if (stopable && curFeedback < feedback){
                i+= 2;
                servo->write(i);
                delay (300);
                curFeedback = analogRead(feedbackPin);
                log_d("%d: %d", i, curFeedback);
                if(curFeedback < feedback){
                    servo->write(i-3);
                    break;
                }
            }
            feedback = curFeedback;
        }
    }
    else {
        for (int i = start-2; i>=end; i -=2){
            servo->write(i);
            delay(100);
            int curFeedback = analogRead(feedbackPin);
            log_d("%d: %d", i, curFeedback);
            if (stopable && curFeedback > feedback){
                i-= 2;
                servo->write(i);
                delay (300);
                curFeedback = analogRead(feedbackPin);
                log_d("%d: %d", i, curFeedback);
                if(curFeedback > feedback){
                    servo->write(i+3);
                    break;
                }
            }
            feedback = curFeedback;
        }
    }
}

void FeedbackServo::sortPoints()
{
	int tempAngle = 0;
	int tempFeedback = 0;
	for (int i = 0; i < pointsCount; i++)
	{
		for (int j = i; j < pointsCount; j++)
		{
			if (anglePoints[i] > anglePoints[j])
			{
				tempAngle = anglePoints[i];
				anglePoints[i] = anglePoints[j];
				anglePoints[j] = tempAngle;

                tempFeedback = feedbackPoints[i];
                feedbackPoints[i] = feedbackPoints[j];
				feedbackPoints[j] = tempFeedback;
			}
		}
	}
}

FeedbackInterval FeedbackServo::getFeedbackInterval(int curFeedback){
    FeedbackInterval res;

    if(curFeedback <= feedbackPoints[0]){
            res.minFeedback = curFeedback - 1;
            res.maxFeedback = curFeedback + 1;
            res.minAngle = anglePoints[0] - 1;
            res.maxAngle = anglePoints[0] + 1;
            return res;
    }

    if(curFeedback >= feedbackPoints[pointsCount-1]){
        res.minFeedback = curFeedback - 1;
        res.maxFeedback = curFeedback + 1;
        res.minAngle = anglePoints[pointsCount-1] - 1;
        res.maxAngle = anglePoints[pointsCount-1] + 1;
        return res;
    }

    for (int i = 0; i < pointsCount; i++){
        if(curFeedback < feedbackPoints[i]){
            res.minFeedback = feedbackPoints[i-1];
            res.maxFeedback = feedbackPoints[i];
            res.minAngle = anglePoints[i-1];
            res.maxAngle = anglePoints[i];
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