#include "WS2812bAnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include "WS2812bAnalyzerCommon.h"

WS2812bAnalyzerSettings::WS2812bAnalyzerSettings()
	:	mInputChannel( UNDEFINED_CHANNEL )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard WS2812b" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mShowLedNumberInterface.reset(new AnalyzerSettingInterfaceBool());
	mShowLedNumberInterface->SetTitleAndTooltip("Show LED number", "Show LED number");
	mShowLedNumberInterface->SetValue(true);

	mAutoBaudInterface.reset(new AnalyzerSettingInterfaceBool());
	mAutoBaudInterface->SetTitleAndTooltip("AutoBaud", "Timing analyze");
	mAutoBaudInterface->SetValue(false);

	mT0HInterface.reset(new AnalyzerSettingInterfaceInteger());
	mT0HInterface->SetTitleAndTooltip("T0H ns", "T0H ns");
	mT0HInterface->SetMin(1);
	mT0HInterface->SetMax(2000);
	mT0HInterface->SetInteger(T0Hns);

	mT0LInterface.reset(new AnalyzerSettingInterfaceInteger());
	mT0LInterface->SetTitleAndTooltip("T0L ns", "T0L ns");
	mT0LInterface->SetMin(1);
	mT0LInterface->SetMax(2000);
	mT0LInterface->SetInteger(T0Lns);

	mT1HInterface.reset(new AnalyzerSettingInterfaceInteger());
	mT1HInterface->SetTitleAndTooltip("T1H ns", "T1H ns");
	mT1HInterface->SetMin(1);
	mT1HInterface->SetMax(2000);
	mT1HInterface->SetInteger(T1Hns);

	mT1LInterface.reset(new AnalyzerSettingInterfaceInteger());
	mT1LInterface->SetTitleAndTooltip("T1L ns", "T1L ns");
	mT1LInterface->SetMin(1);
	mT1LInterface->SetMax(2000);
	mT1LInterface->SetInteger(T1Lns);

	mTDeltaInterface.reset(new AnalyzerSettingInterfaceInteger());
	mTDeltaInterface->SetTitleAndTooltip("TDelta ns", "TDelta ns");
	mTDeltaInterface->SetMin(1);
	mTDeltaInterface->SetMax(500);
	mTDeltaInterface->SetInteger(TDELTAns);

	mTMinimumResetInterface.reset(new AnalyzerSettingInterfaceInteger());
	mTMinimumResetInterface->SetTitleAndTooltip("TMinimumReset ns", "TMinimumReset ns");
	mTMinimumResetInterface->SetMin(1250);
	mTMinimumResetInterface->SetMax(INT32_MAX);
	mTMinimumResetInterface->SetInteger(TRESETns);	// 9 us

	AddInterface(mInputChannelInterface.get());
	AddInterface(mShowLedNumberInterface.get());
	AddInterface(mAutoBaudInterface.get());
	AddInterface(mT0HInterface.get());
	AddInterface(mT0LInterface.get());
	AddInterface(mT1HInterface.get());
	AddInterface(mT1LInterface.get());
	AddInterface(mTDeltaInterface.get());
	AddInterface(mTMinimumResetInterface.get());

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "text", "txt");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();
	AddChannel(mInputChannel, "WS2812", false);
}

WS2812bAnalyzerSettings::~WS2812bAnalyzerSettings()
{
}

bool WS2812bAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mShowLedNumber = mShowLedNumberInterface->GetValue();
	mAutoBaud = mAutoBaudInterface->GetValue();
	mT0H = mT0HInterface->GetInteger();
	mT0L = mT0LInterface->GetInteger();
	mT1H = mT1HInterface->GetInteger();
	mT1L = mT1LInterface->GetInteger();
	mTDelta = mTDeltaInterface->GetInteger();
	mTMinimumReset = mTMinimumResetInterface->GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "WS2812b", true );

	return true;
}

void WS2812bAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mShowLedNumberInterface->SetValue(mShowLedNumber);
	mAutoBaudInterface->SetValue(mAutoBaud);
	mT0HInterface->SetInteger(mT0H);
	mT0LInterface->SetInteger(mT0L);
	mT1HInterface->SetInteger(mT1H);
	mT1LInterface->SetInteger(mT1L);
	mTDeltaInterface->SetInteger(mTDelta);
	mTMinimumResetInterface->SetInteger(mTMinimumReset);
}

void WS2812bAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mShowLedNumber;
	text_archive >> mAutoBaud;
	text_archive >> mT0H;
	text_archive >> mT0L;
	text_archive >> mT1H;
	text_archive >> mT1L;
	text_archive >> mTDelta;
	text_archive >> mTMinimumReset;

	if (-1 == mT0H)
	{
		mT0H = T0Hns;
	}
	if (-1 == mT0L)
	{
		mT0L = T0Lns;
	}
	if (-1 == mT1H)
	{
		mT1H = T1Hns;
	}
	if (-1 == mT1L)
	{
		mT1L = T1Lns;
	}
	if (-1 == mTDelta)
	{
		mT0H = TDELTAns;
	}
	if (-1 == mTMinimumReset)
	{
		mTMinimumReset = TRESETns;
	}

	ClearChannels();
	AddChannel( mInputChannel, "WS2812b", true );

	UpdateInterfacesFromSettings();
}

const char* WS2812bAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mShowLedNumber;
	text_archive << mAutoBaud;
	text_archive << mT0H;
	text_archive << mT0L;
	text_archive << mT1H;
	text_archive << mT1L;
	text_archive << mTDelta;
	text_archive << mTMinimumReset;

	return SetReturnString( text_archive.GetString() );
}
