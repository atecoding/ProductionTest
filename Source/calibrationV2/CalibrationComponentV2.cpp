#include "../base.h"
#include "CalibrationComponentV2.h"
#include "CalibrationManagerV2.h"
#include "../USBDevice.h"
#include "../Description.h"

CalibrationComponentV2::CalibrationComponentV2(CalibrationManagerV2* calibrationManager_) :
calibrationManager(calibrationManager_),
nextButton("Continue"),
cancelButton("Cancel"),
dataDisplay(calibrationManager_),
resultsDisplay(calibrationManager_),
recordProgress(calibrationManager_)
{
	setOpaque(true);
	addAndMakeVisible(nextButton);
	addAndMakeVisible(cancelButton);
	addChildComponent(dataDisplay);
	addChildComponent(resultsDisplay);
	addChildComponent(recordProgress);
	addChildComponent(prompt);

	nextButton.addListener(this);
    cancelButton.setWantsKeyboardFocus(false);
	cancelButton.addListener(this);
	dataDisplay.calibrateButton.addListener(this);
	//dataDisplay.measureButton.addListener(this);
    dataDisplay.resetButton.addListener(this);
    dataDisplay.eraseButton.addListener(this);
	
    calibrationManager_->addStateListener(this);
}

CalibrationComponentV2::~CalibrationComponentV2()
{
	DBG("CalibrationComponentV2::~CalibrationComponentV2");
    
    calibrationManager->removeStateListener(this);
}

void CalibrationComponentV2::paint(Graphics& g)
{
	g.fillAll(Colours::white);
    
    g.setColour(Colours::lightslategrey);
    juce::Rectangle<float> r(getLocalBounds().toFloat());
    g.drawRect(r.reduced(1.0f,1.0f), 3);

	//Font f(g.getCurrentFont());
	Font bigFont(16.0f, Font::bold);

    g.setColour(Colours::black);
	g.setFont(bigFont);
	int y = 20;
	int x = 40;
	int w = getWidth() - (x * 2);
	int h = 25;

    String title("Echo AIO Calibration");
	g.drawText(title, x, y, w, h, Justification::centred, false);
}

void CalibrationComponentV2::resized()
{
	int buttonW = 120;
	int buttonH = 30;

	int x = 40;
	int y = 60;
    int displayH = getHeight() - y - buttonH - 30;
	juce::Rectangle<int> inner(x, y, getWidth() - x * 2, displayH);
	dataDisplay.setBounds(inner);
    
    inner = inner.withSizeKeepingCentre(640, 300);
	resultsDisplay.setBounds(inner);
    recordProgress.setBounds(inner);
    prompt.setBounds(inner);
	
    y = resultsDisplay.getBottom() + 10;
    if (nextButton.isVisible())
    {
        int centreX = getWidth() / 2;
        x = centreX - buttonW - 10;
        nextButton.setBounds(x, y, buttonW, buttonH);
        x = centreX + 10;
        cancelButton.setBounds(x, y, buttonW, buttonH);
    }
    else
    {
        x = (getWidth() - buttonW) / 2;
        cancelButton.setBounds(x, y, buttonW, buttonH);
    }
}

void CalibrationComponentV2::buttonClicked(Button* buttonClicked)
{
    Result result(Result::ok());
    
	if (buttonClicked == &dataDisplay.calibrateButton)
	{
		result = calibrationManager->userAction(CalibrationManagerV2::ACTION_CALIBRATE);
	}

	if (buttonClicked == &nextButton)
	{
        CalibrationManagerV2::State const state( calibrationManager->getState());
        switch (state)
        {
            case CalibrationManagerV2::STATE_ERROR:
                calibrationManager->userAction(CalibrationManagerV2::ACTION_RESTART_MODULE_CALIBRATION);
                break;
                //
                // Fall through
                //
                
            case CalibrationManagerV2::STATE_MODULE_READY:
                if (result.wasOk())
                    result = calibrationManager->userAction(CalibrationManagerV2::ACTION_CALIBRATE);
                break;
                
            default:
                break;
        }
	}

#if 0
	if (buttonClicked == &dataDisplayAIOS.measureButton)
	{
		calibrationManager->userAction(MEASURE);
		return;
	}
#endif
    
    if (buttonClicked == &dataDisplay.resetButton)
    {
        int answer = AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, JUCEApplication::getInstance()->getApplicationName(),
            "Temporarily reset RAM calibration settings?  Calibration data will be still be saved in flash.", "Reset", "Don't reset");

        if (answer)
        {
            result = calibrationManager->userAction(CalibrationManagerV2::ACTION_RESET_RAM_DATA);
            if (result.failed())
            {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), "Error resetting RAM data: " + result.getErrorMessage());
            }
        }
        
        update();
        return;
    }
    
    if (buttonClicked == &dataDisplay.eraseButton)
    {
        int answer = AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, JUCEApplication::getInstance()->getApplicationName(),
            "Permanently erase calibration settings?  Calibration data will be erased from flash.", "Erase", "Don't erase");

        if (answer)
        {
			result = calibrationManager->userAction(CalibrationManagerV2::ACTION_ERASE_FLASH_DATA);
            if (result.failed())
            {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), "Error erasing flash data: " + result.getErrorMessage());
            }
        }
        
        update();
        return;
    }

	if (buttonClicked == &cancelButton)
	{
		calibrationManager->userAction(CalibrationManagerV2::ACTION_CANCEL);
	}
    
    if (result.failed())
    {
        AlertWindow::showMessageBox(AlertWindow::InfoIcon, JUCEApplication::getInstance()->getApplicationName(),
                                    result.getErrorMessage(),
                                    "Close");
    }
}

void CalibrationComponentV2::userTriedToCloseWindow()
{
	calibrationManager->userAction(CalibrationManagerV2::ACTION_CANCEL);
}

void CalibrationComponentV2::visibilityChanged()
{
    if (isVisible())
    {
        update();
    }
}

void CalibrationComponentV2::valueChanged(Value &value)
{
    //CalibrationManagerV2::State const state( calibrationManager->getState() );
    //DBG("CalibrationComponentV2::valueChanged() " << (int)state);
    
    update();
}

void CalibrationComponentV2::update()
{
    CalibrationManagerV2::State const state( calibrationManager->getState() );
    
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        getChildComponent(i)->setVisible(false);
    }
    
    cancelButton.setButtonText("Cancel");

	switch (state)
	{
	case CalibrationManagerV2::STATE_IDLE:
        break;
            
	case CalibrationManagerV2::STATE_SHOW_ACTIVE_CALIBRATION:
		{
			String historyText(calibrationManager->getHistory());
			dataDisplay.historyDisplay.setText(historyText, false);
	
			dataDisplay.setVisible(true);
            break;
		}
            
    case CalibrationManagerV2::STATE_RECORDING:
    case CalibrationManagerV2::STATE_ANALYZING:
        {
            recordProgress.setVisible(true);
            cancelButton.setVisible(true);
            break;
        }
            
    case CalibrationManagerV2::STATE_MODULE_READY:
        {
            prompt.label.setText(calibrationManager->getProcedure()->getConnectPrompt(), dontSendNotification);
            prompt.setVisible(true);
            
            nextButton.setButtonText("Continue");
            nextButton.setVisible(true);
            cancelButton.setVisible(true);
            break;
        }
            
    case CalibrationManagerV2::STATE_ERROR:
        {
            prompt.label.setText("Error: " + calibrationManager->getResult().getErrorMessage(), dontSendNotification);
            prompt.setVisible(true);
            
            nextButton.setButtonText("Restart");
            nextButton.setVisible(true);
            cancelButton.setVisible(true);
            break;
        }
            
    default:
        break;
	}
    
    resized();
	repaint();
}


CalibrationComponentV2::DataDisplayComponent::DataDisplayComponent(CalibrationManagerV2* calibrationManager_) :
calibrationManager(calibrationManager_),
calibrateButton("Calibrate", "Start calibration procedure"),
//measureButton("Measure", "Measure external resistance"),
resetButton("Reset", "Reset RAM calibration data"),
eraseButton("Erase", "Erase flash calibration data")
{
	historyDisplay.setReadOnly(true);
	historyDisplay.setMultiLine(true);
	historyDisplay.setCaretVisible(false);
	historyDisplay.setColour(TextEditor::outlineColourId, Colours::lightslategrey);

	addAndMakeVisible(calibrateButton);
	//addAndMakeVisible(measureButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(eraseButton);
	addAndMakeVisible(historyDisplay);
    
    //measureButton.setEnabled(false);
    //resetButton.setEnabled(false);
    //eraseButton.setEnabled(false);
}

CalibrationComponentV2::DataDisplayComponent::~DataDisplayComponent()
{
}


void CalibrationComponentV2::DataDisplayComponent::paint(Graphics& g)
{
    CalibrationDataV2 const &data(calibrationManager->getData());
	Font normalFont(g.getCurrentFont());
	Font boldFont(normalFont.boldened());
    
	int h = 15;
	juce::Rectangle<int> left(40, 0, 120, h);
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

	left.translate(20, 2 * h);
	right.translate(20, 2 * h);
    
    int startY = left.getY();
    for (int channel = 0; channel < numElementsInArray(data.data.inputGains); ++channel)
    {
        paintValue(g, "Input " + String(channel + 1) + ":", data.data.inputGains[channel], left, right, boldFont, normalFont);
    }
    
    left.setPosition( left.getX() + 320, startY);
    right.setPosition(left.translated(left.getWidth(), 0).getTopLeft());
    
    for (int channel = 0; channel < numElementsInArray(data.data.outputGains); ++channel)
    {
        paintValue(g, "Output " + String(channel + 1) + ":", data.data.outputGains[channel], left, right, boldFont, normalFont);
    }
}


void CalibrationComponentV2::DataDisplayComponent::paintValue(Graphics &g, String const text, uint16 const value, juce::Rectangle<int> &left, juce::Rectangle<int> &right, Font &boldFont, Font &normalFont)
{
	g.setFont(boldFont);
	g.drawText(text, left, Justification::centredLeft, false);
	g.setFont(normalFont);

	String rightText(CalibrationDataV2::printValue(value));
	g.drawText(rightText, right, Justification::centredLeft, false);
	left.translate(0, left.getHeight());
	right.translate(0, right.getHeight());
}

void CalibrationComponentV2::DataDisplayComponent::resized()
{
	int buttonH = 25;
    int historyDisplayY = 178;
	int historyDisplayH = getHeight() - historyDisplayY - buttonH - 20;;
	historyDisplay.setBounds(0, historyDisplayY, getWidth(), historyDisplayH);
    
	{
		int buttonW = 80;
		int y = getHeight() - buttonH/2 - 2;
		float x = 0.24f;

		calibrateButton.setSize(buttonW, buttonH);
		calibrateButton.setCentrePosition(proportionOfWidth(x), y);
		
		x += 0.16f;

		//measureButton.setSize(buttonW, buttonH);
		//measureButton.setCentrePosition(proportionOfWidth(x), y);
        
		x += 0.16f;

        resetButton.setSize(buttonW, buttonH);
        resetButton.setCentrePosition(proportionOfWidth(x), y);
        
		x += 0.16f;

		eraseButton.setSize(buttonW, buttonH);
        eraseButton.setCentrePosition(proportionOfWidth(x), y);
	}
}


CalibrationComponentV2::ResultsDisplayComponent::ResultsDisplayComponent(CalibrationManagerV2* calibrationManager_) :
calibrationManager(calibrationManager_)
{

}

void CalibrationComponentV2::ResultsDisplayComponent::paint(Graphics& g)
{
#if 0
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

	CalibrationManagerV2::State const state(calibrationManager->getState());
	switch (state)
	{
	case CalibrationManagerV2::STATE_SHOW_VOLTAGE_INPUT_RESULTS:
		text = "SPKR V input calibration results";
		numResults = 1;
		channel = calibrationManager->SPKRVinputChannel;
		break;

	case CalibrationManagerV2::STATE_SHOW_OUTPUT_RESULTS:
		text = "SPKR V output calibration results";
		numResults = 1;
		channel = calibrationManager->SPKRVinputChannel;
		break;

	case CalibrationManagerV2::STATE_SHOW_CURRENT_INPUT_RESULTS:
		text = "SPKR I input calibration results";
		numResults = 2;
		channel = calibrationManager->SPKRVinputChannel;
		break;

	case CalibrationManagerV2::STATE_SHOW_RESISTANCE_MEASUREMENT:
		text = "External resistance results: ";
		numResults = 2;
		channel = calibrationManager->SPKRVinputChannel;
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

		if (CalibrationManagerV2::STATE_SHOW_RESISTANCE_MEASUREMENT == state)
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
#endif
}

CalibrationComponentV2::RecordProgressComponent::RecordProgressComponent(CalibrationManagerV2* calibrationManager_):
calibrationManager(calibrationManager_),
progressBar(calibrationManager_->getRecordProgress())
{
    progressBar.setColour(ProgressBar::foregroundColourId, Colours::lightblue);
	addAndMakeVisible(progressBar);
}

void CalibrationComponentV2::RecordProgressComponent::resized()
{
	progressBar.centreWithSize(200, 25);
}

void CalibrationComponentV2::RecordProgressComponent::paint(Graphics& g)
{
    juce::Rectangle<float> r(getLocalBounds().toFloat());
    g.setColour(Colour(0x40cacaca));
    g.fillRoundedRectangle(r, getHeight() * 0.03f);
    
	g.setFont( Font(16.0f, Font::bold));
    g.setColour(Colours::black);
    String text(calibrationManager->getProcedure()->getProgressLabelText());
    g.drawText(text, 0, progressBar.getY() - 50, getWidth(), 24, Justification::centred, false);
}


CalibrationComponentV2::PromptComponent::PromptComponent()
{
    label.setJustificationType(Justification::centred);
	addAndMakeVisible(label);
}

void CalibrationComponentV2::PromptComponent::resized()
{
	label.centreWithSize(getWidth(), 150);
}

void CalibrationComponentV2::PromptComponent::paint(Graphics& g)
{
    juce::Rectangle<float> r(getLocalBounds().toFloat());
    g.setColour(Colour(0x40cacaca));
    g.fillRoundedRectangle(r, getHeight() * 0.03f);
}

