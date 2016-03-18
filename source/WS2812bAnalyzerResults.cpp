#include "WS2812bAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "WS2812bAnalyzer.h"
#include "WS2812bAnalyzerSettings.h"
#include "WS2812bAnalyzerCommon.h"
#include <iostream>
#include <fstream>
#include <sstream>

/* ���������, ��� � WS2812b ����� ����������� � ������� GRB, ������ ���� - 8 ���. ����� ����� ������ 24 ����. */

//static 
U32 WS2812bAnalyzerResults::getColorR(U64 data)
{
	return (data >> 8) & 0xFF;
}

//static 
U32 WS2812bAnalyzerResults::getColorG(U64 data)
{
	return (data >> 16) & 0xFF;
}

//static 
U32 WS2812bAnalyzerResults::getColorB(U64 data)
{
	return data & 0xFF;
}

WS2812bAnalyzerResults::WS2812bAnalyzerResults(WS2812bAnalyzer* analyzer, WS2812bAnalyzerSettings* settings)
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

WS2812bAnalyzerResults::~WS2812bAnalyzerResults()
{
}

/*
* ������ ����� ������������ ��� ����������� ������ ��� �������� �� �������
*/
void WS2812bAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame(frame_index);

	char number_str[128];

	std::stringstream ss;

	if (WS2812bReset == frame.mType)
	{
		/*
		* ������� ������ ������ ������������ ��� "Reset [50 us]"
		*/
		AnalyzerHelpers::GetNumberString((frame.mData1 / 1000), DisplayBase::Decimal, 64, number_str, 128);
		ss << "Reset [" << number_str << " us]";
		AddResultString(ss.str().c_str());
	}
	else
	{
		/*
		* ������ ����� ������������ � ���������� ������ ����� � ����������� �� ������ ������������ �������.
		* ����� ����������� ��� �������.
		*/

		/*
		* ������ 1:
		* <LED Number>: <������ � �������� �������>
		*/
		/* ���������� ����� LED, ���� ��� ��������� � ���������� ��������� */
		if (mSettings->mShowLedNumber)
		{
			ss << frame.mData2 << ": ";
		}
		AnalyzerHelpers::GetNumberString(frame.mData1, display_base, WS2812bBitsCount, number_str, 128);
		ss << number_str;
		AddResultString(ss.str().c_str());
		ss.str("");

		/*
		* ������ 2:
		* <LED Number>: G[<������ � �������� �������>] R[<������ � �������� �������>] B[<������ � �������� �������>]
		*/
		if (mSettings->mShowLedNumber)
		{
			ss << frame.mData2 << ": ";
		}
		AnalyzerHelpers::GetNumberString(getColorG(frame.mData1), display_base, 8, number_str, 128);
		ss << "G[" << number_str << "] ";
		AnalyzerHelpers::GetNumberString(getColorR(frame.mData1), display_base, 8, number_str, 128);
		ss << "R[" << number_str << "] ";
		AnalyzerHelpers::GetNumberString(getColorB(frame.mData1), display_base, 8, number_str, 128);
		ss << "B[" << number_str << "]";
		AddResultString(ss.str().c_str());
		ss.str("");

		/*
		* ������ 3:
		* <LED Number>: Green [<������ � �������� �������>] Red [<������ � �������� �������>] Blue [<������ � �������� �������>]
		*/
		if (mSettings->mShowLedNumber)
		{
			ss << frame.mData2 << ": ";
		}
		AnalyzerHelpers::GetNumberString(getColorG(frame.mData1), display_base, 8, number_str, 128);
		ss << "Green [" << number_str << "] ";
		AnalyzerHelpers::GetNumberString(getColorR(frame.mData1), display_base, 8, number_str, 128);
		ss << "Red [" << number_str << "] ";
		AnalyzerHelpers::GetNumberString(getColorB(frame.mData1), display_base, 8, number_str, 128);
		ss << "Blue [" << number_str << "]";
		AddResultString(ss.str().c_str());
	}
}

/*
* ������ ����� ������������ ��� �������� ������ � ����
*/
void WS2812bAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	/*
	* ������ ������ � ���� (������ ������� ���������� �� ������� �������):
	* ����� � ��������
	* ������� ������ (1) ��� ������ ������ (0)
	* ����� LED (������ � ���������� ����)
	* ������ LED (� �������� �������)
	* ������� ������������ ������ LED (� �������� �������)
	* ������� ������������ ������ LED (� �������� �������)
	* ����� ������������ ������ LED (� �������� �������)
	* "�������� ������" - ��� ������, ������� ����� � ���������� ����������� ���������
	*/

	file_stream << "Time [s],IsData,LED number,Value,G,R,B" << std::endl;

	U64 num_frames = GetNumFrames();
	for (U32 i = 0; i < num_frames; i++)
	{
		Frame frame = GetFrame(i);

		char time_str[128];
		AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

		char number_str[128];
		AnalyzerHelpers::GetNumberString(frame.mData1, display_base, WS2812bBitsCount, number_str, 128);
		char number_str_R[128];
		AnalyzerHelpers::GetNumberString(getColorR(frame.mData1), display_base, 8, number_str_R, 128);
		char number_str_G[128];
		AnalyzerHelpers::GetNumberString(getColorG(frame.mData1), display_base, 8, number_str_G, 128);
		char number_str_B[128];
		AnalyzerHelpers::GetNumberString(getColorB(frame.mData1), display_base, 8, number_str_B, 128);

		file_stream << time_str << "," << (frame.mType == WS2812bData ? 1 : 0) << "," << frame.mData2 << "," << number_str << "," << number_str_G << "," << number_str_R << "," << number_str_B << std::endl;

		if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true)
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

/*
* ������ ����� ��������� ����������, ��������� � ���� "Decoded protocols".
* �������� USE_SIMPLE_TABULAR_TEXT, ����� ��������� ���������� ���� �����, ��� �������� �� �����
*/
// #define USE_SIMPLE_TABULAR_TEXT 1
void WS2812bAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	ClearTabularText();

	char number_str[128];

#if USE_SIMPLE_TABULAR_TEXT
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, WS2812bBitsCount, number_str, 128 );
	AddTabularText(number_str);
#else
	std::stringstream ss;

	if (WS2812bReset == frame.mType)
	{
		AnalyzerHelpers::GetNumberString((frame.mData1 / 1000), DisplayBase::Decimal, 64, number_str, 128);
		ss << "Reset [" << number_str << " us]";
	}
	else
	{
		if (mSettings->mShowLedNumber)
		{
			ss << frame.mData2 << ": ";
		}
		AnalyzerHelpers::GetNumberString(getColorG(frame.mData1), display_base, 8, number_str, 128);
		ss << "G[" << number_str << "] ";
		AnalyzerHelpers::GetNumberString(getColorR(frame.mData1), display_base, 8, number_str, 128);
		ss << "R[" << number_str << "] ";
		AnalyzerHelpers::GetNumberString(getColorB(frame.mData1), display_base, 8, number_str, 128);
		ss << "B[" << number_str << "]";
	}
	AddTabularText(ss.str().c_str());
#endif
}

void WS2812bAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void WS2812bAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}