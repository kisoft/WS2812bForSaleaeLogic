#ifndef WS2812B_ANALYZER_RESULTS
#define WS2812B_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class WS2812bAnalyzer;
class WS2812bAnalyzerSettings;

class WS2812bAnalyzerResults : public AnalyzerResults
{
public:
	WS2812bAnalyzerResults( WS2812bAnalyzer* analyzer, WS2812bAnalyzerSettings* settings );
	virtual ~WS2812bAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	/* Helpers. Выделение из фрейма цветовых составляющих */
	static U32 getColorR(U64 data);
	static U32 getColorG(U64 data);
	static U32 getColorB(U64 data);


protected:  //vars
	WS2812bAnalyzerSettings* mSettings;
	WS2812bAnalyzer* mAnalyzer;
};

#endif //WS2812B_ANALYZER_RESULTS
