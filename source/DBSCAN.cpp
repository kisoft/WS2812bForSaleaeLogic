#include "DBSCAN.h"
#include "WS2812bAnalyzerCommon.h"
#if defined(_DEBUG)
#include <iostream>
#include <sstream>
#include <iomanip>
#endif
#include <algorithm>

#include "WS2812bAnalyzer.h"

#if defined(_DEBUG)
extern void LogToFile(const char *pp_text, U64 p_value);
extern void CloseFile(void);

#define LOGTOFILE(t,v)	LogToFile(t,v)
#define CLOSEFILE()		CloseFile()
#else
#define LOGTOFILE(t,v)
#define CLOSEFILE()
#endif

/*
* Сравнение по значению для поиска
*/
bool compare_for_equ(const DBSCANItem &pItem1, const DBSCANItem &pItem2)
{
	return pItem1.value == pItem2.value;
}

/*
* Сжатие исходного списка данных. На выходе формируется список, где все значения (value) уникальны.
* При этом каждый элемент содержит количество таких значений.
* Это позволяет ускорить обработку данных. (Субъективно)
*/
void DBSCAN::SqueezeMainData(MainDataList *ppFullList)
{
	for (auto iter = ppFullList->cbegin(); iter != ppFullList->cend(); iter++)
	{
		const DBSCANItem &lData = *iter;
		auto lNextIter = iter;
		lNextIter++;
		auto lExistsIter = std::search(mDataList.begin(), mDataList.end(), iter, lNextIter, ::compare_for_equ);
		if (lExistsIter != mDataList.end())
		{
			lExistsIter->kol += lData.kol;
		}
		else
		{
			mDataList.push_back(lData);
		}
	}

	LOGTOFILE("mDataList.size()", mDataList.size());
	LOGTOFILE("ppFullList->size()", ppFullList->size());
}

/*
* Расчет средней длительности семпла (в ns).
* Возвращает 0, если кластер пустой.
*/
U64 DBSCAN::ClusterAverageNs(const NeighborVector &pNeighbor, U64 pClusterNumber /*= NO_CLUSTER */) const
{
	U64 lAverage = 0;
	U64 lCount = 0;
	for (auto iter = pNeighbor.cbegin(); iter != pNeighbor.cend(); iter++)
	{
		if (NO_CLUSTER == pClusterNumber || (*iter)->cluster_number == pClusterNumber)
		{
			lAverage += (*iter)->value;
			lCount++;
		}
	}
	if (lCount != 0)
	{
		return mpWS2812bTimeValues->convertSampleToNs(lAverage / lCount);
	}
	else
	{
		return 0;
	}
}

/*
* Расчет средней длительности семпла (в попугаях).
* Возвращает 0, если кластер пустой.
*/
U64 DBSCAN::ClusterAverage(const NeighborVector &pNeighbor, U64 pClusterNumber /*= NO_CLUSTER */) const
{
	U64 lAverage = 0;
	U64 lCount = 0;
	for (auto iter = pNeighbor.cbegin(); iter != pNeighbor.cend(); iter++)
	{
		if (NO_CLUSTER == pClusterNumber || (*iter)->cluster_number == pClusterNumber)
		{
			lAverage += (*iter)->value;
			lCount++;
		}
	}
	if (lCount != 0)
	{
		return lAverage / lCount;
	}
	else
	{
		return 0;
	}
}

/*
* Расчет средней длительности семпла (в попугаях).
* Возвращает 0, если кластер пустой.
*/
double DBSCAN::ClusterAverage(const MainDataList &pDataList, U64 pClusterNumber /*= NO_CLUSTER*/) const
{
	double lAverage = 0;
	U64 lCounter = 0;
	for (auto iter = pDataList.cbegin(); iter != pDataList.cend(); iter++)
	{
		const DBSCANItem &lData = *iter;
		if (NO_CLUSTER == pClusterNumber || lData.cluster_number == pClusterNumber)
		{
			lAverage += lData.value;
			lCounter++;
		}
	}
	if (lCounter != 0)
	{
		return lAverage / lCounter;
	}
	return 0;
}

/*
* Собственно анализ входных данных.
* Здесь алгоритм притянут к анализу именно WS2812b (для анализа низкого уровня сигнала).
*/
void DBSCAN::Scan(MainDataList *ppDataList, bool pAnalyzeLowImpulse /*= false*/)
{
	/* Сброс статистики предыдущего сканирования */
	mAverages.clear();
	mDataList.clear();
	mClusterNumber = 0;

	/* Сжатие данных */
	SqueezeMainData(ppDataList);

	for (auto iter = mDataList.begin(); iter != mDataList.end(); iter++)
	{
		DBSCANItem &lData = *iter;
		if (lData.visited)
		{
			continue;
		}
		lData.visited = true;
		NeighborVector lNeighbor;
		RegionQuery(lData, &lNeighbor);
		U64 lClusterAverage = T_MIN_RESETns - 1;
		if (pAnalyzeLowImpulse)
		{
			lClusterAverage = ClusterAverage(lNeighbor);
		}
		/* Определение шумовой составляющей. Если кластер меньше заданной величины, его помечаем как шум */
		bool lIsNoise = (ClusterSize(lNeighbor) < mMinClusterSize);
		/* Сигнал сброса мы не считаем шумом, даже если размер такого кластера меньше заданного, потому как он может быть один на всю выборку данных, например */
		if (lIsNoise && pAnalyzeLowImpulse)
		{
			/* Если длительность меньше разрешенного сигнала сброса, то это всё-таки шум */
			lIsNoise = (!mpWS2812bTimeValues->compareTReset(lClusterAverage));
		}
		if (lIsNoise)
		{
			lData.noise = true;
		}
		else
		{
			/* Если это не шум, создаем кластер, помечая все элементы во входных данных, которые входят в этот кластер */
			ExpandCluster(&lData, &lNeighbor, mClusterNumber++);
		}
	}

	AnalyzeInfo(&mDataList);
}

/*
* Возвращает вектор элементов (ppNeighbor), у которых значение, отличается от pCurrentItem.value не более чем на mEpsilon.
* Анализируемые данные берутся из mDataList (это сжатые исходные данные).
*/
void DBSCAN::RegionQuery(const DBSCANItem &pCurrentItem, NeighborVector *ppNeighbor)
{
	for (auto iter = mDataList.begin(); iter != mDataList.end(); iter++)
	{
		DBSCANItem &lData = *iter;
		if ((pCurrentItem.value > lData.value ? pCurrentItem.value - lData.value : lData.value - pCurrentItem.value) < mEpsilon)
		{
			ppNeighbor->push_back(&lData);
		}
	}
}

/*
* Создание кластера на основании исходного элемента (ppCurrentItem).
* Данные кластера собираются в векторе ppNeighbor. Номер кластера задается параметром pClusterNumber.
*/
void DBSCAN::ExpandCluster(DBSCANItem *ppCurrentItem, NeighborVector *ppNeighbor, U64 pClusterNumber)
{
	ppCurrentItem->cluster_number = pClusterNumber;
	U64 lEnd = ppNeighbor->size();
	for (U64 i = 0; i < lEnd; i++)
	{
		DBSCANItem *lpData = (*ppNeighbor)[i];
		if (!lpData->visited)
		{
			lpData->visited = true;

			NeighborVector lNeighbor;
			RegionQuery(*lpData, &lNeighbor);
			if (ClusterSize(lNeighbor) >= mMinClusterSize)
			{
				ppNeighbor->insert(ppNeighbor->end(), lNeighbor.begin(), lNeighbor.end());
				lEnd = ppNeighbor->size();
			}
		}
		lpData = (*ppNeighbor)[i];
		if (lpData->cluster_number == NO_CLUSTER)
		{
			lpData->cluster_number = pClusterNumber;
		}
	}
}

/*
* Расчет размера вектора соседей по кластеру (учитывает сжатые данные, т.е. вектор может иметь размер 3, но содержать 12948 элементов).
*/
U64 DBSCAN::ClusterSize(const NeighborVector &pNeighbor) const
{
	U64 lCounter = 0;
	for (auto iter = pNeighbor.cbegin(); iter != pNeighbor.cend(); iter++)
	{
		const DBSCANItem *lpData = *iter;
		lCounter += (*iter)->kol;
	}
	return lCounter;
}

/*
* Собственно кластеры получены, теперь выделяем из них нужную нам информацию.
* Здесь получаем вектор средних значений длительностей в отсортированном виде для WS2812b.
* Для сигнала верхнего уровня, здесь, обычно, буду два значения (значения длительностей высокого уровня для "0" и "1"), а для
* сигнала низкого уровня, здесь, обычно должно быть не менее 3 значений (значения длительностей низкого уровня для "0", "1" и "Сброс").
*/
void DBSCAN::AnalyzeInfo(MainDataList *ppDataList)
{
	for (U64 i = 0; i < mClusterNumber; i++)
	{
		double lAverage = ClusterAverage(*ppDataList, i);
		mAverages.push_back(lAverage);
	}
	std::sort(mAverages.begin(), mAverages.end());
}
