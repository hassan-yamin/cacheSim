#include <map>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include "pin.H"
#include "Hierarchy.hh"
#include "Cache.hh"
#include "common.hh"

using namespace std;

Hierarchy* my_system;
Access element;

int start_debug;
// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;
uint64_t mem_accesses_interval; //memory accesses per interval
long long unsigned int num_accesses; //total number of memory accesses to monitor.... or memory accesses to run
//ofstream log_file;
ofstream output_file; // results file
ofstream config_file; //configuration file
ofstream miss_rate; // miss rate per interval dump	
ofstream mpki; //mpki per interval dump
ofstream accessfile; // number of accesses per interval dump
ofstream prefetch_issued; //prefetch issued this interval
ofstream prefetch_hits; // prefetcher hits this interval
PIN_LOCK lock;

bool startInstruFlag;

// This function is called before every instruction is executed
inline VOID docount() { 
	icount++; 
	if (icount % interval == 0)
	{
		my_system->printinterval(miss_rate, mpki, interval, prefetch_issued, prefetch_hits);
		accessfile << mem_accesses_interval << endl;
		mem_accesses_interval = 0;
	}
//	if (cpt_time == num_accesses)
	if (icount == num_accesses)
		PIN_ExitApplication(0);
}


VOID access(uint64_t pc , uint64_t addr, MemCmd type, int size, int id_thread)
{
	PIN_GetLock(&lock, id_thread);

	Access my_access = Access(addr, size, pc , type , id_thread);
	my_access.m_idthread = 0;
	my_system->handleAccess(my_access);		
	cpt_time++;
	mem_accesses_interval++;
	PIN_ReleaseLock(&lock);
}

/* Record Data Read Fetch */
VOID RecordMemRead(VOID* pc , VOID* addr, int size, int id_thread)
{
	uint64_t convert_pc = reinterpret_cast<uint64_t>(pc);
	uint64_t convert_addr = reinterpret_cast<uint64_t>(addr);

	access(convert_pc , convert_addr , MemCmd::DATA_READ , size , id_thread);
}


/* Record Data Write Fetch */
VOID RecordMemWrite(VOID * pc, VOID * addr, int size, int id_thread)
{
	uint64_t convert_pc = reinterpret_cast<uint64_t>(pc);
	uint64_t convert_addr = reinterpret_cast<uint64_t>(addr);

	access(convert_pc , convert_addr , MemCmd::DATA_WRITE , size , id_thread);
}



VOID Routine(RTN rtn, VOID *v)
{           
	RTN_Open(rtn);
		
	for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){

		string name = INS_Disassemble(ins);
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

    		UINT32 memOperands = INS_MemoryOperandCount(ins);	    						
	
		for (UINT32 memOp = 0; memOp < memOperands; memOp++){
			if (INS_MemoryOperandIsRead(ins, memOp))
			{
				INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				IARG_INST_PTR,
				IARG_MEMORYOP_EA, memOp,
				IARG_MEMORYREAD_SIZE,
				IARG_THREAD_ID,
				IARG_END);
			}

			if (INS_MemoryOperandIsWritten(ins, memOp))
			{
				INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				IARG_INST_PTR,
				IARG_MEMORYOP_EA, memOp,
				IARG_MEMORYREAD_SIZE,
				IARG_THREAD_ID,
				IARG_END);
			}
		}
	}
	RTN_Close(rtn);
}


VOID Fini(INT32 code, VOID *v)
{
	cout << "EXECUTION FINISHED" << endl;
	
	my_system->finishSimu();
	accessfile << mem_accesses_interval << endl;
	my_system->printinterval(miss_rate, mpki, icount % interval, prefetch_issued, prefetch_hits); //print to miss rate file
	miss_rate.close(); //close the miss rate file
	mpki.close(); //clode the mpki dump file
	accessfile.close(); //close the accesses per interval dump file
	prefetch_issued.close(); //close the prefetch issued this interval file
	prefetch_hits.close(); //close the prefetch issued hits this interval
	config_file.open(CONFIG_FILE);
	my_system->printConfig(config_file);
	config_file.close();

	output_file.open(OUTPUT_FILE);
	output_file << "Execution finished" << endl;
	output_file << "NB Access handled " << cpt_time << endl;
	output_file << "NB insts handled " << icount << endl;
	output_file << "last interval " << icount %interval  << endl;
	output_file << "Printing results : " << endl;
	my_system->printResults(output_file);
	output_file.close();
	DPRINTF(DebugHierarchy, "WRITING RESULTS FINISHED\n");
	
	delete my_system;

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
int Usage()
{
	 cout << "Usage:" << endl;
  cout << " <cache size> <associativity> <prefetch (yes/no)> <number of memory accesses>"<< endl;
	return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
//	int counter;
	if (strcmp(argv[5],"-h")== 0)
				{
					 return Usage();
					 goto end;
				}
	PIN_InitSymbols();
	PIN_Init(argc, argv);
	mem_accesses_interval = 0;
	//if (PIN_Init(argc, argv)) return Usage();
/*	std::printf("Program Name Is: %s",argv[0]);
    if(argc==1)
        std::printf("\nNo Extra Command Line Argument Passed Other Than Program Name");
    if(argc>=2)
    {
        std::printf("\nNumber Of Arguments Passed: %d",argc);
        std::printf("\n----Following Are The Command Line Arguments Passed----");
        for(counter=0;counter<argc;counter++)
            std::printf("\nargv[%d]: %s",counter,argv[counter]);
    }
*/
	int readarg;
	int policytouse;
	sscanf(argv[5],"%d",&readarg);
	simu_parameters.size = readarg;

	sscanf(argv[6],"%d",&readarg);
	simu_parameters.assoc = readarg;

	sscanf(argv[7],"%d",&readarg);
	if (readarg)
		simu_parameters.enablePrefetch = true;
	else
		simu_parameters.enablePrefetch = false;
	
	sscanf(argv[8],"%llu",&num_accesses);
	sscanf(argv[9],"%d",&readarg);
	policytouse = readarg;
	PIN_InitLock(&lock);
	cpt_time = 0;
	start_debug = 1;
	
	init_default_parameters();
	if (policytouse == 1)
		my_system = new Hierarchy("RRIP");
	if (policytouse == 0)
		my_system = new Hierarchy("LRU");
	my_system->stopWarmup();
	miss_rate.open(MISS_RATE); //open the miss rate file
	mpki.open(MPKI); //open the periodic mpki dump file
	accessfile.open(ACCESSES); //open the accesses per interval dump
	prefetch_issued.open(pre_issue);
	prefetch_hits.open(pre_hits);
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);
	// Never returns
	PIN_StartProgram();
end:
	return 0;
}
