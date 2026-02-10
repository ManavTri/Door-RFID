#pragma once
class LCDDisplay
{
private:
    /* data */
public:
    LCDDisplay(/* args */);
    void displayCode(int code);
    void displayStatus(const char* msg);
};