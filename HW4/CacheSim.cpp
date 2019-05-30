#include "CacheSim.h"
#include <sstream>
#include <exception>
#include <stdexcept>
#include <assert.h>

CacheSim::CacheSim(unsigned mem_latency, unsigned logBlockSize, vector<cache_args> args)  : m_L1_miss_count(0), m_L1_access_count(0), m_L2_miss_count(0), m_L2_access_count(0), m_accumulated_time(0.0)
{
	if (args.size() != NUM_CACHES) {
		stringstream err_buff;
		err_buff << __func__ << ": Error: Number of argument vector isn't sufficient, Needs to be " << NUM_CACHES << endl;
		throw std::runtime_error(err_buff.str());
	}

	m_caches.resize(NUM_CACHES);
	

	//initialize the the caches
	//for the record, I dislike this type of initialization
	m_caches[1] = new L2Cache(args[1][0], logBlockSize, args[1][1], NULL);
	m_caches[0] = new L1Cache(args[0][0], logBlockSize, args[0][1], m_caches[1]);

	static_cast<L2Cache*>(m_caches[1])->SetL1(static_cast<L1Cache*>(m_caches[0]));

	//save the access latencies
	m_latencies.reserve(args.size());
	for (size_t i = 0; i < m_latencies.capacity(); i++)
		m_latencies.push_back(args[i][2]);

	m_latencies.push_back(mem_latency);
	return;
}

CacheSim::~CacheSim()
{
	for (int i = m_caches.size() - 1; i >= 0; i--){
		if (m_caches[i])
			delete m_caches[i];
	}

	//m_caches.clear();
}

void CacheSim::Access(char op, unsigned address)
{
	//access the cache first
	auto& L1 = *m_caches[0];
	m_L1_access_count++;
	switch (L1.Access(address))
	{
	case 0:
		m_accumulated_time += m_latencies[0];
		break;

	case 1:
		m_accumulated_time += m_latencies[1];
		m_L1_miss_count++;
		m_L2_access_count++;
		break;

	case 2:
		m_accumulated_time += m_latencies[2];
		m_L1_miss_count++;
		m_L2_access_count++;
		m_L2_miss_count++;
		break;

	default:
		break;
	}

	//operate according to 'op'
	switch (op)
	{
	case 'r':	__Read(address); break;
	case 'w':	__Write(address); break;
	default:
		break;
	}
}

double CacheSim::L1MissRate() const
{
	if (0 == m_L1_access_count) throw std::runtime_error("Devision by zero exception");
	return static_cast<double>(m_L1_miss_count) / m_L1_access_count;
}

double CacheSim::L2MissRate() const
{
	if (0 == m_L2_access_count) throw std::runtime_error("Devision by zero exception");
	return static_cast<double>(m_L2_miss_count) / m_L2_access_count;
}

double CacheSim::AvgAccTime() const
{
	return m_accumulated_time / m_L1_access_count;
}


//private methods
bool CacheSim::_CheckValidity() const
{
	for (size_t i = 0; i < m_caches.size(); i++){
		if (NULL == m_caches[i]) return false;
	}

	return true;
}

void CacheSim::__Read(unsigned address)
{
	assert(_CheckValidity() && "Error: cache is not valid");
	//try to access L1 cache
	//if there's a MISS, try to access L2 cache
	//if there's a MISS, 

	m_caches[0]->Read(address);
}

void CacheSim::__Write(unsigned address)
{
	assert(_CheckValidity() && "Error: cache is not valid");
	//try to access L1 cache
	//if there's a MISS, try to access L2 cache
	//if there's a MISS, 

	m_caches[0]->Write(address);
}
