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
* ��������� �� �������� ��� ������
*/
bool compare_for_equ(const DBSCANItem &pItem1, const DBSCANItem &pItem2)
{
	return pItem1.value == pItem2.value;
}

/*
* ������ ��������� ������ ������. �� ������ ����������� ������, ��� ��� �������� (value) ���������.
* ��� ���� ������ ������� �������� ���������� ����� ��������.
* ��� ��������� �������� ��������� ������. (�����������)
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
* ������ ������� ������������ ������ (� ns).
* ���������� 0, ���� ������� ������.
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
* ������ ������� ������������ ������ (� ��������).
* ���������� 0, ���� ������� ������.
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
* ������ ������� ������������ ������ (� ��������).
* ���������� 0, ���� ������� ������.
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
* ���������� ������ ������� ������.
* ����� �������� �������� � ������� ������ WS2812b (��� ������� ������� ������ �������).
*/
void DBSCAN::Scan(MainDataList *ppDataList, bool pAnalyzeLowImpulse /*= false*/)
{
	/* ����� ���������� ����������� ������������ */
	mAverages.clear();
	mDataList.clear();
	mClusterNumber = 0;

	/* ������ ������ */
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
		/* ����������� ������� ������������. ���� ������� ������ �������� ��������, ��� �������� ��� ��� */
		bool lIsNoise = (ClusterSize(lNeighbor) < mMinClusterSize);
		/* ������ ������ �� �� ������� �����, ���� ���� ������ ������ �������� ������ ���������, ������ ��� �� ����� ���� ���� �� ��� ������� ������, �������� */
		if (lIsNoise && pAnalyzeLowImpulse)
		{
			/* ���� ������������ ������ ������������ ������� ������, �� ��� ��-���� ��� */
			lIsNoise = (!mpWS2812bTimeValues->compareTReset(lClusterAverage));
		}
		if (lIsNoise)
		{
			lData.noise = true;
		}
		else
		{
			/* ���� ��� �� ���, ������� �������, ������� ��� �������� �� ������� ������, ������� ������ � ���� ������� */
			ExpandCluster(&lData, &lNeighbor, mClusterNumber++);
		}
	}

	AnalyzeInfo(&mDataList);
}

/*
* ���������� ������ ��������� (ppNeighbor), � ������� ��������, ���������� �� pCurrentItem.value �� ����� ��� �� mEpsilon.
* ������������� ������ ������� �� mDataList (��� ������ �������� ������).
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
* �������� �������� �� ��������� ��������� �������� (ppCurrentItem).
* ������ �������� ���������� � ������� ppNeighbor. ����� �������� �������� ���������� pClusterNumber.
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
* ������ ������� ������� ������� �� �������� (��������� ������ ������, �.�. ������ ����� ����� ������ 3, �� ��������� 12948 ���������).
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
* ���������� �������� ��������, ������ �������� �� ��� ������ ��� ����������.
* ����� �������� ������ ������� �������� ������������� � ��������������� ���� ��� WS2812b.
* ��� ������� �������� ������, �����, ������, ���� ��� �������� (�������� ������������� �������� ������ ��� "0" � "1"), � ���
* ������� ������� ������, �����, ������ ������ ���� �� ����� 3 �������� (�������� ������������� ������� ������ ��� "0", "1" � "�����").
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
