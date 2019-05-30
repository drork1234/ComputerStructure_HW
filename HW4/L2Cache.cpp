#include "Cache.h"
#include <iostream>
#include <bitset>

L2Cache::L2Cache(unsigned cSizeLog, unsigned bSizeLog, unsigned cNumWaysLog, L1Cache* first_level_cache) : Cache(cSizeLog, bSizeLog, cNumWaysLog), m_L1_cache(first_level_cache)
{
}

L2Cache::~L2Cache()
{
	m_L1_cache = NULL;
}

void L2Cache::Read(unsigned address) {
	unsigned set = (address & m_set_mask) >> m_offset_width;

	//assert(set < m_lines.size() && "Error at read");

	//no need to send a flag, we don't actually write to memory
	CacheBlock thrown_block = m_lines[set]->Read(address & m_tag_mask);

	//if the thrown block from L2 is valid and resides in L1, remove it (inclusive) 
	if (thrown_block.IsValid()) 
		m_L1_cache->__Remove(thrown_block.Tag() | (address & m_set_mask));
}

void L2Cache::Write(unsigned address)
{
	unsigned set = (address & m_set_mask) >> m_offset_width;

	//assert(set < m_lines.size() && "Error at read");

	//no need to send a flag, we don't actually write to memory
	CacheBlock thrown_block = m_lines[set]->Write(address & m_tag_mask);

	//if the thrown block from L2 is valid and resides in L1, remove it (inclusive)
	if (thrown_block.IsValid())
		m_L1_cache->__Remove(thrown_block.Tag() | (address & m_set_mask));
}

void L2Cache::SetL1(L1Cache * l1)
{
	m_L1_cache = l1;	
}
