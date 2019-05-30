#ifndef _CACHE_LINE_H
#define _CACHE_LINE_H
#include <deque>
#include <cstddef>

#include "CacheBlock.h"

using namespace std;

//here we implement the LRU algorithm
class CacheLine
{
	friend class L1Cache;
	typedef unsigned index;
	typedef unsigned tag;
public:
	/** CacheLine: constructor
	\param[in] num_ways - number of ways inside the cache line (default is 1)
	*/
	CacheLine(unsigned num_ways = 1);
	
	/** ~CacheLine: destructor
	*/
	~CacheLine();

public:
	/** Read
	//Read the data inside a block in the cache line
	\param[in] tag - the tag of the block to be looked up inside the line ways
	\param[inout] is_thrown - a flag that indicates if a block has been thrown from the cache line
	\return The thrown cache block from the cache line (block will be invalid if no block has been thrown and is_thrown will be false)
	*/
	CacheBlock Read(unsigned tag, bool* is_thrown = NULL);

	/** Write
	//Write to a cache block
	\param[in] tag - the tag of the block to be looked up inside the line ways
	\param[inout] is_thrown - a flag that indicates if a block has been thrown from the cache line
	\return The thrown cache block from the cache line (block will be invalid if no block has been thrown and is_thrown will be false)
	*/
	CacheBlock Write(unsigned tag, bool *is_thrown = NULL);

	/** Access
	//Find a block inside the cache line according to a given tag
	\param[in] tag - the tag of the block to be looked up inside the line ways
	\return true if the block has been found, false else
	*/
	bool Access (unsigned tag) const;

private:
	/** __Remove
	//Find a block inside the cache line according to a given tag and remove it
	\param[in] tag - the tag of the block to be looked up inside the line ways and removed
	*/
	void __Remove(unsigned tag);
private:
	unsigned m_num_ways;
	
	//each index inside the buffer represents a single way
	//each cell represents a tag
	deque<CacheBlock> m_ways;
};

#endif // _CACHE_LINE_h