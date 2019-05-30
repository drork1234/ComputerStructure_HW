#include "Cache.h"
#include <sstream>
#include <stdexcept>

L1Cache::L1Cache(unsigned cSizeLog, unsigned bSizeLog, unsigned cNumWaysLog, Cache* next_level) : Cache(cSizeLog, bSizeLog, cNumWaysLog) , m_L2_cache(next_level)
{
	if (NULL == next_level){
		stringstream err_buff;
		err_buff << __func__ << ": Error: initializing with invalid pointer" << endl;
		throw std::runtime_error(err_buff.str());
	}
}

L1Cache::~L1Cache()
{
	//L1 doesn't own L2, CacheSim does
	m_L2_cache = NULL;
}

//get the block thrown (if thrown) from L1
void L1Cache::Read(unsigned address)
{
	unsigned set = (address & m_set_mask) >> m_offset_width;
	CacheBlock thrown_block;
	//address isn't inside L1
	//read it from L2, then bring it to L1.
	//if a block is thrown from L1, save it to a temprary CacheBlock object
	if (!m_lines[set]->Access(address & m_tag_mask)){
		m_L2_cache->Read(address);
		thrown_block = m_lines[set]->Read(address & m_tag_mask);
	}

	else thrown_block = m_lines[set]->Read(address & m_tag_mask);

	//if the thrown block is valid and dirty, write it to L2
	if (thrown_block.IsValid() && thrown_block.IsDirty())
		m_L2_cache->Write(thrown_block.Tag() | (address & m_set_mask));
	
	return;
}

void L1Cache::Write(unsigned address)
{
	unsigned set = (address & m_set_mask) >> m_offset_width;
	CacheBlock thrown_block;
	
	//address isn't inside L1
	//read it from L2, then bring it to L1.
	//if a block is thrown from L1, save it to a temprary CacheBlock object
	if (!m_lines[set]->Access(address & m_tag_mask)) {
		m_L2_cache->Read(address);
		thrown_block = m_lines[set]->Write(address & m_tag_mask);
	}

	else thrown_block = m_lines[set]->Write(address & m_tag_mask);

	//if the thrown block is valid and dirty, write it to L2
	if (thrown_block.IsValid() && thrown_block.IsDirty())
		m_L2_cache->Write(thrown_block.Tag() | (address & m_set_mask));

}

unsigned L1Cache::Access(unsigned address) const
{
	unsigned set = (address & m_set_mask) >> m_offset_width;

	
	//if L1 miss
	if (! m_lines[set]->Access(address & m_tag_mask) )
		return m_L2_cache->Access(address) ? (unsigned)L2_HIT : (unsigned)L2_MISS;

	else return (unsigned)L1_HIT; //else L1 hit
}

void L1Cache::__Remove(unsigned address)
{
	unsigned set = (address & m_set_mask) >> m_offset_width;

	m_lines[set]->__Remove(address & m_tag_mask);
}
