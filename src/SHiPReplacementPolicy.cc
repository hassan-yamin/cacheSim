#include <stdint.h>

#include "SHiPReplacementPolicy.hh"

using namespace std;

SHiPPolicy::SHiPPolicy() : ReplacementPolicy()
{
	m_cpt=1;
}


SHiPPolicy::SHiPPolicy(int nbAssoc , int nbSet , vector<vector<CacheEntry*> > cache_entries):\
	ReplacementPolicy(nbAssoc , nbSet , cache_entries)
{

	m_nbCPUs = 1;
	m_sampler_set = 256 * m_nbCPUs;

	if( nbSet < m_sampler_set)
		m_sampler_set = nbSet;

	m_sampler_way = nbAssoc;

	m_rrpv = vector< vector<uint64_t> >(nbSet , vector<uint64_t>(nbAssoc , maxRRPV));

	m_rand_sets = vector<uint32_t>(m_sampler_set , 0);

	m_sampler.resize(m_sampler_set);
	for(unsigned i = 0 ; i < m_sampler_set ; i++)
	{
		m_sampler[i].resize(m_sampler_way);
		for(unsigned j = 0 ; j < m_sampler_way ; j++)
		{
			m_sampler[i][j] = new SHIP_SAMPLER_entry();
			m_sampler[i][j]->lru = j;
		}
	}
	
	m_SHCT_class.resize(m_nbCPUs);
	for(int i = 0 ; i < m_nbCPUs ; i++)
	{
		m_SHCT_class[i].resize(SHCT_SIZE);	
		for(int j = 0 ; j < SHCT_SIZE ; j++)
		{
			m_SHCT_class[i][j] = new SHIP_SHCT_class();
		}
	}
	
	srand(time(NULL));
	/*
	unsigned long rand_seed = 1;
	unsigned long max_rand = 1048576;
	*/
	
	int do_again = 0;


	for (unsigned i=0; i< m_sampler_set; i++)
	{
		do 
		{
		    do_again = 0;
		    
		    /* Not sure what's going on here, probably just selecting a random set from the LLC */
		    /*
		    rand_seed = rand_seed * 1103515245 + 12345;
		    m_rand_sets[i] = ((unsigned) ((rand_seed/65536) % max_rand)) % my_set;
		    */
		    
		    m_rand_sets[i] =  (unsigned long) rand() % nbSet;
		    
		    //printf("Assign rand_sets[%d]: %u  LLC: %u\n", i, m_rand_sets[i], nbSet);
		    for (unsigned j=0; j<i; j++) 
		    {
		        if (m_rand_sets[i] == m_rand_sets[j]) 
		        {
		            do_again = 1;
		            break;
		        }
		    }
		}while (do_again);

		//printf("m_rand_sets[%d]: %d\n", i, m_rand_sets[i]);
	}

}

SHiPPolicy::~SHiPPolicy()
{
	cout << "DESTRUCTEUR" << endl;
	for(unsigned i = 0 ; i < m_sampler_set ;i++)
	{
		for(unsigned j = 0 ; j < m_sampler_way ; j++)
		{
			delete m_sampler[i][j];
		}
	}

	for(int i = 0 ; i < m_nbCPUs ; i++)
	{
		for(int j = 0 ; j < SHCT_SIZE ; j++)
		{
			delete m_SHCT_class[i][j];
		}
	}
	
}


void
SHiPPolicy::updatePolicy(uint64_t set, uint64_t index, Access element)
{

	llc_update_replacement_state( 0 , set, index , element.m_address , element.m_pc , element.m_type , true);
}


void
SHiPPolicy::insertionPolicy(uint64_t set, uint64_t index, Access element)
{
	llc_update_replacement_state( 0 , set, index , element.m_address , element.m_pc , element.m_type , false);
}


int
SHiPPolicy::evictPolicy(int set)
{

	for(int i = 0 ; i < m_assoc ; i++){
		if(!m_cache_entries[set][i]->isValid) 
			return i;
	}

	// look for the maxRRPV line
	while (1)
	{
		for (unsigned i=0; i< m_rrpv[set].size() ; i++)
		{
		    if (m_rrpv[set][i] == maxRRPV)
		    {		    
			return i;		
		    }
		}

		for ( unsigned i=0; i< m_rrpv[set].size(); i++)
		{
		    m_rrpv[set][i]++;		
		}
	}

	assert(0);
	return 0;
}


// check if this set is sampled
uint64_t 
SHiPPolicy::is_it_sampled(uint64_t set)
{
    for (unsigned i=0; i< m_sampler_set; i++)
    {
        if (m_rand_sets[i] == set)
            return i;    
    }

    return m_sampler_set;
}


// update sampler
void
SHiPPolicy::update_sampler(uint32_t cpu, uint32_t s_idx, uint64_t address, uint64_t ip, MemCmd type)
{
    vector<SHIP_SAMPLER_entry*> s_set = m_sampler[s_idx];
    uint64_t tag = address / (64 * m_nb_set); 
    unsigned match;

    // check hit
    for (match=0; match< m_sampler_way ; match++)
    {
        if (s_set[match]->valid && (s_set[match]->tag == tag))
        {
            uint32_t SHCT_idx = s_set[match]->ip % SHCT_PRIME;
            if (m_SHCT_class[cpu][SHCT_idx]->counter > 0)
                m_SHCT_class[cpu][SHCT_idx]->counter--;

            /*
            if (draw_transition)
                printf("cycle: %lu SHCT: %d ip: 0x%llX SAMPLER_HIT cl_addr: 0x%llX page: 0x%llX block: %ld set: %d\n", 
                ooo_cpu[cpu].current_cycle, SHCT[cpu][SHCT_idx].dead, s_set[match]->ip, address>>6, address>>12, (address>>6) & 0x3F, s_idx);
            */

            //s_set[match]->ip = ip; // SHIP does not update ip on sampler hit
            s_set[match]->type = type; 
            s_set[match]->used = 1;
            //D(printf("sampler hit  cpu: %d  set: %d  way: %d  tag: %x  ip: %lx  type: %d  lru: %d\n",
            //            cpu, rand_sets[s_idx], match, tag, ip, type, s_set[match]->lru));

            break;
        }
    }

    // check invalid
    if (match == m_sampler_way)
    {
        for (match = 0 ; match < m_sampler_way ; match++)
        {
            if (s_set[match]->valid == 0)
            {
                s_set[match]->valid = 1;
                s_set[match]->tag = tag;
                s_set[match]->ip = ip;
                s_set[match]->type = type;
                s_set[match]->used = 0;

                //D(printf("sampler invalid  cpu: %d  set: %d  way: %d  tag: %x  ip: %lx  type: %d  lru: %d\n",
                //            cpu, rand_sets[s_idx], match, tag, ip, type, s_set[match]->lru));
                break;
            }
        }
    }

    // miss
    if (match == m_sampler_way)
    {
        for (match = 0 ; match < m_sampler_way ; match++)
        {
            if (s_set[match]->lru == (m_sampler_way-1)) // Sampler uses LRU replacement
            {
                if (s_set[match]->used == 0)
                {
                    uint32_t SHCT_idx = s_set[match]->ip % SHCT_PRIME;
                    if (m_SHCT_class[cpu][SHCT_idx]->counter < SHCT_MAX)
                        m_SHCT_class[cpu][SHCT_idx]->counter++;

                    /*
                    if (draw_transition)
                        printf("cycle: %lu SHCT: %d ip: 0x%llX SAMPLER_MISS cl_addr: 0x%llX page: 0x%llX block: %ld set: %d\n", 
                        ooo_cpu[cpu].current_cycle, SHCT[cpu][SHCT_idx].dead, s_set[match]->ip, address>>6, address>>12, (address>>6) & 0x3F, s_idx);
                    */
                }

                s_set[match]->tag = tag;
                s_set[match]->ip = ip;
                s_set[match]->type = type;
                s_set[match]->used = 0;

                //D(printf("sampler miss  cpu: %d  set: %d  way: %d  tag: %x  ip: %lx  type: %d  lru: %d\n",
                //            cpu, rand_sets[s_idx], match, tag, ip, type, s_set[match]->lru));
                break;
            }
        }
    }

    // update LRU state
    uint32_t curr_position = s_set[match]->lru;
    for ( unsigned i = 0 ; i < m_sampler_way ; i++)
    {
        if (s_set[i]->lru < curr_position)
            s_set[i]->lru++;
    }
    s_set[match]->lru = 0;

}



void 
SHiPPolicy::llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, MemCmd type, uint8_t hit)
{

    // handle writeback access
    /*
    if (type == WRITEBACK) {
        if (hit)
            return;
        else {
            rrpv[set][way] = maxRRPV-1;
            return;
        }
    }*/

    // update sampler
    uint32_t s_idx = is_it_sampled(set);
     
    if (s_idx < m_sampler_set)
    {
        update_sampler(cpu, s_idx, full_addr, ip, type);

    }


    if (hit)
        m_rrpv[set][way] = 0;
    else {
        // SHIP prediction
        uint32_t SHCT_idx = ip % SHCT_PRIME;
        
        m_rrpv[set][way] = maxRRPV-1;
        if ( m_SHCT_class[cpu][SHCT_idx]->counter == SHCT_MAX)
            m_rrpv[set][way] = maxRRPV;
    }
}



