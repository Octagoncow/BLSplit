module.exports = [
  {
    "type": "heading",
    "defaultValue": "Watchface Settings"
  },
  {
    "type": "text",
    "defaultValue": "Customize your watchface appearance and preferences."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color"
      },
	{
        "type": "color",
        "messageKey": "DateColor",
        "defaultValue": "0xFFFFFF",
        "label": "Date Color"
      },
		{
        "type": "color",
        "messageKey": "BatteryColor",
        "defaultValue": "0x5500FF",
        "label": "Battery % Color"
      },
		{
        "type": "color",
        "messageKey": "StepsColor",
        "defaultValue": "0xFF5500",
        "label": "Step Progress Color"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Hide Elements"
      },
      {
        "type": "toggle",
        "messageKey": "ShowDate",
        "label": "Show Date",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "ShowBattery",
        "label": "Show Battery %",
        "defaultValue": true
      },
		{
        "type": "toggle",
        "messageKey": "ShowStepProgress",
        "label": "Show Step Goal Progress",
        "defaultValue": true
      },
		{
        "type": "toggle",
        "messageKey": "ShowBTConnection",
        "label": "Change Blade Color on BT Disconnect",
        "defaultValue": true
      }
    ]
  },
			{
    "type": "section",
    "items": 
				[
      {
        "type": "heading",
        "defaultValue": "Health"
      },
      {
				"type": "input",
				"messageKey": "StepGoal",
				"label": "Step Goal",
				"defaultValue": "8000",
				"attributes": 
				{
					"type": "number",
					"min": 1000,
					"max": 30000,
					"placeholder": "Enter step goal"
				}
			},
		{
        "type": "toggle",
        "messageKey": "VibrateOnStepGoal",
        "label": "Vibrate On Step Goal",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
