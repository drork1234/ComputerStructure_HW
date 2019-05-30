/* 046267 Computer Architecture - Spring 2016 - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

/*-----------------------------------------------------------Implementation Notes------------------------------------------------------------------
In this home work assignment we are required to implement a program dependency analyzer for RAW hazards (true dependency) detection.
The analyzer consists of only a signle class named "Analyzer" which takes a program and determines the direct instruction dependencies for each
instruction and intefaces with the given API functions provided by the course staff.
---------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "dflow_calc.h"
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

#define ENTRY -1

/** class : Analyzer
This class reperesents the analyzer of the program.
Program analyzing is done by Analyzer::reset which saves the direct dependencies of each instruction in the program (can be up to two dependencies per instruction)
Direct dependencies extraction is done by Analyzer::get_inst_deps.
Dependency maximum depth is extracted by Analyzer::get_dependency_depth.
Consumer instructions extraction is done by Analyzer::get_inst_users

NOTE: Since the potential number of dependencies for each instruction is unbounded, dependency depth and dependency users are calcuated at function call and not stored in memory
*/
class Analyzer
{
	//typedefs, private:
	typedef int instruction;

	//the first member of this type is the instruction on which the first soruce register depends
	//the second member of this type is the instruction on which the second source register depends
	typedef std::pair<instruction, instruction> dependency;

	/** DependencyCmp - a function object that is used by Analyzer::reset for determining the instruction dependency
	\The class becomes a function object by operator() overloading
	*/
	class DependencyCmp
	{

	public:
		DependencyCmp(unsigned register_index) : m_reg_index(register_index) {}

		//check the destination register index of the instruction. if it matched, there's a dependency
		bool operator()(InstInfo& inst) const {
			return inst.dstIdx == m_reg_index;
		}

	private:
		unsigned m_reg_index;
	};

	/** ConsumerCmp - a function object that is used by Analyzer::get_inst_users for determining the consumer dependency
	\The class becomes a function object by operator() overloading
	*/
	class ConsumerCmp
	{
	public:
		ConsumerCmp(int inst) : m_inst_idx(inst) {}
		
		//check if the consumer register dependency depends on the instruction index return true, false otherwise
		bool operator ()(const dependency& dep) { 
			return dep.second == m_inst_idx || dep.first == m_inst_idx; 
		}

	private:
		int m_inst_idx;
	};

	
public:
	/** Analyzer - default constructor
	\return - None
	*/
	Analyzer() {}

	/** ~Analyzer - destructor
	\clears the memory held by the analyzer
	\return - None
	*/
	~Analyzer() { clear(); }

	/** clear: clear the memory allocated by the analyzer
	\return - None
	*/
	void clear(){
		//clear the dependencies
		m_direct_dependencies.clear();

		//clear the instructions
		m_instructions.clear();
	}

	/** reset: reset the analyzer with new data
	\param[in] - prog		:	a pointer to a beginning of a InsoInfo array
	\param[in] - numOfInsts	:	the size of the array from the first parameter
	\return - None
	May throw an std::bad_alloc due to allocation errors caused by std::vector<type> allocator
	*/
	void reset(InstInfo prog[], unsigned int numOfInsts){
		//parameter check
		if (NULL == prog) return;
		
		m_instructions.clear();
		m_instructions.reserve(numOfInsts);
		
		//add all of the instructions to the container
		for (size_t i = 0; i < numOfInsts; i++) 
			m_instructions.push_back(prog[i]);

		//now we anaylize the program
		//reserve enough space for each instruction to hold its direct dependencies
		m_direct_dependencies.clear();
		m_direct_dependencies.reserve(numOfInsts);
		//go from the back , for each instruction inside the instruction conatiner, find it's closest dependencies
		for (size_t i = 0; i < numOfInsts; i++){
			InstInfo& inst = m_instructions[numOfInsts - (i + 1)];

			//search from the end to the beginning for the closest dependency
			//this search is done in reverse order (due to reverse_opertor usage)
			std::vector<InstInfo>::reverse_iterator itFirst = std::find_if(m_instructions.rbegin() + (i + 1), m_instructions.rend(), DependencyCmp(inst.src1Idx)),
													itSecond = std::find_if(m_instructions.rbegin() + (i + 1), m_instructions.rend(), DependencyCmp(inst.src2Idx));

			//save the dependency
			//for each one of the two possible dependencies do:
			
			dependency dep;
			//if the iterator returned by find_if isn't the reverse end, save the index which calculates as follows:
			if (m_instructions.rend() != itFirst) 
				dep.first = (numOfInsts - 1)- distance(m_instructions.rbegin(), itFirst);

			//else there's no dependency caused by the register so save the index an ENTRY (-1)
			else dep.first = ENTRY;


			//do the same thing for the second register dependency
			if (m_instructions.rend() != itSecond)
				dep.second = (numOfInsts - 1) - distance(m_instructions.rbegin(), itSecond);

			else dep.second = ENTRY;

			//save the dependency
			m_direct_dependencies.push_back(dep);
		}

		//since the above loop was executed in reverse order (by m_instruction means) we need to reverse the dependency container
		reverse(m_direct_dependencies.begin(), m_direct_dependencies.end());
	}

	/** get_dependency_depth: returns the dependency depth of a certain instruction in the program
	\param[in] - theInst	:	The index of the instruction which is dependency depth shall be reutrned
	\return - the biggest dependency depth of the instruction. Returns -1 for invalid arguments (index under/overflow) and 0 for instructions that depend only on ENTRY.
	*/
	int get_dependency_depth(int theInst) { 
		//argument check
		if (0 > theInst || (int)m_instructions.size() <= theInst) return -1;

		//get a constant reference to the depenedency
		const dependency& dep = m_direct_dependencies[theInst];

		//get the indices of the direct dependency instructions 
		const int&	dep_inst1 = dep.first,
					dep_inst2 = dep.second;
		
		//this is the stop condition
		//if the instruction depends only on the entry return 0
		if (ENTRY == dep_inst1 && ENTRY == dep_inst2) return 0;

		//here we do the recursive calls
		//if only one instruction doesn't depend on ENTRY, return 1 + dependency depth of the direct instrucion dependency
		else if (ENTRY != dep_inst1 && ENTRY == dep_inst2) return 1 + get_dependency_depth(dep_inst1);
		else if (ENTRY == dep_inst1 && ENTRY != dep_inst2) return 1 + get_dependency_depth(dep_inst2);

		//if the both of the instructions don't depend on ENTRY return the maximum depth of the dependency instructions
		else return max(1 + get_dependency_depth(dep_inst1), 1 + get_dependency_depth(dep_inst2));
	}

	/** get_inst_deps: returns the dependency depth of a certain instruction in the program
	\param[in]		- theInst		:	The index of the instruction
	\param[inout]	- src1DepInst	:	A pointer to a dependency on which the first register depends
	\param[inout]	- src2DepInst	:	A pointer to a dependency on which the second register depends
	\return - less than 0 when an error occurs (argument are invalid - pointers are NULL or index under/overflow), 0 on success
	*/
	int get_inst_deps(unsigned int theInst, int *src1DepInst, int *src2DepInst) { 
		//argument check
		if (NULL == src1DepInst || NULL == src2DepInst) return -1;

		if (0 > theInst || m_instructions.size() <= theInst) return -1;

		//save the dependencies to the pointers
		*src1DepInst = m_direct_dependencies[theInst].first;
		*src2DepInst = m_direct_dependencies[theInst].second;

		return 0;
	}

	/** get_inst_user: writes the consumer instructions of a current instruction
	\param[in]		- theInst		:	The index of the instruction
	\param[inout]	- numUsersIn	:	The size of the array passed in parameter #3
	\param[inout]	- users			:	A pointer to an array which holds the consumer instruction indices after the execution of the method
	\return - the number of instructions that depend on the given instruction (may be greater than numUsersIn).
	\may return -1 (FAIL) when arguments are invalid (the pointer passed as #3 is NULL of index under/overflow), greater or even to 0 on success
	*/
	int get_inst_users(int theInst, unsigned int numUsersIn, unsigned int *users) { 
		//argument
		if (0 > theInst || (int)m_instructions.size() <= theInst) return -1;
		if (NULL == users) return -1;

		std::vector<dependency>::iterator it = m_direct_dependencies.begin() + theInst;
		int counter = 0;
		while (it != m_direct_dependencies.end()){
			//find the instrucion that depends on this one
			it = std::find_if(it + 1, m_direct_dependencies.end(), ConsumerCmp(theInst));

			//if found, save it
			if (m_direct_dependencies.end() != it){
				if ((unsigned)counter < numUsersIn)
					users[counter++] = distance(m_direct_dependencies.begin(), it);
				else
					counter++;
			}
		}
		return counter;
	}

private:
	/** m_instructions - random access container that holds the instructions of the program
	\type - std::vector<InstInfo> 
	*/
	std::vector<InstInfo> m_instructions;

	/** m_direct_dependencies - random access container that holds the direct dependencies of a certain instruction
	\type - std::vector<dependency>
	*/
	std::vector<dependency> m_direct_dependencies;
};

//static object the interfaces with the functions
static Analyzer __analyzer;

ProgCtx analyzeProg(InstInfo prog[], unsigned int numOfInsts) {
	try
	{
		__analyzer.reset(prog, numOfInsts);
		return static_cast<ProgCtx>(&__analyzer);
	}
	catch (...)
	{
		return PROG_CTX_NULL;
	}
    
}

void freeProgCtx(ProgCtx ctx) {
	using namespace std;
	if (NULL == ctx) return;
	
	Analyzer& analyzer = *reinterpret_cast<Analyzer*>(ctx);

	//clear the memory held by the analyzer
	analyzer.clear();
}

int getDepDepth(ProgCtx ctx, unsigned int instNum) {
	Analyzer* analyzer = reinterpret_cast<Analyzer*>(ctx);
	
	if (NULL == analyzer) return -1;

	return analyzer->get_dependency_depth(instNum);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
	Analyzer* analyzer = reinterpret_cast<Analyzer*>(ctx);

	if (NULL == analyzer) return -1;

	return analyzer->get_inst_deps(theInst, src1DepInst, src2DepInst);
}

int getInstUsers(ProgCtx ctx, unsigned int theInst, unsigned int numUsersIn, unsigned int *users) {
	Analyzer* analyzer = reinterpret_cast<Analyzer*>(ctx);

	if (NULL == analyzer) return -1;

	return analyzer->get_inst_users(theInst, numUsersIn, users);
}


