#pragma once

#ifndef __DBSCAN_H
#define __DBSCAN_H

#include <Analyzer.h>

#include <list>
#include <vector>

#include <stdint.h>

#if !defined(ANALYZER)
typedef uint32_t U32;
typedef uint64_t U64;
typedef int32_t I32;
typedef int64_t I64;
#endif

/*
* ���������� ������, �������� DBSCAN, �������������� � ������� ��������� WS2812b
*/

#define NO_CLUSTER	(-1)

/* ���� ������� ��� ������� */
typedef struct _DBSCANItem
{
	U64		value;				/* ����������, ��������. ��� WS2812b ��� ������������ (�������� ��� ������� ������) */
	bool	visited;			/* ������� ����, ��� ������� ��� ��������� */
	bool	noise;				/* ������� �� �������� � �������, �.�. �������� ����� */
	U64		cluster_number;		/* ����� ��������, ��-��������� = NO_CLUSTER */
	U64		kol;				/* ���������� ������������� �������� (����������� ��� "������" ����������) */
} DBSCANItem;

/* ������ �������� ��������� */
typedef std::vector<DBSCANItem *> NeighborVector;
/* �������� ������ ������ */
typedef std::list<DBSCANItem> MainDataList;

class WS2812bTimeValues;
class DBSCAN
{
public:
	DBSCAN(U64 pEpsilon, U64 pMinClusterSize, const WS2812bTimeValues *ppWS2812bTimeValues)
		: mEpsilon(pEpsilon)
		, mMinClusterSize(pMinClusterSize)
		, mClusterNumber(0)
		, mpWS2812bTimeValues(ppWS2812bTimeValues)
	{}
	
	void SetParameters(U64 pEpsilon, U64 pMinClusterSize)
	{
		mEpsilon = pEpsilon;
		mMinClusterSize = pMinClusterSize;
	}
	void Scan(MainDataList *ppDataList, bool pAnalyzeLowImpulse = false);
	/* ���������� ���-�� ���������� ��������� */
	U64 ClustersCounter() const { return mClusterNumber; }

	void AnalyzeInfo(MainDataList *ppDataList);

	const std::vector<double> GetAverages() const
	{
		return mAverages;
	}

protected:
	/*
	* ���������� ������ ���������, ������������� �� ���������� �� ����� mEpsilon �� ��������� ��������
	* ppCurrentItem - �������� �������
	* ppNeighbor - ���������. ������ ��������� (������� ��������), ������������� �� ���������� �� ����� mEpsilon
	*/
	void	RegionQuery(const DBSCANItem &pCurrentItem, NeighborVector *ppNeighbor);
	/*
	* ������� (���������) �������
	* ppCurrentItem - �������� �������
	* ppNeighbor - �������� ������ ��� ����������
	* pClusterNumber - ����� ������������ ��������
	*/
	void	ExpandCluster(DBSCANItem *ppCurrentItem, NeighborVector *ppNeighbor, U64 pClusterNumber);

	/* ������ ������� ������, ����� ��������� ���������� �������������� ���������� */
	void	SqueezeMainData(MainDataList *ppFullList);

	/* ������ �������, ������������ ��� �������� ������ ������� */
	U64		ClusterSize(const NeighborVector &pNeighbor) const;

	/* ���������� ������� �� ������� � ��������� �������� � nsec */
	U64		ClusterAverageNs(const NeighborVector &p_neighbor, U64 p_cluster_index = NO_CLUSTER) const;
	/* ���������� ������� �� ������� � ��������� �������� � �������� */
	U64		ClusterAverage(const NeighborVector &pNeighbor, U64 pClusterNumber = NO_CLUSTER) const;
	/* ���������� ������� �� �������� � ��������� �������� � �������� */
	double	ClusterAverage(const MainDataList &pDataList, U64 pClusterNumber = NO_CLUSTER) const;

private:
	U64				mEpsilon;
	U64				mMinClusterSize;
	U64				mClusterNumber;
	/* ������ ������ ������ */
	MainDataList	mDataList;
	/* ������ ������� ��������, ���������� ����� ������� */
	std::vector<double> mAverages;

	const WS2812bTimeValues	*mpWS2812bTimeValues;
};

#endif
