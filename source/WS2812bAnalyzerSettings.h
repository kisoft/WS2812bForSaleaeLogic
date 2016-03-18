#ifndef WS2812B_ANALYZER_SETTINGS
#define WS2812B_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class WS2812bAnalyzerSettings : public AnalyzerSettings
{
public:
	WS2812bAnalyzerSettings();
	virtual ~WS2812bAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	/* ���������� ����� ���������� �� �����*/
	bool mShowLedNumber;

	/* ������� ��������������� ��������� ���������� �������������� ������� � ������ �� � ��������� */
	bool mAutoBaud;

	/* �������� ��� ���������� ������� � ��. ���� ���������������, ���� ��������� �������� */
	U32 mT0H;
	U32 mT0L;

	U32 mT1H;
	U32 mT1L;

	U32 mTDelta;
	U32 mTMinimumReset;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		mShowLedNumberInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		mAutoBaudInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mT0HInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mT0LInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mT1HInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mT1LInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mTDeltaInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mTMinimumResetInterface;
};

#endif //WS2812B_ANALYZER_SETTINGS
