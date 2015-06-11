#pragma once

struct SquareWaveAnalysisResult
{
	SquareWaveAnalysisResult()
	{
		clear(0.0f);
	}

	void add(float sample)
	{
		average += sample;
		min = jmin(min, sample);
		max = jmax(max, sample);
	}

	void clear(float const sign)
	{
		if (sign < 0.0f)
		{
			min = 0.0f;
			max = -FLT_MAX;
			average = 0.0f;
		}
		else
		{
			min = FLT_MAX;
			max = 0.0f;
			average = 0.0f;
		}
	}


	float min;
	float max;
	float average;
};