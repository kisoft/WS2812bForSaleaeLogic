#ifndef WS2812B_ANALYZER_H
#define WS2812B_ANALYZER_H

#include <Analyzer.h>
#include "WS2812bAnalyzerResults.h"
#include "WS2812bSimulationDataGenerator.h"
#include "WS2812bAnalyzerSettings.h"
#include "DBSCAN.h"

/*
* ���������� ����������� ��������� WS2812b ��� Saleae LLC. �����: kisoft (2016)
* Version: 1.0
* Version Saleae SDK: 1.1.32
* ������������ ������������� �� Saleae Logic v1.2.5 Beta
*/

#pragma warning(disable: 4251)

/*
* Helper ��� ������ � ���������� ����������� ��� ������� �������.
* ��������� ������� �������������. ����������� �� �������� � �� � �������.
*/
class WS2812bTimeValues
{
public:
	WS2812bTimeValues() {}
	void Init(const WS2812bAnalyzerSettings *ppSettings, U64 pSampleRateHz);

	/* ���������� true, ���� pValue ��������� � �������� T0H +- TDelta */
	bool compareT0H(U64 pValue) const;
	/* ���������� true, ���� pValue ��������� � �������� T1H +- TDelta */
	bool compareT1H(U64 pValue) const;
	/* ���������� true, ���� pValue ������ ��� ����� TReset */
	bool compareTReset(U64 pValue) const;
	/* ����������� �� �������� � �� */
	U64 convertSampleToNs(U64 pSample) const;
	/* ����������� �� �� � ������� */
	U64 convertNsToSample(U64 pTime) const;

#if defined(_DEBUG)
	void PrintInfo(FILE *f) const;
#endif

private:
	double	mT0H;
	double	mT0L;
	double	mT1H;
	double	mT1L;
	double	mTReset;
	double	mTDelta;
	U64		mSampleRateHz;
	const WS2812bAnalyzerSettings *mpSettings;
};

/*
* ���������� ��������� ������ ��������� DBSCAN � ������� ��������� WS2812b.
* ���������� ���������� ���������, ������������� � ������ DBSCAN.
*/
class WS2812bDBSCAN
{
public:
	WS2812bDBSCAN(WS2812bAnalyzerSettings *ppSettings, AnalyzerChannelData *ppWS2812bData, WS2812bTimeValues *ppWS2812bTimeValues)
		: mpSettings(ppSettings)
		, mpWS2812bData(ppWS2812bData)
		, mpWS2812bTimeValues(ppWS2812bTimeValues)
		, mDBSCAN(new DBSCAN(3, 10, ppWS2812bTimeValues))
	{}
	/* ������������ ������, ���������� ������. ���������� true, ���� ������ ���������� */
	bool scan(U64 pEpsilon, U64 pMinClusterSize);

	/* ���������� ������ � ������ ������ �������� ������ */
	void addHighLevelData(const DBSCANItem &pData)
	{
		mHighLevelData.push_back(pData);
	}
	/* ���������� ������ � ������ ������ ������� ������ */
	void addLowLevelData(const DBSCANItem &pData)
	{
		mLowLevelData.push_back(pData);
	}

private:
	AnalyzerChannelData *mpWS2812bData;
	WS2812bTimeValues *mpWS2812bTimeValues;
	WS2812bAnalyzerSettings *mpSettings;
	/* ����� ������ ������� ������ */
	std::list<DBSCANItem> mHighLevelData;
	/* ����� ������ ������ ������ */
	std::list<DBSCANItem> mLowLevelData;
	/* ���������� ��������� DBSCAN */
	std::auto_ptr<DBSCAN>	mDBSCAN;

	U32	mT0H;
	U32	mT0L;
	U32	mT1H;
	U32	mT1L;
	U32	mTDelta;
	U32	mTMinimumReset;
};

class WS2812bAnalyzerSettings;
class ANALYZER_EXPORT WS2812bAnalyzer : public Analyzer2
{
public:
	WS2812bAnalyzer();
	virtual ~WS2812bAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

	// for Analyzer2 only
	virtual void SetupResults();

protected: //vars
	std::auto_ptr< WS2812bAnalyzerSettings > mSettings;
	std::auto_ptr< WS2812bAnalyzerResults > mResults;
	AnalyzerChannelData* mWS2812bData;

	WS2812bSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//WS2812b analysis vars:
	std::auto_ptr< WS2812bTimeValues > mWS2812bTimeValues;
	std::auto_ptr< WS2812bDBSCAN > mDBSCAN;
	U32 mSampleRateHz;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //WS2812B_ANALYZER_H
