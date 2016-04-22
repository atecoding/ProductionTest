#pragma once
#include "CalibrationDataV2.h"

class CalibrationManagerV2;

class CalibrationComponentV2 : public Component, public Button::Listener, public Value::Listener
{
public:
	CalibrationComponentV2(CalibrationManagerV2* calibrationManager_);
	~CalibrationComponentV2();

	void update();
    
    virtual void valueChanged(Value& value) override;
    virtual void visibilityChanged() override;
	virtual void paint(Graphics& g) override;
	virtual void resized() override;

	virtual void buttonClicked(Button* buttonClicked) override;

	virtual void userTriedToCloseWindow() override;

protected:
	CalibrationManagerV2* calibrationManager;
    Value calibrationStateValue;

    TextButton nextButton;
	TextButton cancelButton;

	class PromptComponent : public Component
	{
	public:
		PromptComponent();
		virtual void resized() override;
        
        virtual void paint(Graphics& g) override;

		Label label;
		//TextButton continueButton;
	} prompt;

	class DataDisplayComponent : public Component
	{
	public:
		DataDisplayComponent(CalibrationManagerV2* calibrationManager_ );
		~DataDisplayComponent();

		virtual void paint(Graphics& g) override;
		void paintValue(Graphics &g, String const text, uint16 const value, juce::Rectangle<int> &left, juce::Rectangle<int> &right, Font &boldFont, Font &normalFont);
		virtual void resized() override;

		CalibrationManagerV2* calibrationManager;

		TextButton calibrateButton;
		//TextButton measureButton;
        TextButton resetButton;
        TextButton eraseButton;
		TextEditor historyDisplay;
	} dataDisplay;


	class ResultsDisplayComponent : public Component
	{
	public:
		ResultsDisplayComponent(CalibrationManagerV2* calibrationManager_);

		virtual void paint(Graphics& g) override;

		CalibrationManagerV2* calibrationManager;

	} resultsDisplay;

	class RecordProgressComponent : public Component
	{
	public:
		RecordProgressComponent(CalibrationManagerV2* calibrationManager_);

		virtual void paint(Graphics& g) override;
		virtual void resized() override;

		CalibrationManagerV2* calibrationManager;
		ProgressBar progressBar;
	} recordProgress;
};
