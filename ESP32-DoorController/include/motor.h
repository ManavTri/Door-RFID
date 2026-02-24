#pragma once
#include <Arduino.h>
#include "event.h"
class MotorController
{
private:
    static constexpr int STEPS_PER_REV = 200;
    static constexpr float OPEN_ROTATIONS = 5.8;
    static constexpr int TOTAL_STEPS = STEPS_PER_REV * OPEN_ROTATIONS;
    static constexpr int MICRO_DELAY = 1000;

    int step_pin;
    int dir_pin;
    int relay_pin;
    bool moving;
    int current_steps;
    int target_steps;
    bool step_high;
    unsigned long last_toggle_time;

    void start();
public:
    MotorController(int step_pin, int dir_pin, int relay_pin);
    void begin();
    void open();
    void close();
    void stop();
    void update();
    bool isDone() const;
};