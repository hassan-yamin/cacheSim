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

//ofstream log_file;
ofstream output_file;
ofstream config_file;
ofstream miss_rate;
PIN_LOCK lock;

bool startInstruFlag;

// This function is called before every instruction is executed
inline VOID docount() { 
	icount++; 
	if (icount % interval == 0)
		my_system->printinterval(miss_rate);
	
/*	if (icount == 1000000000)
		
		PIN_ExitApplication(0);*/
	}




VOID access(uint64_t pc , uint64_t addr, MemCmd type, int size, int id_thread)
{
	PIN_GetLock(&lock, id_thread);

	Access my_access = Access(addr, size, pc , type , id_thread);
	my_access.m_idthread = 0;
	my_system->handleAccess(my_access);		
	cpt_time++;
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
	cout << "NB Access handled " << cpt_time << endl;
	cout << "NB insts handled " << icount << endl;
	cout << "last interval " << icount %interval  << endl;
	my_system->finishSimu();

	my_system->printinterval(miss_rate);
	miss_rate.close(); //close the miss rate file
	config_file.open(CONFIG_FILE);
	my_system->printConfig(config_file);
	config_file.close();

	output_file.open(OUTPUT_FILE);
	output_file << "Execution finished" << endl;
	output_file << "Printing results : " << endl;
	my_system->printResults(output_file);
	output_file.close();
	DPRINTF(DebugHierarchy, "WRITING RESULTS FINISHED\n");
	
	delete my_system;

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
//	PIN_ERROR( "This Pin tool evaluate the RO detection on a LLC \n" 
//	      + KNOB_BASE::StringKnobSummary() + "\n");
	return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
	PIN_InitSymbols();
	if (PIN_Init(argc, argv)) return Usage();
	
	PIN_InitLock(&lock);
	cpt_time = 0;
	start_debug = 1;
	
	init_default_parameters();
	my_system = new Hierarchy("LRU");
	my_system->stopWarmup();
	miss_rate.open(MISS_RATE); //open the miss rate file
	RTN_AddInstrumentFunction(Routine, 0);
	PIN_AddFiniFunction(Fini, 0);
	// Never returns
	PIN_StartProgram();

	return 0;
}
