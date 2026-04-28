#pragma once
#include <pebble.h>

typedef struct ClaySettings 
{
    GColor BackgroundColor;
    GColor HourColor;
    GColor MinuteColor;
    GColor DateColor;
    GColor BatteryColor;
    GColor StepsColor;
    bool ShowDate;
    bool ShowBattery;
    bool ShowStepProgress;
	bool ShowBTConnection;
	bool VibrateOnStepGoal;
    int StepGoal;
} ClaySettings;

void settings_load(ClaySettings *settings);
void settings_save(ClaySettings *settings);