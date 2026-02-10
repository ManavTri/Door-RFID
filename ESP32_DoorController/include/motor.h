#pragma once
#include "event.h"
class MotorController
{
private:
    static constexpr int STEPS_PER_REV = 200;
    static constexpr float OPEN_ROTATIONS = 5.8;
    static constexpr int MICRO_DELAY = 1000;
public:
    MotorController(/* args */);
    void open();
    void close();
    void stop();
    bool isAtOpenLimit();
    bool isAtCloseLimit();
};