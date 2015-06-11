#pragma once

struct ExternalSpeakerMonitorTestResult
{
	ExternalSpeakerMonitorTestResult() : 
		passed(false),
		voltage(0.0f),
		current(0.0f),
		resistance(0.0f)
	{
	}

	bool passed;
	String message;

	float voltage;
	float current;
	float resistance;
};

