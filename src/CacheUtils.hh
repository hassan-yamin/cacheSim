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


#ifndef CACHE_HPP
#define CACHE_HPP

#include <map>
#include <vector>
#include <ostream>


#define READ_ACCESS 0
#define WRITE_ACCESS 1


enum MemCmd{
	DATA_READ,
	DATA_WRITE,
	DATA_PREFETCH,
	NUM_MEM_CMDS
};


class Access{
	
	public : 
		Access() : m_address(0), m_size(0), m_pc(0), m_type(DATA_READ), m_idthread(0) {};
		Access(uint64_t address, int size, uint64_t pc , MemCmd type, int id_thread) : m_address(address), m_size(size), \
				m_pc(pc), m_hints(0), m_type(type) , m_idthread(id_thread) {};

		bool isWrite() { return m_type == MemCmd::DATA_WRITE;};
		bool isPrefetch() { return m_type == MemCmd::DATA_PREFETCH;};
		
		void print(std::ostream& out) const;

		uint64_t m_address;
		int m_size;
		uint64_t m_pc;
		int m_hints;
		MemCmd m_type;		
		unsigned m_idthread;
};


class CacheEntry{
	public :
		CacheEntry() { 
			initEntry();
		};


		void initEntry() {
			isValid = false; 
			isDirty = false;
			isPrefetch = false;
			address = 0;
			policyInfo = 0; 
			m_pc = 0;
			nbRead = 0;
			nbWrite = 0;
		}
		
		void copyCL(CacheEntry* a)
		{
			address = a->address;
			isDirty = a->isDirty;
			nbWrite = a->nbWrite;
			nbRead = a->nbRead;
			m_pc = a->m_pc;
			isPrefetch = a->isPrefetch;
		}
		bool isValid;
		bool isDirty;
		uint64_t address;
		uint64_t m_pc;

		int policyInfo;
		
		int nbWrite;
		int nbRead;
		
		bool isPrefetch;
};

typedef std::vector<std::vector<CacheEntry*> > DataArray;


class ConfigCache{
	public : 
		ConfigCache(): m_size(0), m_assoc(0), m_blocksize(0), m_policy(""){};
		ConfigCache(int size , int assoc , int blocksize, std::string policy ) : \
		m_size(size), m_assoc(assoc), m_blocksize(blocksize), m_policy(policy){};
		
		ConfigCache(const ConfigCache& a): m_size(a.m_size), m_assoc(a.m_assoc), m_blocksize(a.m_blocksize),\
					 m_policy(a.m_policy) {};		
		int m_size;
		int m_assoc;
		int m_blocksize;
		std::string m_policy;	
};





#endif

