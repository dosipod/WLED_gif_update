#ifndef UMBLion_h
#define UMBLion_h

#include "battery_defaults.h"
#include "battery.h"

/**
 *  Lion Battery
 * 
 */
class Lion : public Battery
{
    private:

    public:
        Lion()
        {
            this->setMinVoltage(USERMOD_BATTERY_LION_MIN_VOLTAGE);
            this->setMaxVoltage(USERMOD_BATTERY_LION_MAX_VOLTAGE);
            this->setCapacity(USERMOD_BATTERY_LION_CAPACITY);
            this->setVoltage(this->getVoltage());
            this->setCalibration(USERMOD_BATTERY_LION_CALIBRATION);
        }

        float mapVoltage(float v, float min, float max) override
        {
            return this->linearMapping(v, min, max); // basic mapping
        };

        void calculateAndSetLevel(float voltage) override
        {
            this->setLevel(this->mapVoltage(voltage, this->getMinVoltage(), this->getMaxVoltage()));
        };

        virtual void setMaxVoltage(float voltage) override
        {
            this->maxVoltage = max(getMinVoltage()+1.0f, voltage);
        }
};

#endif