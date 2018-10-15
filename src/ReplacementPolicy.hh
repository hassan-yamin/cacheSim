#ifndef REPLACEMENT_POLICY_HPP
#define REPLACEMENT_POLICY_HPP

#include <vector>
#include <stdint.h>

#include "CacheUtils.hh"

class CacheEntry;

class ReplacementPolicy{
	
	public : 
		ReplacementPolicy() : m_nb_set(0), m_assoc(0) {};
		ReplacementPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries ) : m_cache_entries(cache_entries),\
											m_nb_set(nbSet) , m_assoc(nbAssoc) {};
		virtual ~ReplacementPolicy();
		virtual void updatePolicy(uint64_t set, uint64_t index, Access element) = 0;
		virtual void insertionPolicy(uint64_t set, uint64_t index, Access element) = 0;
		virtual int evictPolicy(int set) = 0;
		
	protected : 
		std::vector<std::vector<CacheEntry*> > m_cache_entries;
		int m_nb_set;
		int m_assoc;
	
};


class LRUPolicy : public ReplacementPolicy {

	public :
		LRUPolicy() : ReplacementPolicy(){ m_cpt=1;};
		LRUPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries);
		void updatePolicy(uint64_t set, uint64_t index, Access element);
		void insertionPolicy(uint64_t set, uint64_t index, Access element) { updatePolicy(set,index, element);}
		int evictPolicy(int set);
		~LRUPolicy() {};
	private : 
		uint64_t m_cpt;
		
};

class RandomPolicy : public ReplacementPolicy {

	public :
		RandomPolicy() :  ReplacementPolicy(){};
		RandomPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries) : ReplacementPolicy(nbAssoc, nbSet, cache_entries) {
			srand(time(NULL));
		};
		void updatePolicy(uint64_t set, uint64_t index, Access element){};
		void insertionPolicy(uint64_t set, uint64_t index, Access element){};
		int evictPolicy(int set);
		~RandomPolicy(){};
};

class RRIPPolicy : public ReplacementPolicy {

	public :
		RRIPPolicy() : ReplacementPolicy(){ m_cpt=1;};
		RRIPPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries);
		void updatePolicy(uint64_t set, uint64_t index, Access element);
		void insertionPolicy(uint64_t set, uint64_t index, Access element);
		int evictPolicy(int set);
		~RRIPPolicy() {};
	private : 
		uint64_t m_cpt;
		int insert = 6;
		int hit = 0;
		int max = 7;
		
};
#endif
