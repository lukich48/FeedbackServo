#pragma once
#include <Arduino.h>

#include <ServoSmooth.h>
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
  };
Data points;
FileData fileData;

class FeedbackServo {
    public:
        byte feedbackPin;
        FeedbackServo(Smooth *servo,  const char* dataPath, byte feedbackPin, int minAngle = 0, int maxAngle = 180);
        void calibrate();                   // запись показаний с feedbackPin в память
        void printCalibrationData();        // вывести в монитор таблицу соответствия углов
        int getCurrentAngle();              // получить текущий угол
        void setTargetAngle(int angle);     // установка целевой позиции в градусах
        boolean tickManual();               // метод, управляющий сервой, без встроенного таймера. Возвращает true, когда целевая позиция достигнута
    protected:
        Smooth *servo;
        int _minAngle;
        int _maxAngle;
        bool _inProcess = false;
        int _finalizeDelay = 500; // ждать после расчетного времени
        int _accurancyDeg = 1; // подгон под результат
        FeedbackInterval getFeedbackInterval(int curFeedback);
        int getFilteredFeedback(); // блокирующая функция
};

FeedbackServo::FeedbackServo(Smooth *servo, const char *dataPath, byte feedbackPin, int minAngle, int maxAngle)
{
    this->servo = servo;
    this->feedbackPin = feedbackPin;
    this->_minAngle = minAngle;
    this->_maxAngle = maxAngle;

    fileData.setFS(&LittleFS, dataPath);
    fileData.setData(&points, sizeof(points));
    LittleFS.begin();
    fileData.read();

    servo->setAutoDetach(false);
    int curAngle = getCurrentAngle();
    servo->setCurrentDeg(curAngle);
    servo->setTargetDeg(curAngle);

    log_d("setCurrentDeg(%d)\n", curAngle);
}

void sort(int a[], int bufCount)
{
	int temp = 0;
	for (int i = 0; i < bufCount; i++)
	{
		for (int j = i; j < bufCount; j++)
		{
			if (a[i] > a[j])
			{
				temp = a[i];
				a[i] = a[j];
				a[j] = temp;
			}
		}
	}
}

int FeedbackServo::getFilteredFeedback(){
    int buf[5];

    for (int i = 0; i < 5; i++){
        delay(50);
        buf[i] = analogRead(feedbackPin);
    }
    
    sort(buf, 5);

    return buf[2];
}

void FeedbackServo::calibrate(){
    
    servo->write(_minAngle);
    delay(3000);

    int curAngle = _minAngle;
    int maxIndex = sizeof(points.anglePoints)/sizeof(*points.anglePoints) - 1;
    int angleIncrement = ceil((_maxAngle - _minAngle) / (double)maxIndex);

    for (int i = 0; i <= maxIndex; i++){

        servo->write(curAngle);
        delay(2000);

        int curFeedback = getFilteredFeedback();
        log_d("%d %d: %d", i, curAngle, curFeedback);

        points.anglePoints[i] = curAngle;
        points.feedbackPoints[i] = curFeedback;

        curAngle = constrain(curAngle + angleIncrement, _minAngle, _maxAngle);
    }

    fileData.updateNow();
}

void FeedbackServo::printCalibrationData(){
    int maxIndex = sizeof(points.anglePoints)/sizeof(*points.anglePoints) - 1;
    Serial.println("Number, Angle, Feedback");
    for(int i = 0; i <= maxIndex; i++){
        Serial.printf("%d %d %d\n", i, points.anglePoints[i], points.feedbackPoints[i]);
    }
}

void FeedbackServo::setTargetAngle(int angle){
    angle = constrain(angle, _minAngle, _maxAngle);
    servo->setTargetDeg(angle);
    _inProcess = true;
}

boolean FeedbackServo::tickManual() {
    bool success = servo->tickManual();

    if (!success){
        return false;
    }

    // ждем какое-то время чтобы факт догнал расчетные значения
    if (_inProcess){
        int curAngle = getCurrentAngle();
        int targetAngle = servo->getTargetDeg();
        // int curDelta = constrain(targetAngle - curAngle, -_accurancyDeg, _accurancyDeg);

        if (abs(targetAngle - curAngle) > _accurancyDeg){
            
            static uint32_t delayTmr;
            if (delayTmr == 0){
                delayTmr = millis();
            }

            log_d("tickManual() curAngle: %d", curAngle);

            if (millis() - delayTmr >= _finalizeDelay){
                servo->write(curAngle);
                servo->setCurrentDeg(curAngle);
                delayTmr = 0;
                _inProcess = false;

                log_d("tickManual() curAngle: %d timeout", curAngle);
            }
        }
        else{
            _inProcess = false;
            log_d("tickManual() curAngle: %d finish", curAngle);
        }
    }

    return true;
}

FeedbackInterval FeedbackServo::getFeedbackInterval(int curFeedback){

    FeedbackInterval res;
    int maxIndex = sizeof(points.anglePoints)/sizeof(*points.anglePoints) - 1;

    if(curFeedback <= points.feedbackPoints[0]){
            res.minFeedback = curFeedback - 1;
            res.maxFeedback = curFeedback + 1;
            res.minAngle = points.anglePoints[0] - 1;
            res.maxAngle = points.anglePoints[0] + 1;
            return res;
    }

    if(curFeedback >= points.feedbackPoints[maxIndex]){
        res.minFeedback = curFeedback - 1;
        res.maxFeedback = curFeedback + 1;
        res.minAngle = points.anglePoints[maxIndex] - 1;
        res.maxAngle = points.anglePoints[maxIndex] + 1;
        return res;
    }

    for (int i = 0; i <= maxIndex; i++){
        if(curFeedback < points.feedbackPoints[i]){
            res.minFeedback = points.feedbackPoints[i-1];
            res.maxFeedback = points.feedbackPoints[i];
            res.minAngle = points.anglePoints[i-1];
            res.maxAngle = points.anglePoints[i];
            return res;
        }
    }

    return res;
}

int FeedbackServo::getCurrentAngle(){
    int feedback = analogRead(feedbackPin);
    FeedbackInterval interval = getFeedbackInterval(feedback);
    int curAngle = map(feedback, interval.minFeedback, interval.maxFeedback, interval.minAngle, interval.maxAngle);
    // log_d("feedback: %d angle: %d", feedback, curAngle);
    return curAngle;
}