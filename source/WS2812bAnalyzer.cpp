#include "WS2812bAnalyzer.h"
#include "WS2812bAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include "WS2812bAnalyzerCommon.h"

#if defined(_DEBUG)
#include <sstream>
#endif

/*
* Игнорировать AutoBaud, делать его всегда
*/
#if defined(_DEBUG)
#define USE_DEBUG_FILE 1
#endif

#if defined(USE_DEBUG_FILE)

#define FILENAME "D:\\ws2812b.log"

FILE *f = nullptr;

void LogToFile(const char *pp_text, U64 p_value)
{
	if (!f)
	{
		fopen_s(&f, FILENAME, "wt");
	}
	fprintf(f, "%s: %d\n", pp_text, p_value);
	fflush(f);
}

void CloseFile(void)
{
	if (f)
	{
		fclose(f);
		f = nullptr;
	}
}

#define LOGTOFILE(t,v)	LogToFile(t,v)
#define CLOSEFILE()		CloseFile()

#else
#define LOGTOFILE(t,v)
#define CLOSEFILE()
#endif

/*
* Функция сравнения по длительности для сортировки
*/
bool compare_for_sort_by_duration(const DBSCANItem &pData1, const DBSCANItem &pData2)
{
	U64 lValue1 = pData1.value;
	U64 lValue2 = pData2.value;
	return lValue1 < lValue2;
}

/*
* Сравнение для поиска
*/
bool compare_for_equ_by_duration(const DBSCANItem &pData1, const DBSCANItem &pData2)
{
	U64 lValue1 = pData1.value;
	U64 lValue2 = pData2.value;
	return lValue1 == lValue2;
}

/*
* Пересчет из наносекунд в семплы или наоборот
*/
#define RECALC_TIME(x,s)	((x)*(s)/1000000.0/1000.0)
#define RECALC_STONS(x,s)	((x)*1000000.0/(s)*1000.0)

/*
* Инициализация временнЫх коэффициентов, используемых анализатором
* pp_settings используется для чтения текущих временнЫх параметров
* sample_rate_hz - частота дискретизации анализатора
*/
void WS2812bTimeValues::Init(const WS2812bAnalyzerSettings *ppSettings, U64 pSampleRateHz)
{
	mSampleRateHz = pSampleRateHz;
	mT0H = RECALC_TIME(ppSettings->mT0H, pSampleRateHz);
	mT0L = RECALC_TIME(ppSettings->mT0L, pSampleRateHz);
	mT1H = RECALC_TIME(ppSettings->mT1H, pSampleRateHz);
	mT1L = RECALC_TIME(ppSettings->mT1L, pSampleRateHz);
	mTDelta = RECALC_TIME(ppSettings->mTDelta, pSampleRateHz);
	mTReset = RECALC_TIME(ppSettings->mTMinimumReset, pSampleRateHz);
}

U64 WS2812bTimeValues::convertSampleToNs(U64 pSample) const
{
	return (U64)RECALC_STONS(pSample, mSampleRateHz);
}

U64 WS2812bTimeValues::convertNsToSample(U64 time) const
{
	return (U64)RECALC_TIME(time, mSampleRateHz);
}

bool WS2812bTimeValues::compareT0H(U64 value) const
{
	return (value > (mT0H - mTDelta) && value < (mT0H + mTDelta));
}

bool WS2812bTimeValues::compareT1H(U64 value) const
{
	return (value >(mT1H - mTDelta) && value < (mT1H + mTDelta));
}

bool WS2812bTimeValues::compareTReset(U64 value) const
{
	return (value >= mTReset);
}

#if defined(_DEBUG)
void WS2812bTimeValues::PrintInfo(FILE *f) const
{
	LOGTOFILE("T0H", (U64)mT0H);
	LOGTOFILE("T0L", (U64)mT0L);
	LOGTOFILE("T1H", (U64)mT1H);
	LOGTOFILE("T1L", (U64)mT1L);
	LOGTOFILE("TDelta", (U64)mTDelta);
	LOGTOFILE("TReset", (U64)mTReset);
	LOGTOFILE("SampleRateHz", mSampleRateHz);
}

/*
* Сохраняет список в бинарном виде (сохраняется только значение value)
* Первые U64 содержат количество записей.
*/
void saveToFile(const MainDataList &pList, const char *pFileName)
{
	std::string lFileName = "D:/";
	lFileName += pFileName;

	FILE *fout = nullptr;
	fopen_s(&fout, lFileName.c_str(), "wb");
	if (fout)
	{
		U64 lCounter = pList.size();
		fwrite(&lCounter, sizeof(lCounter), 1, fout);
		for (auto iter = pList.cbegin(); iter != pList.cend(); iter++)
		{
			const DBSCANItem &lData = *iter;
			fwrite(&lData.value, sizeof(lData.value), 1, fout);
		}
		fclose(fout);
	}
}

#endif

/*
* Сканирование данных, кластерный анализ
* Для низкого уровня, размеры кластера не ограничены минимальным значением,
* эти значения ограничены только минимальным значением 9000 нс (TRESETns), всё, что меньше - мусор
*/
bool WS2812bDBSCAN::scan(U64 pEpsilon, U64 pMinClusterSize)
{
	bool lResult = false;
	LOGTOFILE("Start SCAN", 0);
	U64 lAllCnt = mHighLevelData.size() + mLowLevelData.size();

	/*
	* ВНИМАНИЕ! Данные нужно собрать при анализе, иначе автоматическое определение здесь работать не будет!
	* Здесь производится анализ того, что уже собрано
	*/

	if (0 == lAllCnt)
	{
		return false;
	}

#if 0
	::saveToFile(mHighLevelData, "HighLevelData.dat");
	::saveToFile(mLowLevelData, "LowLevelData.dat");
	return false;
#endif

	/* Устанавливаем отклонение и минимальный размер кластера */
	mDBSCAN->SetParameters(pEpsilon, pMinClusterSize);

	/* Обработка длительностей высокого уровня сигнала */
	mDBSCAN->Scan(&mHighLevelData);
	std::vector<double> lHighAverages = mDBSCAN->GetAverages();

	/* Обработка длительностей низкого уровня сигнала */
	mDBSCAN->Scan(&mLowLevelData, true);
	std::vector<double> lLowAverages = mDBSCAN->GetAverages();

	int lLevelsCounter = 0;
	/* Количество уровней высокого уровня должно быть 2, а низкого не менее 2, иначе игнорируем результаты расчетов */
	if ((2 == lHighAverages.size()) && (lLowAverages.size() >= 2))
	{
		U64 lValue1 = 0;
		U64 lValue2 = 0;
		auto iter = lHighAverages.cbegin();
		lValue1 = U64(*iter);
		iter++;
		lValue2 = U64(*iter);

		LOGTOFILE("===== value1 =====", lValue1);
		LOGTOFILE("===== value2 =====", lValue2);
		if (lValue1 < lValue2)
		{
			mT0H = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue1);
			mT1H = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue2);
		}
		else
		{
			mT0H = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue2);
			mT1H = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue1);
		}
		iter = lLowAverages.cbegin();
		lValue1 = U64(*iter);
		iter++;
		lValue2 = U64(*iter);
		LOGTOFILE("===== value1 =====", lValue1);
		LOGTOFILE("===== value2 =====", lValue2);
		if (lValue1 < lValue2)
		{
			mT1L = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue1);
			mT0L = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue2);
		}
		else
		{
			mT1L = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue2);
			mT0L = (U32)mpWS2812bTimeValues->convertSampleToNs(lValue1);
		}
		LOGTOFILE("===== T0H =====", mT0H);
		LOGTOFILE("===== T0L =====", mT0L);
		LOGTOFILE("===== T1H =====", mT1H);
		LOGTOFILE("===== T1L =====", mT1L);
		lLevelsCounter += 2;
	}

	/* Только если оба уровня есть, позволяем перезагрузку сигнала */
	if (2 == lLevelsCounter)
	{
		bool lChanged = false;
		if (mpSettings->mT0H != mT0H)
		{
			mpSettings->mT0H = mT0H;
			lChanged = true;
		}
		if (mpSettings->mT1H != mT1H)
		{
			mpSettings->mT1H = mT1H;
			lChanged = true;
		}
		if (mpSettings->mT0L != mT0L)
		{
			mpSettings->mT0L = mT0L;
			lChanged = true;
		}
		if (mpSettings->mT1L != mT1L)
		{
			mpSettings->mT1L = mT1L;
			lChanged = true;
		}
		if (lChanged)
		{
			mpSettings->UpdateInterfacesFromSettings();
			LOGTOFILE("===== Data changed, rerun! =====", 1);
			lResult = true;
		}
	}

	return lResult;
}

WS2812bAnalyzer::WS2812bAnalyzer()
	: Analyzer2()
	, mSettings(new WS2812bAnalyzerSettings())
	, mSimulationInitilized(false)
	, mWS2812bTimeValues(new WS2812bTimeValues())
{
	SetAnalyzerSettings(mSettings.get());
}

WS2812bAnalyzer::~WS2812bAnalyzer()
{
	KillThread();
}

/*
* Рабочая нить. Выполняется бесконечно.
*/
void WS2812bAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	mWS2812bTimeValues->Init(mSettings.get(), mSampleRateHz);
#if defined(USE_DEBUG_FILE)
	mWS2812bTimeValues->PrintInfo(f);
#endif

	mWS2812bData = GetAnalyzerChannelData(mSettings->mInputChannel);

	mDBSCAN.reset(new WS2812bDBSCAN(mSettings.get(), mWS2812bData, mWS2812bTimeValues.get()));
	U64 lStartLocation = mWS2812bData->GetSampleNumber();

	/*
	* Начало распознавания сигнала "сброс" нужно обязательно, иначе будет некорректно распознана информация.
	* Для синхронизации необходимо найти первый сигнал сброса.
	*/
	while (mWS2812bData->DoMoreTransitionsExistInCurrentData())
	{
		U64 lLastLocation = mWS2812bData->GetSampleNumber();
		if (mWS2812bData->GetBitState() == BIT_LOW)
		{
			U64 lNextLocation = mWS2812bData->GetSampleOfNextEdge();
			if (mWS2812bTimeValues->compareTReset(lNextLocation - lLastLocation))
			{
				/* Обнаружен сигнал сброса */
				break;
			}
		}
		mWS2812bData->AdvanceToNextEdge();
	}

	U64 lLedNumber = 0;
	U32 lBitsCount = 0;
	U64 lStartingSample = 0;
	U32 lData = 0;
	for (;;)
	{
		/* Сохраняем текущее время */
		U64 lEdgeLocation = mWS2812bData->GetSampleNumber();
		/* Если уровень высокий, значит начинаются данные */
		if (mWS2812bData->GetBitState() == BIT_HIGH)
		{
			U32 lLastBit = 0;
			/* Переходим к низкому уровню */
			mWS2812bData->AdvanceToNextEdge();
			/* Определяем длительность высокого уровня */
			U64 lEndEdgeLocation = mWS2812bData->GetSampleNumber();
			U64 lEdgeDuration = lEndEdgeLocation - lEdgeLocation;

			if (mSettings->mAutoBaud)
			{
				mDBSCAN->addHighLevelData({ lEdgeDuration, false, false, NO_CLUSTER, 1 });
			}
			/* Проверка на 0 */
			if (mWS2812bTimeValues->compareT0H(lEdgeDuration))
			{
				lLastBit = 0;
				lBitsCount++;
			}
			/* Проверка на 1 */
			else if (mWS2812bTimeValues->compareT1H(lEdgeDuration))
			{
				lLastBit = 1;
				lBitsCount++;
			}
			/* Иначе это ошибка */
			else
			{

				lLedNumber = 0;
				lBitsCount = 0;
				lData = 0;
				continue;
			}
			/* Сдвигаем данные и готовим место под текущий бит */
			lData <<= 1;
			/* Добавляем текущий бит */
			lData |= lLastBit;
			/* Если это первый бит, сохраняем начальное время */
			if (1 == lBitsCount)
			{
				lStartingSample = lEdgeLocation;
			}
			/* Если обработаны все 24 бита, нужно сохранить фрейм */
			if (lBitsCount >= WS2812bBitsCount)
			{
				/*
				* Фрейм данных содержит следующую информацию:
				* mData1 - собственно распознанные данные
				* mData2 - номер LED
				* mType - тип данных (здесь это данные)
				* mStartingSampleInclusive - начальный семпл фрейма
				* mEndingSampleInclusive - конечный сэмпл фрейма
				*/
				Frame lFrame;
				lFrame.mData1 = lData;
				lFrame.mData2 = lLedNumber;
				lFrame.mType = WS2812bData;
				lFrame.mFlags = 0;
				lFrame.mStartingSampleInclusive = lStartingSample;

				U64 lStartNextEdgeLocation = mWS2812bData->GetSampleOfNextEdge();
				bool lNextReset = mWS2812bTimeValues->compareTReset(lStartNextEdgeLocation - lEndEdgeLocation);
				if (lNextReset)
				{
					/* Низкий уровень является паузой */
					lFrame.mEndingSampleInclusive = lEndEdgeLocation;
				}
				else
				{
					lFrame.mEndingSampleInclusive = lStartNextEdgeLocation;
				}

				mResults->AddFrame(lFrame);
				/* Добавляем маркер начала (зеленая точка) */
				mResults->AddMarker(lFrame.mStartingSampleInclusive, AnalyzerResults::MarkerType::Start, mSettings->mInputChannel);
				if (lNextReset)
				{
					/* Добавляем маркер конца (оранжевая точка) */
					mResults->AddMarker(lFrame.mEndingSampleInclusive, AnalyzerResults::MarkerType::Stop, mSettings->mInputChannel);
				}
				mResults->CommitResults();
				ReportProgress(lFrame.mEndingSampleInclusive);

				/* Переходим к следующему семплу */
				lLedNumber++;
				lBitsCount = 0;
				lData = 0;
			}
		}
		/* Низкий уровень. Это может быть окончание бита, либо Reset */
		else
		{
			/* Переходим к высокому уровню */
			mWS2812bData->AdvanceToNextEdge();
			/* Определяем длительность низкого уровня */
			U64 lEdgeDuration = mWS2812bData->GetSampleNumber() - lEdgeLocation;
			if (mSettings->mAutoBaud)
			{
				/* Внимание! Здесь нет проверки на корректность длительности низкого уровня! Данные распознаются по длительности высокого уровня */
				mDBSCAN->addLowLevelData({ lEdgeDuration, false, false, NO_CLUSTER, 1 });
			}
			/* Если это Reset */
			if (mWS2812bTimeValues->compareTReset(lEdgeDuration))
			{
				/*
				* Фрейм сигнала сброса содержит следующую информацию:
				* mData1 - длительность сигнала сброса в нс
				* mData2 - 0
				* mType - тип данных (здесь это сброс)
				* mStartingSampleInclusive - начальный семпл сигнала сброса
				* mEndingSampleInclusive - конечный сэмпл сигнала сброса
				*/
				lLedNumber = 0;
				Frame lFrame;
				lFrame.mData1 = mWS2812bTimeValues->convertSampleToNs(lEdgeDuration);
				lFrame.mData2 = lLedNumber;
				lFrame.mType = WS2812bReset;
				lFrame.mFlags = 0;
				lFrame.mStartingSampleInclusive = lEdgeLocation;
				lFrame.mEndingSampleInclusive = mWS2812bData->GetSampleNumber();

				mResults->AddFrame(lFrame);
				mResults->CommitResults();
				ReportProgress(lFrame.mEndingSampleInclusive);
				lBitsCount = 0;
				lData = 0;
			}
			else
			{
				/* Это не Reset, это окончание бита. Ничего не делаем. */
			}
		}
	}
	CLOSEFILE();
}

/*
* Здесь можно вызвать распознавалку частоты входного сигнала. Собственно здесь и выполняется
* алгоритм DBSCAN и производится распознавание длительностей сигнала. Если исходные значения и
* полученные здесь различны, то выполняется обновления параметров установок ианализатор "перезапускается".
*/
bool WS2812bAnalyzer::NeedsRerun()
{
	/* Если выключено автоопределение времянок, ничего не делаем */
	if (!mSettings->mAutoBaud)
	{
		return false;
	}

	U64 lDelta0 = T0Lns - T0Hns;
	U64 lDelta1 = T1Hns - T1Lns;
	/* Отклонение считаем как минимальную разницу между длительностями уровней, деленная пополам. Почти от фонаря. */
	U64 lEpsilon = mWS2812bTimeValues->convertNsToSample((lDelta0 < lDelta1 ? lDelta0 : lDelta1) / 2);
	/* Минимальный размер кластера устанавливаем 3, чтобы отрезать шумы. Значение почти от фонаря. */
	return mDBSCAN->scan(lEpsilon, 3);
}

U32 WS2812bAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (mSimulationInitilized == false)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

/* Опытным путем выяснено, что минимальная частота для распознавания равна 12,5МГц, меньше - уже длительность в попугаях очень малая величина. */
U32 WS2812bAnalyzer::GetMinimumSampleRateHz()
{
	return 12500000;
}

const char* WS2812bAnalyzer::GetAnalyzerName() const
{
	return "WS2812b";
}

/* Новая "фишка" для Analyzer2 */
void WS2812bAnalyzer::SetupResults()
{
	mResults.reset(new WS2812bAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());
	mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

const char* GetAnalyzerName()
{
	return "WS2812b";
}

Analyzer* CreateAnalyzer()
{
	return new WS2812bAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
