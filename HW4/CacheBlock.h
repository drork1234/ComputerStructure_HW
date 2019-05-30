#ifndef _CACHE_BLOCK_H
#define _CACHE_BLOCK_H

class CacheBlock
{
public:
	/** CacheBlock: default constructor
	*/
	CacheBlock();

	/** CacheBlock: constructor
	\param[in] tag - the tag of the address of the data inside the block
	*/
	CacheBlock(unsigned tag);
	
	/** CacheBlock: copy constructor
	\param[in] cp_blk - a const reference to a block object to be copied
	*/
	CacheBlock(CacheBlock const& cp_blk);
	
	/** ~CacheBlock: destructor
	*/
	~CacheBlock();

public:
	/** IsValid
	\return - true if the block is valid, false otherwise
	*/
	bool IsValid() const;
	
	/** IsDirty
	\return - true if the block has been written to, false otherwise
	*/
	bool IsDirty() const;
	
	/** Tag
	\return - the tag of the block
	*/
	unsigned Tag() const;
	
	/** SetTag
	\sets the tag of the block
	\param[in] new_tag - the new tag of the block
	*/
	void SetTag(unsigned new_tag);
	
	/** SetDirty
	\sets the dirty bit of the block
	\param[in] dirty - the new state of the block - true for dirty, false for not dirty
	*/
	void SetDirty(bool dirty);

	/** operator=
	\assignment operator overload
	\param[in] blk - a const reference to a CacheBlock object to be assigned to this object
	*/
	CacheBlock& operator=(CacheBlock const& blk);

private:
	bool m_is_valid;
	bool m_is_dirty;
	unsigned m_tag;
};


#endif // _CACHE_BLOCK_H
