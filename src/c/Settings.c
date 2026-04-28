#include "Settings.h"

#define SETTINGS_KEY 1

void settings_load(ClaySettings *settings) 
{
	//***REMEMBER TO UPDATE ClaySettings STRUCT IN HEADER FILE!***
    // Default values
    settings->BackgroundColor   = GColorBlack;
    settings->DateColor         = GColorWhite;
    settings->BatteryColor      = GColorVeryLightBlue;
    settings->StepsColor        = GColorOrange;
    settings->ShowDate          = true;
    settings->ShowBattery       = true;
    settings->ShowStepProgress  = true;
	settings->ShowBTConnection  = true;
	settings->VibrateOnStepGoal = true;
    settings->StepGoal          = 8000;

    if (persist_exists(SETTINGS_KEY)) 
	{
        persist_read_data(SETTINGS_KEY, settings, sizeof(ClaySettings));
    }
}

void settings_save(ClaySettings *settings) 
{
    persist_write_data(SETTINGS_KEY, settings, sizeof(ClaySettings));
}