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

#include <map>	
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <math.h>

#include "CacheUtils.hh"
#include "Cache.hh"
#include "common.hh"

#include "ReplacementPolicy.hh"

using namespace std;



Cache::Cache(int size , int assoc , int blocksize , string policy, Hierarchy* my_system){

	m_assoc = assoc;
	m_cache_size = size;
	m_blocksize = blocksize;
	m_nb_set = size / (assoc * blocksize);
	
	m_system = my_system;
	
	assert(isPow2(m_nb_set));
	assert(isPow2(m_blocksize));

	m_policy = policy;
	m_isWarmup = false;
	m_Deallocating = false;
		
	m_dataArray.resize(m_nb_set);
	for(int i = 0  ; i < m_nb_set ; i++){
		m_dataArray[i].resize(m_assoc);
		for(int j = 0 ; j < m_assoc ; j++){
			m_dataArray[i][j] = new CacheEntry();
		}
	}

	if(m_policy == "LRU")
		 m_replacementPolicy = new LRUPolicy(m_assoc, m_nb_set, m_dataArray);	
	else if(m_policy == "Random")
		 m_replacementPolicy = new RandomPolicy(m_assoc, m_nb_set, m_dataArray);
	else if(m_policy == "RRIP")
		 m_replacementPolicy = new RRIPPolicy(m_assoc, m_nb_set, m_dataArray);	
	else {
		assert(false && "Cannot initialize replacement policy for Cache");
	}
			
	m_start_index = log2(blocksize)-1;
	m_end_index = log2(m_blocksize) + log2(m_nb_set);
	
	stats_miss = vector<uint64_t>(2 , 0);
	stats_hits = vector<uint64_t>(2 , 0);
	misses = vector<uint64_t>(2 , 0);
	hits = vector<uint64_t>(2 , 0);
	stats_evict = 0;
	
	stats_nbFetchedLines = 0;
	stats_nbLostLine = 0;
	stats_nbROlines = 0;
	stats_nbROaccess = 0;
	stats_nbRWlines = 0;
	stats_nbRWaccess = 0;
	stats_nbWOlines = 0 ;
	stats_nbWOaccess = 0;
	
	// Record the number of operations issued by the cache 
	stats_operations = vector<uint64_t>(NUM_MEM_CMDS , 0); 

}

Cache::Cache(const Cache& a) : Cache( a.getSize(), a.getAssoc() , a.getBlockSize(), a.getPolicy(), a.getSystem()) 
{
}


Cache::~Cache(){
	for (int i = 0; i < m_nb_set; i++) {
		for (int j = 0; j < m_assoc; j++) {
		    delete m_dataArray[i][j];
		}
	}
}

void 
Cache::finishSimu()
{
	for (int i = 0; i < m_nb_set; i++) {
		for (int j = 0; j < m_assoc; j++) {
		    updateStatsDeallocate(m_dataArray[i][j]);
		}
	}
}

bool
Cache::lookup(Access element)
{	
	DPRINTF(DebugCache , "Lookup of addr %#lx\n" ,  element.m_address);
	return getEntry(element.m_address) != NULL;
}

void  
Cache::handleAccess(Access element)
{
	uint64_t address = element.m_address;
	bool isWrite = element.isWrite();
 	int size = element.m_size;
	
	if(!m_isWarmup)
		stats_operations[element.m_type]++;
	
	assert(size > 0);
	
	uint64_t block_addr = bitRemove(address , 0 , m_start_index+1);
	
	int id_set = addressToCacheSet(address);

	int stats_index = isWrite ? 1 : 0;

	CacheEntry* current = getEntry(address);
	
	if(current == NULL){ // The cache line is not in the cache, Miss !
				
		CacheEntry* replaced_entry = NULL;
		
		int id_assoc = -1;			
		id_assoc = m_replacementPolicy->evictPolicy(id_set);	
		
		replaced_entry = m_dataArray[id_set][id_assoc];
			
		//Deallocate the cache line in the lower levels (inclusive system)
		if(replaced_entry->isValid){
			DPRINTF(DebugCache , "Invalidation of the cache line : %#lx , id_assoc %d\n" , replaced_entry->address,id_assoc);

			//WB this cache line to the system 
			m_system->signalWB(replaced_entry->address, replaced_entry->isDirty);	
			
			if(!m_isWarmup)
				stats_evict++;
		}


		deallocate(replaced_entry);
		allocate(address , id_set , id_assoc, element.m_pc, element.isPrefetch());			
		m_replacementPolicy->insertionPolicy(id_set , id_assoc , 0);
			
		if(!m_isWarmup && !element.isPrefetch()){				
			stats_miss[stats_index]++;			
		//	stats_hits[1]++; // The insertion write 
			misses[stats_index]++;	//hassan		
		//	hits[1]++; // The insertion write, hassan
		}
		if(element.isWrite())
			m_dataArray[id_set][id_assoc]->isDirty = true;
	}
	else{
		// It is a hit in the cache 		
		int id_assoc = m_tag_index.find(current->address)->second;

		DPRINTF(DebugCache , "It is a hit ! Block[%#lx] Set=%d, Way=%d\n" , block_addr, id_set, id_assoc);
		
		m_replacementPolicy->updatePolicy(id_set , id_assoc, 0);
		
		if(element.isWrite())
		{
			current->isDirty = true;
			current->nbWrite++;
		}
		else
			current->nbRead++;

		if(!m_isWarmup && !element.isPrefetch()) //dont count the prefetch...
			{
				stats_hits[stats_index]++;
				hits[stats_index]++;
			}
	}

}


void 
Cache::deallocate(CacheEntry* replaced_entry)
{
	uint64_t addr = replaced_entry->address;
	updateStatsDeallocate(replaced_entry);

	map<uint64_t,int>::iterator it = m_tag_index.find(addr);	
	if(it != m_tag_index.end()){
		m_tag_index.erase(it);	
	}	

	replaced_entry->initEntry();
}

void
Cache::updateStatsDeallocate(CacheEntry* current)
{

	if(!current->isValid || m_isWarmup)
		return;
	
	if(current->nbWrite == 0 && current->nbRead > 0){
		stats_nbROlines++;
		stats_nbROaccess+= current->nbRead; 
	}
	else if(current->nbWrite > 0 && current->nbRead == 0){
		stats_nbWOlines++;
		stats_nbWOaccess+= current->nbWrite; 	
	}
	else if( current->nbWrite == 0 && current->nbRead == 0){	
		stats_nbLostLine++;
	}
	else
	{
		stats_nbRWlines++;
		stats_nbRWaccess+= current->nbWrite + current->nbRead; 	
	}
}

void
Cache::deallocate(uint64_t block_addr)
{
	DPRINTF(DebugCache , "DEALLOCATE %#lx\n", block_addr);
	map<uint64_t,int>::iterator it = m_tag_index.find(block_addr);	
	
	if(it != m_tag_index.end()){

		int id_set = blockAddressToCacheSet(block_addr);

		int current_way = it->second;
		CacheEntry* current = m_dataArray[id_set][current_way]; 		

		updateStatsDeallocate(current);
		current->initEntry();				
		m_tag_index.erase(it);	

	}
}

void 
Cache::allocate(uint64_t address , int id_set , int id_assoc, uint64_t pc, bool isPrefetch)
{

	uint64_t block_addr = bitRemove(address , 0 , m_start_index+1);
	
 	assert(!m_dataArray[id_set][id_assoc]->isValid);

	m_dataArray[id_set][id_assoc]->isValid = true;	
	m_dataArray[id_set][id_assoc]->address = block_addr;
	m_dataArray[id_set][id_assoc]->policyInfo = 0;		
	m_dataArray[id_set][id_assoc]->m_pc = pc;		
	m_dataArray[id_set][id_assoc]->isPrefetch = isPrefetch;
	
	if(!m_isWarmup)
		stats_nbFetchedLines++;	
		
	m_tag_index.insert(pair<uint64_t , int>(block_addr , id_assoc));
}


int 
Cache::addressToCacheSet(uint64_t address) 
{
	uint64_t a = address;
	a = bitRemove(a , 0, m_start_index);
	a = bitRemove(a , m_end_index,64);
	
	a = a >> (m_start_index+1);
	assert(a < (unsigned int)m_nb_set);
	
	return (int)a;
}


void Cache::startWarmup()
{
	m_isWarmup = true;
}

void Cache::stopWarmup()
{
	m_isWarmup = false;
}


int 
Cache::blockAddressToCacheSet(uint64_t block_addr) 
{
	uint64_t a =block_addr;
	a = bitRemove(a , m_end_index,64);
	
	a = a >> (m_start_index+1);
	assert(a < (unsigned int)m_nb_set);
	
	return (int)a;
}

CacheEntry*
Cache::getEntry(uint64_t addr)
{
	uint64_t block_addr =  bitRemove(addr , 0 , m_start_index+1);

	map<uint64_t,int>::iterator p = m_tag_index.find(block_addr);
	
	if (p != m_tag_index.end()){
	
		int id_set = addressToCacheSet(addr);
		int current_way = p->second;
		return m_dataArray[id_set][current_way];	
	}
	else{
		return NULL;
	}
}

void 
Cache::openNewTimeFrame()
{
}
	
bool
Cache::isPrefetchBlock(uint64_t addr)
{
	CacheEntry* entry = getEntry(addr);
	if(entry != NULL)
		return entry->isPrefetch;
	return false;
}

void
Cache::resetPrefetchFlag(uint64_t addr)
{
	CacheEntry* entry = getEntry(addr);
	if(entry != NULL)
		entry->isPrefetch = false;
}

void 
Cache::print(ostream& out) 
{
	printResults(out);	
}

void 
Cache::printinterval(ostream& out, ostream& out2, unsigned long long int x) 
{
	string entete = "Cache";
	uint64_t total_miss =  misses[0] + misses[1];
	uint64_t total_access =  hits[0] + hits[1] + misses[0] + misses[1];
//	out << total_miss << "..." << total_access << "..." << (double)(total_miss)*100 / (double)(total_access) << "%"<< endl;
//	out2 << total_miss << "..." << x << "..." << (double)(total_miss)*1000 / (double)(x) << endl;
	out << (double)(total_miss)*100 / (double)(total_access) << "%"<< endl;
	out2 << (double)(total_miss)*1000 / (double)(x) << endl;
	misses[0] = 0;
	misses[1] = 0;
	hits[0] = 0;
	hits[1] = 0;
}


void
Cache::printConfig(std::ostream& out) 
{		
	string entete = "Cache";
	out << entete << ":Size\t" << m_cache_size << endl;
	out << entete << ":Assoc\t" << m_assoc << endl;
	out << entete << ":Blocksize\t" << m_blocksize << endl;
	out << entete << ":Sets    \t" << m_nb_set << endl;
	out << entete << ":ReplacementPolicy\t" << m_policy << endl;
	out << "************************" << endl;
}


void 
Cache::printResults(std::ostream& out) 
{
	uint64_t total_miss =  stats_miss[0] + stats_miss[1];
	uint64_t total_access =  stats_hits[0] + stats_hits[1] + stats_miss[0] + stats_miss[1];
	uint64_t total_hits = stats_hits[0] + stats_hits[1];
	string entete = "Cache";

	if(total_miss != 0){
	
		out << entete << ":Results" << endl;
		out << entete << ":TotalAccess\t"<< total_access << endl;
		out << entete << ":TotalHits\t" << total_hits << endl;
		out << entete << ":TotalMiss\t" << total_miss << endl;		
		out << entete << ":MissRate\t" << (double)(total_miss)*100 / (double)(total_access) << "%"<< endl;
		out << entete << ":Eviction\t" << stats_evict << endl;	

		out << entete << ":BlockClassification" << endl;
		out << entete << ":FetchedBlocks\t" << stats_nbFetchedLines << endl;
		out << entete << ":DeadBlocks\t" << stats_nbLostLine << "\t" << \
			(double)stats_nbLostLine*100/(double)stats_nbFetchedLines << "%" << endl;
		out << entete << ":ROBlocks\t" << stats_nbROlines << "\t" << \
			(double)stats_nbROlines*100/(double)stats_nbFetchedLines << "%" << endl;
		out << entete << ":WOBlocks\t" << stats_nbWOlines << "\t" << \
			(double)stats_nbWOlines*100/(double)stats_nbFetchedLines << "%" << endl;
		out << entete << ":RWBlocks\t" << stats_nbRWlines << "\t" << \
			(double)stats_nbRWlines*100/(double)stats_nbFetchedLines << "%" << endl;

		out << entete << ":instructionsDistribution" << endl;

		for(unsigned i = 0 ; i < stats_operations.size() ; i++){
			if(stats_operations[i] != 0)
				out << entete << ":" << memCmd_str[i]  << "\t" << stats_operations[i] << endl;
		}
	
	}
}


