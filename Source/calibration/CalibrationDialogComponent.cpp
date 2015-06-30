#include "../base.h"
#include "CalibrationDialogComponent.h"
#include "CalibrationManager.h"
#include "ehw.h"
#include "../DescriptionAIO.h"

CalibrationDialogComponent::CalibrationDialogComponent(CalibrationManager* calibrationManager_, ReferenceCountedObjectPtr<ehw>  device_) :
calibrationManager(calibrationManager_),
device(device_),
nextButton("Next"),
cancelButton("Cancel"),
dataDisplayAIOS(calibrationManager_->calibrationDataAIOS),
externalSpeakerMonitorStart(calibrationManager_),
externalSpeakerMonitorDone(calibrationManager_),
resultsDisplay(calibrationManager_),
recordProgress(calibrationManager_)
{
	setOpaque(true);

	addAndMakeVisible(nextButton);
	addAndMakeVisible(cancelButton);
	addChildComponent(dataDisplayAIOS);
	addChildComponent(resultsDisplay);
	addChildComponent(recordProgress);
	addChildComponent(prompt);
	addChildComponent(externalSpeakerMonitorStart);
	addChildComponent(externalSpeakerMonitorDone);

	nextButton.addListener(this);
    cancelButton.setWantsKeyboardFocus(false);
    cancelButton.addShortcut(KeyPress(KeyPress::escapeKey));
	cancelButton.addListener(this);
	dataDisplayAIOS.calibrateButton.addListener(this);
	dataDisplayAIOS.measureButton.addListener(this);
    dataDisplayAIOS.resetButton.addListener(this);
    dataDisplayAIOS.eraseButton.addListener(this);
	dataDisplayAIOS.closeButton.addListener(this);
	prompt.continueButton.addListener(this);
	externalSpeakerMonitorStart.startTestButton.addListener(this);
	externalSpeakerMonitorDone.externalSpeakerMonitorDoneButton.addListener(this);
}

CalibrationDialogComponent::~CalibrationDialogComponent()
{
	DBG("CalibrationDialogComponent::~CalibrationDialogComponent");
}

void CalibrationDialogComponent::paint(Graphics& g)
{
	g.fillAll(Colours::white);
    
    g.setColour(Colours::lightslategrey);
    juce::Rectangle<float> r(getLocalBounds().toFloat());
    g.drawRect(r.reduced(1.0f,1.0f), 3);

	//Font f(g.getCurrentFont());
	Font bigFont(16.0f, Font::bold);

    g.setColour(Colours::black);
	g.setFont(bigFont);
	int y = 40;
	int x = 40;
	int w = getWidth() - (x * 2);
	int h = 25;

	String title;
	switch (device->getDescription()->getModuleType(0))
	{
	case ACOUSTICIO_ANALOG_MODULE :
		title = "Echo Speaker Monitor ";
		switch (calibrationManager->getState())
		{
		case CalibrationManager::STATE_EXTERNAL_SPEAKER_MONITOR_READY:
		case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK:
		case CalibrationManager::STATE_START_EXTERNAL_SPEAKER_MONITOR_TEST:
		case CalibrationManager::STATE_FINISH_EXTERNAL_SPEAKER_MONITOR_TEST:
			title += "Test";
			break;

		case CalibrationManager::STATE_CONNECT_PRODUCTION_TEST_ADAPTER:
		case CalibrationManager::STATE_PROMPT_CONNECT_LOOPBACK_MIC2:
			title += "AIO Calibration";
			break;
            
        default:
            break;
		}
		break;

	case ACOUSTICIO_SPKRMON_MODULE:
		title = "Echo AIO-S Calibration";
		break;
	}
	g.drawText(title, x, y, w, h, Justification::centred, false);
}

void CalibrationDialogComponent::resized()
{
	int w = 80;
	int h = 20;
	nextButton.setBounds(getWidth() - w * 2 - 20, getHeight() - h - 10, w, h);
	cancelButton.setBounds(nextButton.getBounds().translated(w + 10, 0));

	int x = 40;
	int y = 80;
	juce::Rectangle<int> inner(x, y, getWidth() - x * 2, nextButton.getY() - y - 10);
	dataDisplayAIOS.setBounds(inner);
	resultsDisplay.setBounds(inner);
	recordProgress.setBounds(inner);
	prompt.setBounds(inner);
	externalSpeakerMonitorStart.setBounds(inner);
	externalSpeakerMonitorDone.setBounds(inner);
}

void CalibrationDialogComponent::buttonClicked(Button* buttonClicked)
{
	if (buttonClicked == &nextButton || buttonClicked == &dataDisplayAIOS.calibrateButton)
	{
		exitModalState(CALIBRATE);
		return;
	}

	if (buttonClicked == &dataDisplayAIOS.measureButton)
	{
		exitModalState(MEASURE);
		return;
	}
    
    if (buttonClicked == &dataDisplayAIOS.resetButton)
    {
        exitModalState(RESET_RAM_DATA);
        return;
    }
    
    if (buttonClicked == &dataDisplayAIOS.eraseButton)
    {
        exitModalState(ERASE_FLASH_DATA);
        return;
    }

	if (buttonClicked == &cancelButton || buttonClicked == &dataDisplayAIOS.closeButton)
	{
#if SPEAKER_MONITOR_TEST
		JUCEApplication::quit();
#else
		exitModalState(CANCEL);
#endif
		return;
	}

	if (buttonClicked == &prompt.continueButton)
	{
		exitModalState(CONTINUE);
		return;
	}

	if (buttonClicked == &externalSpeakerMonitorStart.startTestButton)
	{
		Result result(calibrationManager->setSerialNumber(externalSpeakerMonitorStart.serialNumberEditor.getText()));
		if (result.failed())
		{
			AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Speaker Monitor Test", result.getErrorMessage());
			externalSpeakerMonitorStart.serialNumberEditor.grabKeyboardFocus();
			return;
		}

		exitModalState(START_EXTERNAL_SPKRMON_TEST);
		return;
	}

	if (buttonClicked == &externalSpeakerMonitorDone.externalSpeakerMonitorDoneButton)
	{
		exitModalState(EXTERNAL_SPEAKER_MONITOR_DONE);
	}
}

void CalibrationDialogComponent::userTriedToCloseWindow()
{
#if SPEAKER_MONITOR_TEST
    int button = AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Speaker Monitor Test",
                                 "Do you want to close the speaker monitor test?",
                                 "Close", "Don't close");
	if (button)
    {
        JUCEApplication::quit();
    }
#else
	exitModalState(CANCEL);
#endif
}

void CalibrationDialogComponent::update()
{
	for (int i = 0; i < getNumChildComponents(); ++i)
	{
		getChildComponent(i)->setVisible(false);
	}

	cancelButton.setButtonText("Cancel");

	switch (calibrationManager->getState())
	{
#if 0
	case CalibrationManager::STATE_SHOW_ACTIVE_AIOS_CALIBRATION:
		{
			String historyText(calibrationManager->getHistory());
			dataDisplayAIOS.historyDisplay.setText(historyText, false);
	
			dataDisplayAIOS.setVisible(true);
		}
		break;
#endif

	case CalibrationManager::STATE_EXTERNAL_SPEAKER_MONITOR_READY:
		{
			externalSpeakerMonitorStart.setVisible(true);
			externalSpeakerMonitorStart.serialNumberEditor.grabKeyboardFocus();
		}
		break;

	case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE:
	case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK:
	case CalibrationManager::STATE_START_CALIBRATE_CURRENT_INPUT:
	case CalibrationManager::STATE_START_RESISTANCE_MEASUREMENT:
	case CalibrationManager::STATE_START_EXTERNAL_SPEAKER_MONITOR_TEST:
		recordProgress.setVisible(true);
		cancelButton.setVisible(true);
		break;

	case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_OUTPUT:
		{
			uint8 moduleType = device->getDescription()->getModuleType(0);
			switch (moduleType)
			{
			case ACOUSTICIO_ANALOG_MODULE:
				break;

			case ACOUSTICIO_SPKRMON_MODULE:
				recordProgress.setVisible(true);
				break;
			}
			cancelButton.setVisible(true);
		}
		break;

	case CalibrationManager::STATE_SHOW_VOLTAGE_INPUT_RESULTS:
	case CalibrationManager::STATE_SHOW_VOLTAGE_OUTPUT_RESULTS:
	case CalibrationManager::STATE_SHOW_CURRENT_INPUT_RESULTS:
		nextButton.setVisible(true);
		resultsDisplay.setVisible(true);
		cancelButton.setVisible(true);
		break;

	case CalibrationManager::STATE_SHOW_RESISTANCE_MEASUREMENT:
		resultsDisplay.setVisible(true);
		cancelButton.setButtonText("Close");
		cancelButton.setVisible(true);
		break;
            
	case CalibrationManager::STATE_CONNECT_PRODUCTION_TEST_ADAPTER:
		prompt.label.setText("Connect the production test adapter USB port to this computer\n\n"
			"Connect the production test adapter MIC1/5 to the AIO MIC1\n\n"
			"Connect the production test adapter 'AIO AMP OUTPUT 1/2' to the AIO AMP1", dontSendNotification);
		prompt.setVisible(true);
		break;

	case CalibrationManager::STATE_PROMPT_CONNECT_LOOPBACK_MIC2:
		prompt.label.setText("Connect the production test adapter MIC1/5 to the AIO MIC2", dontSendNotification);
		prompt.setVisible(true);
		break;

	case CalibrationManager::STATE_FINISH_EXTERNAL_SPEAKER_MONITOR_TEST:
		calibrationManager->getExternalSpeakerMonitorTestResult(externalSpeakerMonitorDone.result);
		externalSpeakerMonitorDone.setVisible(true);
		break;

    default:
        break;
	}

	repaint();
}

CalibrationDialogComponent::AIOSDataDisplayComponent::AIOSDataDisplayComponent(AIOSCalibrationData const &data_) :
data(data_),
calibrateButton("Calibrate", "Start calibration procedure"),
measureButton("Measure", "Measure external resistance"),
resetButton("Reset", "Reset RAM calibration data"),
eraseButton("Erase", "Erase flash calibration data"),
closeButton("Close", "Close this window")
{
	historyDisplay.setReadOnly(true);
	historyDisplay.setMultiLine(true);
	historyDisplay.setCaretVisible(false);
	historyDisplay.setColour(TextEditor::outlineColourId, Colours::lightslategrey);

	addAndMakeVisible(calibrateButton);
	addAndMakeVisible(measureButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(eraseButton);
	addAndMakeVisible(closeButton);
	addAndMakeVisible(historyDisplay);

	closeButton.addShortcut(KeyPress(KeyPress::escapeKey));
}

void CalibrationDialogComponent::AIOSDataDisplayComponent::paint(Graphics& g)
{
	Font normalFont(g.getCurrentFont());
	Font boldFont(normalFont.boldened());

	int h = 20;
	juce::Rectangle<int> left(0, 0, 120, h);
	juce::Rectangle<int> right(left.translated(left.getWidth(), 0));
	right.setWidth(getWidth() - left.getWidth());
	g.setFont(boldFont);
	g.drawText("Date:", left, Justification::centredLeft, false);
	g.setFont(normalFont);
	g.drawText(data.getDate(), right, Justification::centredLeft, false);
	left.translate(0, h);
	right.translate(0, h);

	g.setFont(boldFont);
	g.drawText("Checksum:", left, Justification::centredLeft, false);
	g.setFont(normalFont);
	if (false == data.isChecksumOK())
	{
		g.setColour(Colours::red);
		g.drawText("0x" + String::toHexString((int32)data.data.checksum) + " (checksum error)", right, Justification::centredLeft, false);
		g.setColour(Colours::black);
	}
	else
	{
		g.drawText("0x" + String::toHexString((int32)data.data.checksum) + " (OK)", right, Justification::centredLeft, false);
	}

	left.translate(0, h);
	right.translate(0, h);

	paintValue(g, "Voltage input:", data.data.inputGains[AIOS_VOLTAGE_INPUT_CHANNEL], left, right, boldFont, normalFont);
	paintValue(g, "Current input:", data.data.inputGains[AIOS_CURRENT_INPUT_CHANNEL], left, right, boldFont, normalFont);
	paintValue(g, "Voltage output:", data.data.outputGains[AIOS_VOLTAGE_OUTPUT_CHANNEL], left, right, boldFont, normalFont);
}

void CalibrationDialogComponent::AIOSDataDisplayComponent::paintValue(Graphics &g, String const text, uint16 const value, juce::Rectangle<int> &left, juce::Rectangle<int> &right, Font &boldFont, Font &normalFont)
{
	g.setFont(boldFont);
	g.drawText(text, left, Justification::centredLeft, false);
	g.setFont(normalFont);

	String rightText(AIOSCalibrationData::printValue(value));
	g.drawText(rightText, right, Justification::centredLeft, false);
	left.translate(0, left.getHeight());
	right.translate(0, right.getHeight());
}

void CalibrationDialogComponent::AIOSDataDisplayComponent::resized()
{
	int buttonH = 25;
	int historyDisplayH = proportionOfHeight(0.65f);
	historyDisplay.setBounds(0, getHeight() - historyDisplayH - buttonH - 10, getWidth(), historyDisplayH);

	{
		int buttonW = 80;
		int y = getHeight() - buttonH/2 - 2;
	
		calibrateButton.setSize(buttonW, buttonH);
		calibrateButton.setCentrePosition(proportionOfWidth(0.16f), y);
		
		measureButton.setSize(buttonW, buttonH);
		measureButton.setCentrePosition(proportionOfWidth(0.32f), y);
        
        resetButton.setSize(buttonW, buttonH);
        resetButton.setCentrePosition(proportionOfWidth(0.48f), y);
        
        eraseButton.setSize(buttonW, buttonH);
        eraseButton.setCentrePosition(proportionOfWidth(0.64f), y);
        
		closeButton.setSize(buttonW, buttonH);
		closeButton.setCentrePosition(proportionOfWidth(0.80f), y);
	}
}


CalibrationDialogComponent::ResultsDisplayComponent::ResultsDisplayComponent(CalibrationManager* calibrationManager_) :
calibrationManager(calibrationManager_)
{

}

void CalibrationDialogComponent::ResultsDisplayComponent::paint(Graphics& g)
{
	SquareWaveAnalysisResult positiveResult[2], negativeResult[2];
	int numResults;
	int channel;

	int y = 40;
	int x = 40;
	int w = 350;
	int h = 25;

	String text;
	Font f(g.getCurrentFont());

	g.setFont(f.boldened());

	CalibrationManager::State const state(calibrationManager->getState());
	switch (state)
	{
	case CalibrationManager::STATE_SHOW_VOLTAGE_INPUT_RESULTS:
		text = "Voltage input calibration results";
		numResults = 1;
		channel = AIOS_VOLTAGE_INPUT_CHANNEL;
		break;

	case CalibrationManager::STATE_SHOW_VOLTAGE_OUTPUT_RESULTS:
		text = "Voltage output calibration results";
		numResults = 1;
		channel = AIOS_VOLTAGE_INPUT_CHANNEL;
		break;

	case CalibrationManager::STATE_SHOW_CURRENT_INPUT_RESULTS:
		text = "Current input calibration results";
		numResults = 2;
		channel = AIOS_VOLTAGE_INPUT_CHANNEL;
		break;

	case CalibrationManager::STATE_SHOW_RESISTANCE_MEASUREMENT:
		text = "External resistance results: ";
		numResults = 2;
		channel = AIOS_VOLTAGE_INPUT_CHANNEL;
		break;

	default:
		return;
	}

	calibrationManager->getSquareWaveResults(channel, positiveResult[0], negativeResult[0]);
	if (1 == numResults)
	{
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
	
		g.setFont(f);
		text = "Positive Min: " + String(positiveResult[0].min, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Positive Average: " + String(positiveResult[0].average, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Positive Max: " + String(positiveResult[0].max, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Negative Min: " + String(negativeResult[0].min, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Negative Average: " + String(negativeResult[0].average, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Negative Max: " + String(negativeResult[0].max, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
			text = "Average P-P: " + String(fabs(negativeResult[0].average) + positiveResult[0].average, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
	}
	else
	{
		calibrationManager->getSquareWaveResults(channel + 1, positiveResult[1], negativeResult[1]);

		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;

		g.setFont(f);
		String seperation(" / ");
		text = "Positive Min: " + String(positiveResult[0].min, 6) + seperation + String(positiveResult[1].min, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Positive Average: " + String(positiveResult[0].average, 6) + seperation + String(positiveResult[1].average, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Positive Max: " + String(positiveResult[0].max, 6) + seperation + String(positiveResult[1].max, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Negative Min: " + String(negativeResult[0].min, 6) + seperation + String(negativeResult[1].min, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Negative Average: " + String(negativeResult[0].average, 6) + seperation + String(negativeResult[1].average, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Negative Max: " + String(negativeResult[0].max, 6) + seperation + String(negativeResult[1].max, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h;
		text = "Average P-P: " + String(fabs(negativeResult[0].average) + positiveResult[0].average, 6) + seperation + String(fabs(negativeResult[1].average) + positiveResult[1].average, 6);
		g.drawText(text, x, y, w, 25, Justification::centredLeft, false);
		y += h * 2;

		if (CalibrationManager::STATE_SHOW_RESISTANCE_MEASUREMENT == state)
		{
			g.setFont(f.boldened());
			float voltage = fabs(negativeResult[0].average) + positiveResult[0].average;
			float current = (fabs(negativeResult[1].average) + positiveResult[1].average) * 0.5f;
			String resistanceText("Resistance: ");
			if (current != 0.0f)
			{
				float resistance = voltage / current;
				resistanceText += String(resistance, 3) + " ohms";
			}
			else
			{
				resistanceText += "unknown";
			}
			g.drawText(resistanceText, x, y, w, 25, Justification::centredLeft, false);
		}
	}
}

CalibrationDialogComponent::RecordProgressComponent::RecordProgressComponent(CalibrationManager* calibrationManager_):
calibrationManager(calibrationManager_),
progressBar(calibrationManager_->getRecordProgress())
{
	addAndMakeVisible(progressBar);
}

void CalibrationDialogComponent::RecordProgressComponent::resized()
{
	progressBar.centreWithSize(200, 25);
}

void CalibrationDialogComponent::RecordProgressComponent::paint(Graphics& g)
{
	g.setFont( Font(16.0f, Font::bold));
	
	String text;
	switch (calibrationManager->getState())
	{
	case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_REFERENCE_VOLTAGE:
	case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_INPUT_WITH_LOOPBACK:
		text = "Calibrating voltage input...";
		break;

	case CalibrationManager::STATE_START_CALIBRATE_VOLTAGE_OUTPUT:
		text = "Calibrating output...";
		break;

	case CalibrationManager::STATE_START_CALIBRATE_CURRENT_INPUT:
		text = "Calibrating current input...";
		break;

	case CalibrationManager::STATE_START_RESISTANCE_MEASUREMENT:
		text = "Measuring external resistance...";
		break;

	case CalibrationManager::STATE_START_EXTERNAL_SPEAKER_MONITOR_TEST:
		text = "Testing speaker monitor...";
		break;

	default:
		return;
	}

	g.drawText(text, 0, progressBar.getY() - 30, getWidth(), 24, Justification::centred, false);
}

CalibrationDialogComponent::PromptComponent::PromptComponent() :
continueButton("Continue")
{
	addAndMakeVisible(label);
	addAndMakeVisible(continueButton);
}


void CalibrationDialogComponent::PromptComponent::resized()
{
	label.centreWithSize(getWidth(), 150);
	continueButton.setSize(100, 30);
	continueButton.setCentreRelative(0.5f, 0.75f);
}

CalibrationDialogComponent::ExternalSpeakerMonitorStartComponent::ExternalSpeakerMonitorStartComponent(CalibrationManager* calibrationManager_) :
calibrationManager(calibrationManager_),
serialNumberLabel(String::empty, "Serial number SPM"),
startTestButton("Start Test")
{
	serialNumberLabel.setJustificationType(Justification::centredRight);
	serialNumberEditor.setInputRestrictions(CalibrationManager::serialNumberLength, "0123456789");

	addAndMakeVisible(serialNumberEditor);
	addAndMakeVisible(serialNumberLabel);
	addAndMakeVisible(startTestButton);
	startTestButton.addShortcut(KeyPress(KeyPress::returnKey));
}

void CalibrationDialogComponent::ExternalSpeakerMonitorStartComponent::paint(Graphics& g)
{
	Font normalFont(g.getCurrentFont());
	Font boldFont(normalFont.boldened());

	int h = 20;
	juce::Rectangle<int> left(0, 60, 120, h);
	juce::Rectangle<int> right(left.translated(left.getWidth(), 0));
	right.setWidth(getWidth() - left.getWidth());

	paintValue(g, "Voltage input:", calibrationManager->calibrationDataExternal.voltageInputGain, left, right, boldFont, normalFont);
	paintValue(g, "Current input:", calibrationManager->calibrationDataExternal.currentInputGain, left, right, boldFont, normalFont);
}


void CalibrationDialogComponent::ExternalSpeakerMonitorStartComponent::paintValue(Graphics &g, String const text, float const value, juce::Rectangle<int> &left, juce::Rectangle<int> &right, Font &boldFont, Font &normalFont)
{
	g.setFont(boldFont);
	g.drawText(text, left, Justification::centredLeft, false);
	g.setFont(normalFont);

	String rightText(ExternalSpeakerMonitorCalibrationData::printValue(value));
	g.drawText(rightText, right, Justification::centredLeft, false);
	left.translate(0, left.getHeight());
	right.translate(0, right.getHeight());
}

void CalibrationDialogComponent::ExternalSpeakerMonitorStartComponent::resized()
{
	int editorW = 200;
	int buttonW = 120;
	int h = 25;

	serialNumberEditor.setSize(editorW, h);
	serialNumberEditor.setCentreRelative(0.5f, 0.5f);
	serialNumberLabel.setSize(editorW, h);
	serialNumberLabel.attachToComponent(&serialNumberEditor, true);

	startTestButton.setSize(buttonW, h);
	startTestButton.setCentreRelative(0.5f,0.58f);
}

CalibrationDialogComponent::ExternalSpeakerMonitorDoneComponent::ExternalSpeakerMonitorDoneComponent(CalibrationManager* calibrationManager_) :
calibrationManager(calibrationManager_),
externalSpeakerMonitorDoneButton("Close")
{
	addAndMakeVisible(externalSpeakerMonitorDoneButton);
}

void CalibrationDialogComponent::ExternalSpeakerMonitorDoneComponent::paint(Graphics& g)
{
	juce::Rectangle<float> r(getLocalBounds().toFloat());
	r.reduce(70.0f, 70.0f);

	g.setColour(Colours::lightslategrey.withAlpha(0.25f));
	g.fillRoundedRectangle(r, 10.0f);
// 	g.setColour(Colours::red);
// 	g.drawRect(getLocalBounds());

	g.setColour(Colours::black);
	juce::Rectangle<float> backgroundR(r.withSizeKeepingCentre(100.0f, 70.0f));
	//backgroundR.setY(50.0f);

	juce::Rectangle<float> textR(r);
	textR.setHeight(backgroundR.getY() - r.getY());
	g.setFont(Font(22.0f, Font::bold));
	g.drawText("Speaker Monitor " + calibrationManager->getSerialNumber(), textR, Justification::centred, false);

	Colour c(Colours::limegreen);
	String text("PASS");

	textR.setHeight(50.0f);
	textR.setY(backgroundR.getBottom());
	g.setFont(Font(16.0f, Font::bold));

	if (false == result.passed)
	{
		c = Colours::red;
		text = "FAIL";
	}
	else
	{
		g.drawText("Measured resistance: " + String(result.resistance, 4) + " ohms", textR, Justification::centred, false);
	}

	g.drawText(result.message, textR.translated(0.0f, 25.0f), Justification::centred, false);

	g.fillRoundedRectangle(backgroundR, backgroundR.getHeight() * 0.1f);
	g.setColour(c);
	g.setFont(Font(28.0f, Font::bold));
	g.drawText(text, backgroundR, Justification::centred, false);
}

void CalibrationDialogComponent::ExternalSpeakerMonitorDoneComponent::resized()
{
	int buttonW = 120;
	int h = 25;

	externalSpeakerMonitorDoneButton.setSize(buttonW, h);
	externalSpeakerMonitorDoneButton.setCentreRelative(0.5f, 0.78f);
}