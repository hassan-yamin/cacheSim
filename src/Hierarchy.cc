#include <math.h>
#include <iostream>
#include <string>
#include <vector>

#include "Hierarchy.hh"
#include "Cache.hh"
#include "common.hh"

using namespace std;


Hierarchy::Hierarchy()
{
	DPRINTF(DebugHierarchy, "Hierarchy:: null constructor\n");

	Hierarchy("LRU");
}

Hierarchy::Hierarchy(const Hierarchy& a)
{
	DPRINTF(DebugHierarchy, "Hierarchy:: copy constructor\n");
}


Hierarchy::Hierarchy(string policy)
{
	
	int nb_sets = simu_parameters.nb_sets;
	int assoc = simu_parameters.assoc;
	int size = assoc * nb_sets * BLOCK_SIZE;

	m_prefetcher = new SimplePrefetcher( simu_parameters.prefetchDegree , simu_parameters.prefetchStreams , true);

	stats_beginTimeFrame = 0;

	m_cache = new Cache(size , assoc , BLOCK_SIZE , policy , this );
	m_start_index = log2(BLOCK_SIZE)-1;
	
	stats_cleanWB_MainMem = 0;
	stats_dirtyWB_MainMem = 0;
	stats_writeMainMem = 0;
	stats_readMainMem = 0;
	
	stats_hitsPrefetch = 0;
	stats_issuedPrefetchs = 0;
}


Hierarchy::~Hierarchy()
{
	delete m_cache;
	delete m_prefetcher;
}

void
Hierarchy::startWarmup()
{
	m_cache->startWarmup();
}

void
Hierarchy::stopWarmup()
{
	m_cache->stopWarmup();
}


void
Hierarchy::printResults(ostream& out)
{

	m_cache->printResults(out);
	out << "***************" << endl;

	out << "Prefetcher:Stats" << endl;
	out << "Prefetcher:issuedPrefetch\t"<< stats_issuedPrefetchs << endl;
	out << "Prefetcher:hitPrefecth\t"<< stats_hitsPrefetch << endl;
	out << "Prefetcher:prefetchErrors\t"<< stats_issuedPrefetchs - stats_hitsPrefetch << endl;


	out << "MainMem:ReadFetch\t" << stats_readMainMem << endl;
	out << "MainMem:WriteFetch\t" << stats_writeMainMem << endl;
	out << "MainMem:CleanWB\t" << stats_cleanWB_MainMem << endl;
	out << "MainMem:DirtyWB\t" << stats_dirtyWB_MainMem << endl;
	
}

void
Hierarchy::printinterval(ostream& out)
{

	m_cache->printinterval(out);
}

void
Hierarchy::printConfig(ostream& out)
{
	m_cache->printConfig(out);
}

void
Hierarchy::print(std::ostream& out) 
{
	printResults(out);
}

void
Hierarchy::signalWB(uint64_t addr, bool isDirty)
{
	// It is a WB from the LLC to the Main Mem
	if(isDirty)
		stats_dirtyWB_MainMem++;		
	else
		stats_cleanWB_MainMem++;
	
}

void 
Hierarchy::finishSimu()
{
	m_cache->finishSimu();
}


void 
Hierarchy::prefetchAddress(Access element)
{
	uint64_t addr = element.m_address;
	uint64_t block_addr = bitRemove(addr , 0 , m_start_index+1);
	

	DPRINTF(DebugHierarchy, "HIERARCHY::Prefetching of block %#lx\n", block_addr);

	bool isPresent = m_cache->getEntry(addr) == NULL ? false : true;
			
	if(!isPresent)
	{
		m_cache->handleAccess(element);
		stats_issuedPrefetchs++;
	}
	else
	{
		DPRINTF(DebugHierarchy, "HIERARCHY::Prefetcher Block %#lx already present\n", block_addr);
	}
}

void
Hierarchy::handleAccess(Access element)
{
	DPRINTF(DebugHierarchy, "HIERARCHY::New Access : Data %#lx Req %s, Id_Thread %d\n", element.m_address , memCmd_str[element.m_type], element.m_idthread);
	
	uint64_t addr = element.m_address;
	uint64_t block_addr = bitRemove(addr , 0 , m_start_index+1);
	
	bool isMiss = m_cache->getEntry(block_addr) == NULL ? true : false;
	
	if( isMiss)
	{
		if(element.isWrite())
			stats_writeMainMem++;
		else
			stats_readMainMem++;
		
		if( simu_parameters.enablePrefetch) //We generate prefetch only for miss 
		{
			vector<uint64_t> prefetch_addresses = m_prefetcher->getNextAddress(element.m_address);
			for(auto p : prefetch_addresses)
			{
				Access request;
				request.m_address = p;
				request.m_size = 4;
				request.m_type = MemCmd::DATA_PREFETCH;
				prefetchAddress(request);
			}
		}	 
	}
	else 
	{
		/* Prefetch stats collection */ 
		stats_hitsPrefetch += m_cache->isPrefetchBlock(element.m_address) ? 1 : 0;
		m_cache->resetPrefetchFlag(element.m_address);
	}

	m_cache->handleAccess(element);

	if( (cpt_time - stats_beginTimeFrame) > PREDICTOR_TIME_FRAME)
		openNewTimeFrame();
	
	DPRINTF(DebugHierarchy, "HIERARCHY::End of handleAccess\n");
}


void
Hierarchy::openNewTimeFrame()
{	
	m_cache->openNewTimeFrame();	
	stats_beginTimeFrame = cpt_time;
}


