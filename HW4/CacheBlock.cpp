#include "CacheBlock.h"

CacheBlock::CacheBlock() : m_is_valid(false), m_is_dirty(false), m_tag(0x00000000)
{

}

CacheBlock::CacheBlock(unsigned tag) : m_is_valid(true), m_is_dirty(false), m_tag(tag)
{
}

CacheBlock::CacheBlock(CacheBlock const& cp_blk) : m_is_valid(cp_blk.m_is_valid), m_is_dirty(cp_blk.m_is_dirty), m_tag(cp_blk.m_tag)
{

}

CacheBlock::~CacheBlock()
{
	m_is_valid = m_is_dirty = false;
	m_tag = 0x00000000;
}

bool CacheBlock::IsValid() const
{
	return m_is_valid;
}

bool CacheBlock::IsDirty() const
{
	return m_is_dirty;
}

unsigned CacheBlock::Tag() const
{
	return m_tag;
}

void CacheBlock::SetTag(unsigned new_tag)
{
	m_tag = new_tag;
	m_is_valid = true;
}

void CacheBlock::SetDirty(bool dirty)
{
	m_is_dirty = dirty;
}

CacheBlock & CacheBlock::operator=(CacheBlock const & blk)
{
	this->m_is_valid = blk.m_is_valid;
	this->m_is_dirty = blk.m_is_dirty;
	this->m_tag = blk.m_tag;

	return *this;
}
