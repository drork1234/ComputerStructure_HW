#include "CacheLine.h"
#include <algorithm>
#include <cstddef>
#define ADDRESS_WIDTH 32



//-------------------------helper function object------------------------
class FindBlockFN
{
public:
	FindBlockFN(unsigned tag) : m_tag(tag) {}
	
	bool operator()(CacheBlock const& block) { return block.Tag() == m_tag && block.IsValid(); }

private:
	unsigned m_tag;
};

//lambdas could have made it a lot easier

CacheLine::CacheLine(unsigned num_ways) : m_num_ways(num_ways)
{
}

CacheLine::~CacheLine()
{
	m_ways.clear();
}

CacheBlock CacheLine::Read(unsigned tag, bool* is_thrown)
{
	auto found_block_it = find_if(m_ways.begin(), m_ways.end(), FindBlockFN(tag));
	CacheBlock thrown_block;
	//block is aleady inside the ways container, just rearrange the LRU
	if (m_ways.end() != found_block_it) {
		rotate(m_ways.begin(), found_block_it, found_block_it + 1);
		if (is_thrown)
			*is_thrown = false;
	}
	//else block was not found
	else{
		//if the way container is full throw away the LRU 
		if (m_ways.size() >= m_num_ways) {
			thrown_block = m_ways.back();
			m_ways.pop_back();
			if(is_thrown) 
				*is_thrown = true;
		} 
		else if (is_thrown) 
			*is_thrown = false;
		//insert a new block to the way container as the MRU
		m_ways.push_front(CacheBlock(tag));
	}
	
	return thrown_block;
}

CacheBlock CacheLine::Write(unsigned tag, bool* is_thrown)
{
	auto found_block_it = find_if(m_ways.begin(), m_ways.end(), FindBlockFN(tag));
	CacheBlock thrown_tag;
	//block is aleady inside the ways container, just rearrange the LRU
	if (m_ways.end() != found_block_it) {
		found_block_it->SetDirty(true);
		//replace the MRU
		rotate(m_ways.begin(), found_block_it, found_block_it + 1);
		if (is_thrown)
			*is_thrown = false;
	}
	//else block was not found
	else {
		//if the way container is full throw away the LRU 
		if (m_ways.size() >= m_num_ways) {
			thrown_tag = m_ways.back().Tag();
			m_ways.pop_back();
			if (is_thrown)
				*is_thrown = true;
		}
		else if (is_thrown)
			*is_thrown = false;
		//insert a new block to the way container as the MRU
		m_ways.push_front(CacheBlock(tag));
		m_ways.front().SetDirty(true);
	}

	return thrown_tag;
}

bool CacheLine::Access(unsigned tag) const
{
	return m_ways.end() != find_if(m_ways.begin(), m_ways.end(), FindBlockFN(tag));
}

void CacheLine::__Remove(unsigned tag)
{
	//find and remove a block according to a tag using a FindBlockFN instance
	m_ways.erase(	remove_if(m_ways.begin(), m_ways.end(), FindBlockFN(tag)), 
					m_ways.end());

	return;
}
