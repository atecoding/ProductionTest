#if 0
#pragma once
#include "CalibrationData.h"
#include "ExternalSpeakerMonitorTestResult.h"

class CalibrationManager;
class ehw;

class CalibrationDialogComponent : public Component, Button::Listener
{
public:
	CalibrationDialogComponent(CalibrationManager* calibrationManager_, ReferenceCountedObjectPtr<ehw>  device_);
	~CalibrationDialogComponent();

	void update();

	virtual void paint(Graphics& g) override;
	virtual void resized() override;

	virtual void buttonClicked(Button* buttonClicked) override;

	virtual void userTriedToCloseWindow() override;

	enum 
	{
		CANCEL,
		CALIBRATE,
		MEASURE,
		CONTINUE,
		START_EXTERNAL_SPKRMON_TEST,
		EXTERNAL_SPEAKER_MONITOR_DONE,
        RESET_RAM_DATA,
        ERASE_FLASH_DATA
	};

protected:
	CalibrationManager* calibrationManager;
	ReferenceCountedObjectPtr<ehw> device;
	TextButton nextButton;
	TextButton cancelButton;

	class PromptComponent : public Component
	{
	public:
		PromptComponent();
		virtual void resized() override;

		Label label;
		TextButton continueButton;
	} prompt;

	class AIOSDataDisplayComponent : public Component
	{
	public:
		AIOSDataDisplayComponent(AIOSCalibrationData const &data_);

		virtual void paint(Graphics& g) override;
		void paintValue(Graphics &g, String const text, uint16 const value, juce::Rectangle<int> &left, juce::Rectangle<int> &right, Font &boldFont, Font &normalFont);
		virtual void resized() override;

		AIOSCalibrationData const &data;
		TextButton calibrateButton;
		TextButton measureButton;
        TextButton resetButton;
        TextButton eraseButton;
		TextButton closeButton;
		TextEditor historyDisplay;
	} dataDisplayAIOS;

	class ExternalSpeakerMonitorStartComponent : public Component
	{
	public:
		ExternalSpeakerMonitorStartComponent(CalibrationManager* calibrationManager_);

		virtual void paint(Graphics& g) override;
		void paintValue(Graphics &g, String const text, float const value, juce::Rectangle<int> &left, juce::Rectangle<int> &right, Font &boldFont, Font &normalFont);
		virtual void resized() override;

		CalibrationManager* calibrationManager;
		Label serialNumberLabel;
		TextEditor serialNumberEditor;
		TextButton startTestButton;
	} externalSpeakerMonitorStart;

	class ExternalSpeakerMonitorDoneComponent : public Component
	{
	public:
		ExternalSpeakerMonitorDoneComponent(CalibrationManager* calibrationManager_);

		virtual void paint(Graphics& g) override;
		virtual void resized() override;

		CalibrationManager* calibrationManager;
		ExternalSpeakerMonitorTestResult result;
		TextButton externalSpeakerMonitorDoneButton;
	} externalSpeakerMonitorDone;

	class ResultsDisplayComponent : public Component
	{
	public:
		ResultsDisplayComponent(CalibrationManager* calibrationManager_);

		virtual void paint(Graphics& g) override;

		CalibrationManager* calibrationManager;

	} resultsDisplay;

	class RecordProgressComponent : public Component
	{
	public:
		RecordProgressComponent(CalibrationManager* calibrationManager_);

		virtual void paint(Graphics& g) override;
		virtual void resized() override;

		CalibrationManager* calibrationManager;
		ProgressBar progressBar;
	} recordProgress;
};
#endif