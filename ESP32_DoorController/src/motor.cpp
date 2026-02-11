#include "motor.h"

MotorController::MotorController(int step_pin, int dir_pin, int relay_pin)
    : step_pin(step_pin)
    , dir_pin(dir_pin)
    , relay_pin(relay_pin)
{}

void MotorController::start() {
    moving = true;
    current_steps = 0;
    step_high = false;
    last_toggle_time = micros();
    digitalWrite(step_pin, LOW);
}

void MotorController::begin() {
    pinMode(step_pin, OUTPUT);
    pinMode(dir_pin, OUTPUT);
    digitalWrite(step_pin, LOW);
}

void MotorController::open() {
    digitalWrite(dir_pin, HIGH);
    start();
}

void MotorController::close() {
    digitalWrite(dir_pin, LOW);
    start();
}

void MotorController::stop() {
    moving = false;
    current_steps = 0;
    step_high = false;
    digitalWrite(step_pin, LOW);
}

void MotorController::update() {
    if (!moving) return;

    unsigned long now = micros();
    if (now - last_toggle_time < MICRO_DELAY) return;
    last_toggle_time = now;

    if (!step_high) {
        step_high = true;
        digitalWrite(step_pin, HIGH);
    } else {
        step_high = false;
        digitalWrite(step_pin, LOW);

        if (++current_steps >= TOTAL_STEPS)
            stop();
    }
}

bool MotorController::isDone() const {
    return !moving;
}