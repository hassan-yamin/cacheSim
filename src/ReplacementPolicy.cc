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

#include <vector>
#include <iostream>
#include <map>
#include <climits>

#include "common.hh"
#include "ReplacementPolicy.hh"


using namespace std;


ReplacementPolicy::~ReplacementPolicy()
{

}

LRUPolicy::LRUPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries) : ReplacementPolicy(nbAssoc , nbSet, cache_entries) 
{
	m_cpt = 1;
}

void
LRUPolicy::updatePolicy(uint64_t set, uint64_t index, Access element)
{	
	m_cache_entries[set][index]->policyInfo = m_cpt;
	m_cpt++;
}

int
LRUPolicy::evictPolicy(int set)
{
	int smallest_time = m_cache_entries[set][0]->policyInfo , smallest_index = 0;

	for(int i = 0 ; i < m_assoc ; i++){
		if(!m_cache_entries[set][i]->isValid) 
			return i;
	}
	
	for(int i = 0 ; i < m_assoc ; i++){
		if(m_cache_entries[set][i]->policyInfo < smallest_time){
			smallest_time =  m_cache_entries[set][i]->policyInfo;
			smallest_index = i;
		}
	}
	return smallest_index;
}
//do this for RRIP policy
RRIPPolicy::RRIPPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries) : ReplacementPolicy(nbAssoc , nbSet, cache_entries) 
{

}

void
RRIPPolicy::updatePolicy(uint64_t set, uint64_t index, Access element)
{	
	//when hit, move the block to 0
	m_cache_entries[set][index]->policyInfo = 0;
}
void RRIPPolicy::insertionPolicy(uint64_t set, uint64_t index, Access element)
{
	//insert at 6
	m_cache_entries[set][index]->policyInfo = 6;
}
int
RRIPPolicy::evictPolicy(int set)
{
	int largest_time = m_cache_entries[set][0]->policyInfo , smallest_index = 0;
// if there is an empty space, no need to evict
	for(int i = 0 ; i < m_assoc ; i++){
		if(!m_cache_entries[set][i]->isValid) 
			return i;
	}
// find the largest RRPV value
	for(int i = 0 ; i < m_assoc ; i++){
		if(m_cache_entries[set][i]->policyInfo > largest_time){
			largest_time =  m_cache_entries[set][i]->policyInfo;
	
		}
	}
//find the first index with the largest time
	for(int i = 0 ; i < m_assoc ; i++){
		if(m_cache_entries[set][i]->policyInfo == largest_time){
			smallest_index = i;
			goto breakout;
		}
	}

breakout:
//update +1 on all of the cache blocks
	for(int i = 0 ; i < m_assoc ; i++){
		
		if (i!= smallest_index && m_cache_entries[set][i]->policyInfo < max)
		{
			m_cache_entries[set][i]->policyInfo = m_cache_entries[set][i]->policyInfo + 1;
		}
		
	}

	return smallest_index;
}






int 
RandomPolicy::evictPolicy(int set)
{
	return rand()%m_assoc;
}

