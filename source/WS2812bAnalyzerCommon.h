#ifndef WS2812B_ANALYZER_COMMON
#define WS2812B_ANALYZER_COMMON

/* ����� ��������� */

/* ������� ������� WS2812b */
#define WS2812bFreq			(800000UL)
/* ������ (���-�� ���) ������ ��� ������ LED */
#define WS2812bBitsCount	(24)

#define MICRO_SEC	(1e-6)
#define NANO_SEC	(1e-9)

#define TIME_50us	(MICRO_SEC*50UL)
#define TIME_800ns	(NANO_SEC*800UL)
#define TIME_450ns	(NANO_SEC*450UL)
#define TIME_850ns	(NANO_SEC*850UL)
#define TIME_400ns	(NANO_SEC*400UL)

/* �������������� ������� ��-��������� */

/* T0 - ������� "0" � �� */
#define T0Hns		(400)
#define T0Lns		(850)
/* T1 - ������� "1" � �� */
#define T1Hns		(800)
#define T1Lns		(450)
/* ����������� ������������ ������� ������ � ��. � �� ��� �������� ����� 50us, ������ � ��������� ���� ���������� � ���, ��� ����������� �������� ����� 9us. ������, ���, ��� */
#define TRESETns	(9000)

#define T_MIN_RESETns	(T1Hns + T0Lns)

/* ���������� ������������ �������� � �� */
#define TDELTAns	(150)

/* ���������� ��������� ������ ������ � ������ */
typedef enum WS2812DataType
{
	WS2812bData = 0,
	WS2812bReset
};

#endif
