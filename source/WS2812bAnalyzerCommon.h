#ifndef WS2812B_ANALYZER_COMMON
#define WS2812B_ANALYZER_COMMON

/* Общие константы */

/* Частота сигнала WS2812b */
#define WS2812bFreq			(800000UL)
/* Размер (кол-во бит) данных для одного LED */
#define WS2812bBitsCount	(24)

#define MICRO_SEC	(1e-6)
#define NANO_SEC	(1e-9)

#define TIME_50us	(MICRO_SEC*50UL)
#define TIME_800ns	(NANO_SEC*800UL)
#define TIME_450ns	(NANO_SEC*450UL)
#define TIME_850ns	(NANO_SEC*850UL)
#define TIME_400ns	(NANO_SEC*400UL)

/* Характеристики сигнала по-умолчанию */

/* T0 - тайминг "0" в нс */
#define T0Hns		(400)
#define T0Lns		(850)
/* T1 - тайминг "1" в нс */
#define T1Hns		(800)
#define T1Lns		(450)
/* Минимальная длительность сигнала сброса в нс. В ДШ эта величина равна 50us, однако в интернете была информация о том, что минимальное значение около 9us. Ссылки, увы, нет */
#define TRESETns	(9000)

#define T_MIN_RESETns	(T1Hns + T0Lns)

/* Отклонение длительности периодов в нс */
#define TDELTAns	(150)

/* Анализатор различает сигнал сброса и данных */
typedef enum WS2812DataType
{
	WS2812bData = 0,
	WS2812bReset
};

#endif
