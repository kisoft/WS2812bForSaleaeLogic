#ifndef WS2812B_SIMULATION_DATA_GENERATOR
#define WS2812B_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <AnalyzerHelpers.h>
#include <string>

class WS2812bAnalyzerSettings;

class WS2812bSimulationDataGenerator
{
public:
	WS2812bSimulationDataGenerator();
	~WS2812bSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, WS2812bAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	WS2812bAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	ClockGenerator mClockGenerator;

protected:
	void CreateWS2812bData();
	/* Указатель на выводимый массив данных для демонстрации */
	const U32 *mpWS2812bSimData;
	/* Индекс выводимого фрейма данных */
	U32 mWS2812bSimDataIndex;

	SimulationChannelDescriptor mWS2812bSimulationData;

};
#endif //WS2812B_SIMULATION_DATA_GENERATOR