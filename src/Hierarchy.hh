#ifndef HIERARCHY_HH
#define HIERARCHY_HH

#include <vector>
#include <string>
#include <assert.h>
#include <iostream>

#include "Cache.hh"
#include "CacheUtils.hh"
#include "Prefetcher.hh"

class Access;
class Cache;

class Hierarchy
{

	public:
		Hierarchy();
		Hierarchy(const Hierarchy& a);
		Hierarchy(std::string policy);
		~Hierarchy();
		void print(std::ostream& out);
		void handleAccess(Access element);
		void signalWB(uint64_t addr, bool isDirty);
		void printResults(std::ostream& out);
		void printinterval(std::ostream& out);
		void printConfig(std::ostream& out);
		void finishSimu();
		void openNewTimeFrame();
		int convertThreadIDtoCore(int id_thread);			
		void prefetchAddress(Access element);

		void startWarmup();
		void stopWarmup();
		
	protected:
	
		Cache* m_cache;
		Prefetcher* m_prefetcher;

		int m_start_index;
		uint64_t stats_beginTimeFrame;
		uint64_t stats_cleanWB_MainMem;
		uint64_t stats_dirtyWB_MainMem;
		uint64_t stats_readMainMem;
		uint64_t stats_writeMainMem;

		uint64_t stats_issuedPrefetchs;
		uint64_t stats_hitsPrefetch;
};


#endif
