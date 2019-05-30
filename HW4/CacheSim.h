#ifndef _CACHE_SIM_H
#define _CACHE_SIM_H

#include <vector>
#include "Cache.h"

using namespace std;

#define NUM_CACHES 2

typedef vector<unsigned> cache_args;

//the class that interfaces with the main method of the program 
class CacheSim
{
	
public:
	/** CacheSim - contructor
	\param[in] mem_latency - the latency of full memory access: L1 miss followed by L2 miss and then finally hit in main memory
	\param[in] logBlockSize - log2 of the size of a single block inside a cache
	\param[in] args - vector of cache_args. 
	Each cache_args object holds the log2 of the cache size, log2 of the associativity of the cache and the latency of the cache (in this exact order)
	*/
	CacheSim(unsigned mem_latency, unsigned logBlockSize, vector<cache_args> args);
	
	/** ~CacheSim - destructor
	*/
	~CacheSim();

public:
	/** Access
	//Access the cache hierarcy
	//This method invokes Cache::Access for L1 and L2, figuring out whether the block addressed is inside the cache hierarchy somewhere,
	//updating the statistics of the hierarchy (accumulative access time and miss count for L1 and L2)
	\param[in] op - the operation required. Can be either 'r' for read or 'w' for write.
	\param[in] address - the address to access inside the caches
	*/
	void Access(char op, unsigned address);

	/** L1MissRate
	\return the miss rate of L1 cache
	*/
	double L1MissRate() const;
	
	/** L2MissRate
	\return the miss rate of L2 cache
	*/
	double L2MissRate() const;
	
	/** AvgAccTime
	\return the average access time of the hierarchy
	*/
	double AvgAccTime() const;
	
private:
	/** _CheckValidity
	\return true if all cache pointers are valid (not NULL), for debug purposes
	*/
	bool _CheckValidity() const;
	
	/** __Read
	//Perform read operation. 
	//Executes L1Cache::Read operation
	\paran[in] address - the address the simulator is trying to access and read from
	*/
	void __Read(unsigned address);

	/** __Write
	//Perform write operation.
	//Executes L1Cache::Write operation
	\paran[in] address - the address the simulator is trying to access and write to
	*/
	void __Write(unsigned address);

private:
	unsigned m_L1_miss_count;
	unsigned m_L1_access_count;
	unsigned m_L2_miss_count;
	unsigned m_L2_access_count;
	double m_accumulated_time;
	vector<unsigned> m_latencies;
	vector<Cache*> m_caches;
};


#endif //_CACHE_SIM_H
