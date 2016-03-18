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
* Кластерный анализ, алгоритм DBSCAN, адаптированный к анализу протокола WS2812b
*/

#define NO_CLUSTER	(-1)

/* Один элемент для анализа */
typedef struct _DBSCANItem
{
	U64		value;				/* Собственно, значение. Для WS2812b это длительность (высокого или низкого уровня) */
	bool	visited;			/* Признак того, что элемент уже обработан */
	bool	noise;				/* Элемент не попадает в кластер, т.е. является шумом */
	U64		cluster_number;		/* Номер кластера, по-умолчанию = NO_CLUSTER */
	U64		kol;				/* Количество повторяющихся значений (заполняется при "сжатии" информации) */
} DBSCANItem;

/* Вектор соседних элементов */
typedef std::vector<DBSCANItem *> NeighborVector;
/* Основной список данных */
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
	/* Возвращает кол-во полученных кластеров */
	U64 ClustersCounter() const { return mClusterNumber; }

	void AnalyzeInfo(MainDataList *ppDataList);

	const std::vector<double> GetAverages() const
	{
		return mAverages;
	}

protected:
	/*
	* Возвращает вектор элементов, расположенных на расстоянии не более mEpsilon от заданного элемента
	* ppCurrentItem - заданный элемент
	* ppNeighbor - результат. Вектор элементов (включая заданный), расположенных на расстоянии не более mEpsilon
	*/
	void	RegionQuery(const DBSCANItem &pCurrentItem, NeighborVector *ppNeighbor);
	/*
	* Создает (расширяет) кластер
	* ppCurrentItem - заданный элемент
	* ppNeighbor - исходный вектор для расширения
	* pClusterNumber - номер создаваемого кластера
	*/
	void	ExpandCluster(DBSCANItem *ppCurrentItem, NeighborVector *ppNeighbor, U64 pClusterNumber);

	/* Сжатие входных данных, чтобы уменьшить количество обрабатываемой информации */
	void	SqueezeMainData(MainDataList *ppFullList);

	/* Размер вектора, используется для подсчета сжатых списков */
	U64		ClusterSize(const NeighborVector &pNeighbor) const;

	/* Возвращает среднее по соседям с указанным индексом в nsec */
	U64		ClusterAverageNs(const NeighborVector &p_neighbor, U64 p_cluster_index = NO_CLUSTER) const;
	/* Возвращает среднее по соседям с указанным индексом в попугаях */
	U64		ClusterAverage(const NeighborVector &pNeighbor, U64 pClusterNumber = NO_CLUSTER) const;
	/* Возвращает среднее по кластеру с указанным индексом в попугаях */
	double	ClusterAverage(const MainDataList &pDataList, U64 pClusterNumber = NO_CLUSTER) const;

private:
	U64				mEpsilon;
	U64				mMinClusterSize;
	U64				mClusterNumber;
	/* Сжатый список данных */
	MainDataList	mDataList;
	/* Массив средних значений, полученных после анализа */
	std::vector<double> mAverages;

	const WS2812bTimeValues	*mpWS2812bTimeValues;
};

#endif
