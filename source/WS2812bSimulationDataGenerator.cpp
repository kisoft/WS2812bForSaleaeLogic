#include "WS2812bSimulationDataGenerator.h"
#include "WS2812bAnalyzerSettings.h"
#include "WS2812bAnalyzerCommon.h"

#include <AnalyzerHelpers.h>

/* Напоминаю, что один фрейм данных протокола WS2812b содержит цвета в последовательности GRB */
const U32 SimData[] =
{
	0x0000FF00UL,		// R=255
	0x00FF0000UL,		// G=255
	0x000000FFUL,		// B=255
	0x00112233UL		// G=0x11, R=0x22, B=0x33
};

const U32 SimDataLen = sizeof(SimData) / sizeof(SimData[0]);

WS2812bSimulationDataGenerator::WS2812bSimulationDataGenerator()
	: mpWS2812bSimData(SimData)
	, mWS2812bSimDataIndex(0)
{
}

WS2812bSimulationDataGenerator::~WS2812bSimulationDataGenerator()
{
}

void WS2812bSimulationDataGenerator::Initialize( U32 simulation_sample_rate, WS2812bAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;
	/* Генератор будет использоваться для формирования временных задержек сигнала */
	mClockGenerator.Init(WS2812bFreq, simulation_sample_rate);

	mWS2812bSimulationData.SetChannel(mSettings->mInputChannel);
	mWS2812bSimulationData.SetSampleRate(simulation_sample_rate);
	mWS2812bSimulationData.SetInitialBitState(BIT_LOW);
}

/*
* Метод, используемый для генерации тестового сигнала
*/
U32 WS2812bSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	/* Сначала формируем короткий сигнал высокого уровня размером в половину T0H */
	mWS2812bSimulationData.TransitionIfNeeded(BIT_HIGH);
	mWS2812bSimulationData.Advance(mClockGenerator.AdvanceByTimeS((NANO_SEC * mSettings->mT0H) / 2.0));
	double lMinimumReset = NANO_SEC * mSettings->mTMinimumReset;
	double resetTime = (lMinimumReset < TIME_50us ? TIME_50us : lMinimumReset) + (NANO_SEC * mSettings->mT1L);
	while (mWS2812bSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		if (0 == mWS2812bSimDataIndex)
		{
			/* Формируем сигнал сброса не менее 50us в начале посылки */
			mWS2812bSimulationData.TransitionIfNeeded(BIT_LOW);
			/* Выдерживаем паузу для формирования сигнала сброса */
			mWS2812bSimulationData.Advance(mClockGenerator.AdvanceByTimeS(resetTime));

		}
		/* Выводим тестовые данные (здесь выводится только один фрейм) */
		CreateWS2812bData();
	}

	*simulation_channel = &mWS2812bSimulationData;
	return 1;
}

/*
* Метод выводит один тестовый фрейм. Сигнал сброса (если нужен), формируется уровнем выше.
*/
void WS2812bSimulationDataGenerator::CreateWS2812bData()
{
	U32 data = mpWS2812bSimData[mWS2812bSimDataIndex];
	mWS2812bSimDataIndex++;
	if (mWS2812bSimDataIndex == SimDataLen)
	{
		/* Массив тестовых данных закончился, сбрасываем индекс. Сброс индекса будет сопровождаться формированием сигнала сброса перед следующей серией данных */
		mWS2812bSimDataIndex = 0;
	}

	U32 mask = 0x1UL << (WS2812bBitsCount - 1);
	for (U32 i = 0; i < WS2812bBitsCount; i++)
	{
		/* "1" */
		if ((data & mask) != 0)
		{
			mWS2812bSimulationData.Transition();
			/* T1H */
			mWS2812bSimulationData.Advance(mClockGenerator.AdvanceByTimeS(NANO_SEC * mSettings->mT1H));		// 800 ns
			mWS2812bSimulationData.Transition();
			/* T1L */
			mWS2812bSimulationData.Advance(mClockGenerator.AdvanceByTimeS(NANO_SEC * mSettings->mT1L));		// 450 ns
		}
		/* "0" */
		else
		{
			mWS2812bSimulationData.Transition();
			/* T0H */
			mWS2812bSimulationData.Advance(mClockGenerator.AdvanceByTimeS(NANO_SEC * mSettings->mT0H));		// 400 ns
			mWS2812bSimulationData.Transition();
			/* T0L */
			mWS2812bSimulationData.Advance(mClockGenerator.AdvanceByTimeS(NANO_SEC * mSettings->mT0L));		// 850 ns
		}
		mask = mask >> 1;
	}
}
