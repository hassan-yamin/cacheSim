/** 
Copyright (C) 2016 Gregory Vaumourin

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**/


#ifndef __CACHE_HH__
#define __CACHE_HH__

#include <map>
#include <stdint.h>
#include <vector>
#include <ostream>

#include "Cache.hh"
#include "ReplacementPolicy.hh"
#include "Hierarchy.hh"
#include "common.hh"

class Access;
class CacheEntry;
class Hierarchy;

class Cache {


	public : 
		Cache(int size , int assoc , int blocksize, std::string policy, Hierarchy* m_system);
		Cache(const Cache& a);
		~Cache();

		void handleAccess(Access element);

		void printResults(std::ostream& out);
		void printConfig(std::ostream& out);
		void print(std::ostream& out);
		void printinterval(std::ostream& out);

		bool lookup(Access element);
		int addressToCacheSet(uint64_t address);
		int blockAddressToCacheSet(uint64_t block_add); 
		int findTagInSet(int id_set, uint64_t address); 
		void deallocate(CacheEntry* entry);
		void deallocate(uint64_t addr);
		void allocate(uint64_t address , int id_set , int id_assoc, uint64_t pc, bool isPrefetch);		
		CacheEntry* getEntry(uint64_t addr);
		
		void updateStatsDeallocate(CacheEntry* current);
		void finishSimu();
		void openNewTimeFrame();
		
		void startWarmup();
		void stopWarmup();
		
		/** Accessors */
		int getSize() const { return m_cache_size;}
		Hierarchy* getSystem() const { return m_system;}
		int getBlockSize() const { return m_blocksize;}
		int getAssoc() const { return m_assoc;}
		int getNbSets() const { return m_nb_set;}
		int getStartBit() const { return m_start_index;}
		std::string getPolicy() const { return m_policy;}
		bool isPrefetchBlock(uint64_t block_addr);
		void resetPrefetchFlag(uint64_t block_addr);

	private :
	
		int m_ID;
		
		std::vector<std::vector<CacheEntry*> > m_dataArray;
		Hierarchy* m_system;

		std::map<uint64_t , int> m_tag_index;    		
   		ReplacementPolicy *m_replacementPolicy;		
		std::string m_policy;
		bool m_isWarmup; //Stats are not recorded during warmup phase
		bool m_Deallocating;
		
		int m_start_index;
		int m_end_index;
		int m_cache_size;
		int m_assoc;
		int m_nb_set;
		int m_blocksize;    
		
		/* Stats */ 		
		std::vector<uint64_t> stats_miss;
		std::vector<uint64_t> stats_hits;
		std::vector<uint64_t> misses; //hassan
		std::vector<uint64_t> hits; //hassan
		uint64_t stats_evict;

		std::vector<uint64_t> stats_operations;
		uint64_t stats_nbFetchedLines;
		uint64_t stats_nbLostLine;

		uint64_t stats_nbROlines;
		uint64_t stats_nbROaccess;

		uint64_t stats_nbRWlines;
		uint64_t stats_nbRWaccess;

		uint64_t stats_nbWOlines;
		uint64_t stats_nbWOaccess;

};



#endif

