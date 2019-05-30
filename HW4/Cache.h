#ifndef _CACHE_H
#define _CACHE_H
#include <vector>
#include "CacheLine.h"

#define ADDRESS_WIDTH 32

class Cache
{
public:
	/** Cache: constructor
	\param[in] cSizeLog - log2 of the size of the cache
	\param[in] bSizeLog - log2 of the size of a single block inside a cache
	\param[in] cNumWaysLog - log2 of the size of the number of ways inside a single cache line
	*/
	Cache(unsigned cSizeLog, unsigned bSizeLog, unsigned cNumWaysLog);

	/** ~Cache: destructor (virtual)
	\cleans the lines of the cache from the memory
	*/
	virtual ~Cache();

public:
	/** Read (virtual)
	\Reads from a memory address and brings the wordline to the cache (a whole block)
	\If the address already inside the cache we have a cache hit, so advance the address tag to be the MRU
	\Else we have a cache miss so we have to bring the block to the cache (and save it as MRU)
	\param[in] address - the address to be looked up in the cache
	*/
	virtual void Read(unsigned address);

	/** Write (virtual)
	\Writes to a cache block
	\If the address already inside the cache we have a cache hit, change the 'dirty' bit and replace the addressed block as the MRU
	\Else we have a cache miss so we have to bring the block to the cache, set its 'dirty' bit and set the block as the new MRU
	\param[in] address - the address to be looked up in the cache
	*/
	virtual void Write(unsigned address);

	/** Access (virtual)
	\Searches the cache lines for a block with tag of the address
	\param[in] address - the address to be looked up in the cache
	\return:
		regular cache will return 0 or 1 (hit or miss)
		L1 type cache will return:
		0 for hit
		1 for L1 miss and L2 hit
		2 for L1 miss and L2 miss
	*/
	virtual unsigned Access(unsigned address) const;


protected:
	//configuration arguments
	unsigned m_tag_mask;
	unsigned m_tag_width;
	unsigned m_set_mask;
	unsigned m_set_width;
	unsigned m_offset_mask;
	unsigned m_offset_width;

	//this is the actual memory
	std::vector<CacheLine*> m_lines;
};

class L1Cache : public Cache
{
	friend class L2Cache;
	typedef enum CACHE_MISS { L1_HIT = 0, L2_HIT = 1, L2_MISS = 2 } CACHE_MISS;

public:
	/** L1Cache: constructor
	\param[in] cSizeLog - log2 of the size of the cache
	\param[in] bSizeLog - log2 of the size of a single block inside a cache
	\param[in] cNumWaysLog - log2 of the size of the number of ways inside a single cache line
	\param[in] second_level_cache - a pointer to L2 cache
	*/
	L1Cache(unsigned cSizeLog, unsigned bSizeLog, unsigned cNumWaysLog, Cache* second_level_cache);
	
	/** ~L1Cache: destructor (virtual)
	*/
	virtual ~L1Cache();

	/** Read
	If the block isn't inside L2, add it to L2 (maintaining inculsivity)
	Try to read from the lines of L1. 
	If there's a thrown block from L1 and it's dirty, write it to L2
	\param[in] address - an address to be looked up in the cache and read from
	*/
	virtual void Read(unsigned address);

	/** Write
	If the block isn't inside L2, add it to L2 (maintaining inculsivity)
	Try to write to the lines of L1.
	If there's a thrown block from L1 and it's dirty, write it to L2
	\param[in] address - an address to be looked up in the cache and written to
	*/
	virtual void Write(unsigned address);

	/** Access
	Access L1 with an address and return the type of miss (or hit)
	\param[in] address - an address to be looked up in the cache
	\return:
	0: L1_HIT
	1: L2_HIT - L1 miss and L2 hit
	2: L2_MISS - L1 miss and L2 miss
	*/
	virtual unsigned Access(unsigned address) const;

private:
	/** __Remove
	Access L1 and remove a block according to an address supplied
	\param[in] address - an address to be looked up in the cache and removed
	*/
	void __Remove(unsigned address);

private:
	Cache* m_L2_cache;
};


class L2Cache : public Cache
{
public:
	/** L2Cache: constructor
	\param[in] cSizeLog - log2 of the size of the cache
	\param[in] bSizeLog - log2 of the size of a single block inside a cache
	\param[in] cNumWaysLog - log2 of the size of the number of ways inside a single cache line
	\param[in] first_level_cache - a pointer to L1 cache
	*/
	L2Cache(unsigned cSizeLog, unsigned bSizeLog, unsigned cNumWaysLog, L1Cache* first_level_cache);

	/** ~L2Cache: destructor (virtual)
	*/
	virtual ~L2Cache();

	/** Read
	Try to read from the lines of the cache.
	Try to read from the lines of L1.
	If there's a thrown block from L2, remove it also from L1 (maintaining inclusivity)
	\param[in] address - an address to be looked up in the cache and read from
	*/
	virtual void Read(unsigned address);

	/** Write
	If the block isn't inside L2, add it to L2 (maintaining inculsivity)
	Try to write to the lines of L1.
	If there's a thrown block from L2, remove it also from L1 (maintaining inclusivity)
	\param[in] address - an address to be looked up in the cache and written to
	*/
	virtual void Write(unsigned address);

	/** SetL1
	//Sets the pointer to L1 Cache
	\param[in] l1 - a pointer to an L1Cache
	*/
	void SetL1(L1Cache* l1);

private:
	L1Cache* m_L1_cache;
};


#endif //_CACHE_H