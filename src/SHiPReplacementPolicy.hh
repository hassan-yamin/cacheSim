#ifndef __SHIP_REPLACEMENT_POLICY_HH
#define __SHIP_REPLACEMENT_POLICY_HH

#include <vector>
#include <stdint.h>

#include "Cache.hh"

#define maxRRPV 3
#define SHCT_SIZE  16384
#define SHCT_PRIME 16381
#define SHCT_MAX 7


class SHIP_SAMPLER_entry
{
  public:
    uint8_t valid, used;
    uint64_t tag, cl_addr, ip;
    uint32_t lru;
    
    MemCmd type;

    SHIP_SAMPLER_entry() {
        valid = 0;
        type = DATA_READ;
        used = 0;

        tag = 0;
        cl_addr = 0;
        ip = 0;

        lru = 0;
    };
};


// prediction table structure
class SHIP_SHCT_class {
  public:
    uint32_t counter;

    SHIP_SHCT_class() {
        counter = 0;
    };
};



class SHiPPolicy : public ReplacementPolicy {

	public :
	
		/******** Standard ReplacementPolicy API ************/
		SHiPPolicy();
		SHiPPolicy(int nbAssoc , int nbSet , std::vector<std::vector<CacheEntry*> > cache_entries);
		void updatePolicy(uint64_t set, uint64_t index, Access element);
		void insertionPolicy(uint64_t set, uint64_t index, Access element);
		int evictPolicy(int set);
		~SHiPPolicy();
		
		/*****************************************************/ 
		
		
		void update_sampler(uint32_t cpu, uint32_t s_idx, uint64_t address, uint64_t ip, MemCmd type);
		void llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, MemCmd type, uint8_t hit);
		uint64_t is_it_sampled(uint64_t set);

		
	private : 
	
		uint64_t m_cpt;
		unsigned m_sampler_set;
		unsigned m_sampler_way;
		
		int m_nbCPUs;
		
		std::vector< std::vector<uint64_t> > m_rrpv;

		std::vector<uint32_t> m_rand_sets;
		std::vector< std::vector<SHIP_SAMPLER_entry*> > m_sampler;
		std::vector< std::vector<SHIP_SHCT_class*> > m_SHCT_class;

};

#endif
