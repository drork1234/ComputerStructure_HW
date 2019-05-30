#include "Cache.h"
#include <iostream>
#include <cmath>

#define MISS 0
#define HIT 1

Cache::Cache(unsigned cSizeLog, unsigned bSizeLog, unsigned cNumWaysLog) : m_offset_width(bSizeLog)
{
	//construct the size of each argument of the cache
	unsigned	cSize		= (unsigned)pow<unsigned>(2, cSizeLog),
				bSize		= (unsigned)pow<unsigned>(2, bSizeLog),
				num_ways	= (unsigned)pow<unsigned>(2, cNumWaysLog);

	//calculate the number of lines inside the cache (number of sets)
	unsigned num_lines = cSize / (bSize * num_ways);
	
	//get the number of bits inside the set
	unsigned m_set_width = 0;
	while (num_lines >> 1){
		m_set_width++;
		num_lines = num_lines >> 1;
	}
	//get the tag width (in bits)
	m_tag_width = ADDRESS_WIDTH - m_set_width - m_offset_width;

	//create the offset, tag and set bit masks
	m_offset_mask = (1 << m_offset_width) - 1;
	m_set_mask = ((1 << m_set_width) - 1) << m_offset_width;
	m_tag_mask = ((1 << m_tag_width) - 1) << (m_offset_width + m_set_width);

	//create enough space to hold the data
	m_lines.resize(cSize / (bSize * num_ways));

	//allocate all the new lines
	for (size_t i = 0; i < m_lines.size(); i++)
		m_lines[i] = new CacheLine(num_ways);
}

Cache::~Cache()
{
	//free the lines of the cache from the memory
	for (size_t i = 0; i < m_lines.size(); i++){
		if (m_lines[i])
			delete m_lines[i];
	}
	m_lines.clear();
}

void Cache::Read(unsigned address)
{
	//get the set from the address using the set bit mask and right logical shift (with m_offset_width number of bits)
	unsigned set = (address & m_set_mask) >> m_offset_width;

	
	//reference the set'th set inside the cache lines and try to read from it, 
	//sending the address masked by the tag bit mask
	m_lines[set]->Read(address & m_tag_mask);
}

void Cache::Write(unsigned address)
{
	//get the set from the address using the set bit mask and right logical shift (with m_offset_width number of bits)
	unsigned set = (address & m_set_mask) >> m_offset_width;

	//reference the set'th set inside the cache lines and try write to it, 
	//sending the address masked by the tag bit mask
	m_lines[set]->Write(address & m_tag_mask);
}

unsigned Cache::Access(unsigned address) const
{
	//get the set from the address using the set bit mask and right logical shift (with m_offset_width number of bits)
	unsigned set = (address & m_set_mask) >> m_offset_width;

	//reference the set'th set inside the cache lines and try to access it to find the tag inside the line, 
	//sending the address masked by the tag bit mask
	return m_lines[set]->Access(address & m_tag_mask) ? HIT : MISS;
}


