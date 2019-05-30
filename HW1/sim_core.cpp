/* 046267 Computer Architecture - Spring 2016 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
#include <vector>
#ifdef _WIN32
#else
#include <tr1/memory>
#endif

#define FLUSH_ALL -1

/*! SimCore
The main class representing a MIPS CPU that supports LOAD, STORE, ADD, SUB, BR, BREQ and BRNEQ commands
SimCore class has declaration and definition of sub-systems inside the MIPS CPU: 
	1. PipeStage - an abstract class that represents a single stage inside the pipe. The stages are:
		1. InstructionFetch
		2. InstructionDecode
		3. Execute
		4. Memory
		5. WriteBack
	2. Forward - a class that implements value forwarding from MEM/WB stage to EXE.
				 Implemented via operator() overloading.

	3. HDU - a class that implements the hazard detection unit.
			  Implemented via operator() overloading

	All of the above sub-classes do not exist on their own hence declared and defined in 'containment' notation
	and have access (via 'friend' declaration) to the SimCore owner control values and data structures 
*/
class SimCore
{
	friend class PipeStage;
	friend class Forward;
	friend class HDU;

	/*! PipeStage
	An abstract class for representing a single pipe stage in the pipeline.
	Since the operation of each pipe stage is different, the method 'Perform()' is abstarct and only implemented in the final derived class.
	Derived classes are:
		1. InstructionFetch
		2. InstructionDecode
		3. Execute
		4. Memory
		5. WriteBack

	A 'Propagate' method is also implemented in this base class but can be overriden in each derived class since every stage has it's 
	own control values.
	*/
	class PipeStage
	{
	public:
		/*! PipeStage::PipeStage: base cunstructor
		\param[in] stage This stage index inside the stages container in SimCore class
		\param[in] owner This stage owner
		*/
		PipeStage(short unsigned stage, SimCore& owner) : m_pipe_stage(stage), core_owner(owner) {}
		
		/*! PipeStage::~PipeSatge: destructor (virtual)
		*/
		virtual ~PipeStage() {}

		//abstract, each pipe operates differently
		/*! PipeStage::Perform: Executes this pipe stage unique operation (abstract)
		*/
		virtual void Perform() = 0;

		/*! PipeStage::Propagate (virtual)
		Pulls the command pc from the previous pipe stage
		Can be overriden in derived classes
		*/
		virtual void Propagate() {
			if (core_owner.m_update_flag) 
				m_curr_cmd_pc = core_owner.m_stages[m_pipe_stage - 1]->CurrentCommandPC();
			
		}

		/*! PipeStage::CurrentCommandPC
		\return current pc of the current command inside the current pipe stage
		*/
		int32_t CurrentCommandPC() const {
			return m_curr_cmd_pc;
		}

	protected:
		/*! PipeStage::m_pipe_stage
		The index inside this->core_owner stages container
		Typically, for the derived classes m_pipe_stage will be:
			InstructionFetch:	0
			InstructionDecode:	1
			Execute:			2
			Memory:				3
			WriteBack:			4
		*/
		short unsigned m_pipe_stage;

		/*! PipeStage::m_curr_cmd_pc
		The program counter of the current command in the pipe
		*/
		int32_t m_curr_cmd_pc;

		/*! PipeStage::core_owner
		A reference to a SimCore class, the owner that holds this pipe stage inside a container
		*/
		SimCore& core_owner;
	};

	/*! WriteBack (PipeStage)
	A -final- and complete class derived from PipeStage abstract class.
	Implements 'Perform' method and overrides PipeStage::Propagate
	Allowes Forward class to read it's protected values for quick propagation to EXE.
	*/
	class WriteBack : public PipeStage
	{
		friend class Forward;

	public:
		/*! WriteBack::WriteBack
		\param[in] owner This stage owner
		*/
		WriteBack(SimCore& owner) : PipeStage(SIM_PIPELINE_DEPTH - 1, owner) {}
		
		/*! WriteBack::~WriteBack (virtual)
		Destructor
		*/
		virtual ~WriteBack() {}

		/*! WriteBack::Perform (virtual)
		Implements PipeStage::Perform abstract method:
		In case of LOAD, SUB or ADD commands, write back the value the register file
		*/
		virtual void Perform() {
			
			//Get the reference to the core owner of the stage
			SimCore& this_owner = core_owner;
			
			//Get a reference to the current command at the pipe stage
			SIM_cmd& this_stage_cmd = this_owner.m_machine_state.pipeStageState[m_pipe_stage].cmd;
			
			//Get a reference to the register file of the core
			int32_t (&register_file)[SIM_REGFILE_SIZE] = this_owner.m_machine_state.regFile;

			//if the command is 'add', 'load' or 'sub', write back 
			if (CMD_ADD == this_stage_cmd.opcode || 
				CMD_LOAD == this_stage_cmd.opcode ||
				CMD_SUB == this_stage_cmd.opcode){
				//write the data back to the register file
				register_file[this_stage_cmd.dst] = m_written_data;
			}
			return;
		}

		/*! WriteBack::Propagate (virtual)
		Overrides PipeStage::Propagate virtual base method:
		In the owner can be updated (not in memory stall or hazard mode):
			1. Get the current command at the WB stage.
			2. Check WB command opcode:
				1. If the opcode is CMD_LOAD, pull the data loaded from the data memory save by MEM stage
				2. Else if the opcode is CMD_ADD or CMD_SUB, pull the data calculated from EXE stage and propagated to MEM stage

			(no need for calling PipeStage::Propagate, the current command pc is not being used here)
		*/
		virtual void Propagate() {
			//If we can update the pipe, do the following:
			if (core_owner.m_update_flag){

				//Get a reference to the core owner of this pipe stage
				SimCore& core = core_owner;

				//Get a reference to the current command data in this pipe stage
				const SIM_cmd& WB_cmd = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 1].cmd;

				//if the command is 'load' then we shall pull the loaded data from MEM stage
				if (CMD_LOAD == WB_cmd.opcode)
					m_written_data = dynamic_cast<Memory*>(core.m_stages[SIM_PIPELINE_DEPTH - 2])->m_loaded_data;

				//else if the command is 'add' or 'sub' we shall pull the ALU calculated data from MEM stage (the data propagated from EXE stage)
				else if (CMD_ADD == WB_cmd.opcode || CMD_SUB == WB_cmd.opcode)
					m_written_data = dynamic_cast<Memory*>(core.m_stages[SIM_PIPELINE_DEPTH - 2])->m_EXE_calculations.EXE_calculation;

				//else do nothing
			}
			
		}

		/*! WriteBack::WrittenData
		\return m_written_data	The value of the protected field of WriteBack class, 
								holds the value of the current field to be written to the register file if 
								the current command in WB is LOAD, ADD or SUB
								
		*/
		const int32_t WrittenData() const { return m_written_data; }

	protected:
		/*! WriteBack::m_written_data
		The value of the protected field of WriteBack class,
		holds the value of the current field to be written to the register file if
		the current command in WB is LOAD, ADD or SUB
		*/
		int32_t m_written_data;
	};

	/*! Memory (PipeStage)
	A -final- and complete class derived from PipeStage abstract class.
	Implements 'Perform' method and overrides PipeStage::Propagate
	*/
	class Memory : public PipeStage
	{
		friend class WriteBack;
		friend class SimCore;
		friend class Forward;

	public:
		/*! Memory::Memory
		\param[in] owner This stage owner
		*/
		Memory(SimCore& owner) : PipeStage(SIM_PIPELINE_DEPTH - 2, owner) {
			m_EXE_calculations.EXE_calculation = 0;
			m_EXE_calculations.EXE_is_branch = false;
		}

		/*! Memory::~Memory (virtual)
		Destructor
		*/
		virtual ~Memory() {}

		/*! Memory::Perform (virtual)
		Implements PipeStage::Perform abstract method:
		Operates according to the propagated values and command opcode from EXE stage:
			1.	If opcode is BR, BREQ or BRNEQ and the branch condition flag is true, return and 
				SimCore::UpdateMachineState will handle the branch
			
			2.	Else if opcode us LOAD, get the address of the value to be read from the memory and try to load the data.
				If the call to SIM_MemDataRead failed, reset the core owner's update flag, a call to the next SimCore::UpdateMachineState will not update the core state,
				and Memory::Perform will be called until SIM_MemDataRead will succeed,
				Else set the core owner's update flag and the machine will update as normal

			3. Else if opcode is STORE, get the address of memory where the value calculated by EXE will be written, and write the value
		*/
		virtual void Perform() {

			SimCore& core = core_owner;
			const SIM_cmd& MEM_cmd = core.m_machine_state.pipeStageState[m_pipe_stage].cmd;

			//Branch machine update is handled by SimCore::UpdateMachineState
			if ((CMD_BR == MEM_cmd.opcode || CMD_BREQ == MEM_cmd.opcode || CMD_BRNEQ == MEM_cmd.opcode) && m_EXE_calculations.EXE_is_branch) {
				return;
			}

			//Memory stall machine update is handled by SimCore::UpdateMachineState
			//Here only change the core owner flags
			else if (CMD_LOAD == MEM_cmd.opcode){
				//calculate the target address
				int32_t addr = m_EXE_calculations.EXE_calculation;
				if (0 > SIM_MemDataRead(addr, &m_loaded_data)){
					//memory stall hence:
					//halt the execution of the pipeline
					core.m_update_flag = false;
					return;
				}
				//memory access granted and the value has been loaded, so the machine can continue execution
				else core.m_update_flag = true;
			}

			//If the command is STORE, load the data according the appropriate address
			else if (CMD_STORE == MEM_cmd.opcode){
				//calculate store address
				//not corrent, read values from execute stage
				int32_t addr = m_EXE_calculations.EXE_calculation;
				//write the data to memory
				int32_t src1Val = core.m_machine_state.pipeStageState[m_pipe_stage].src1Val;
				SIM_MemDataWrite(addr, src1Val);
			}

			//if the command is not one of the three kinds of branch, put the the flag
			m_EXE_calculations.EXE_is_branch = false;
			
		}

		/*! Memory::Propagate (virtual)
		Overrides PipeStage::Propagate virtual base method:
		If the owner can be updated (not in memory stall or hazard mode):
			1. Get a pointer to the Execute stage.
			2. Get the values (EXE calculated value, EXE branch control value) from EXE stage
			3. Propagate the command pc from EXE (by a call to PipeStage::Propagate
		*/
		virtual void Propagate() {
			if (core_owner.m_update_flag){
				SimCore::Execute* EXE_pipe_stage = dynamic_cast<Execute*>(core_owner.m_stages[m_pipe_stage - 1]);
				m_EXE_calculations.EXE_is_branch = EXE_pipe_stage->mf_is_branch;
				m_EXE_calculations.EXE_calculation = EXE_pipe_stage->m_calculated_data;

				PipeStage::Propagate();
				return;
			}
			
		}

	private:
		/*! Memory::m_loaded_data
		The loaded value from the data memory
		*/
		int32_t m_loaded_data;

		/*! Memory::m_EXE_calculations
		An unnamed struct that holds the EXE calculation propagated from EXE stage via Memory::Propagate
		*/
		struct {	int32_t EXE_calculation; 
					bool EXE_is_branch; } m_EXE_calculations;
	};

	/*! Execute (PipeStage)
	A -final- and complete class derived from PipeStage abstract class.
	Implements 'Perform' method and overrides PipeStage::Propagate
	*/
	class Execute : public PipeStage
	{
		friend class SimCore;
		friend class Forward;

	public:
		/*! Execute::Execute
		\param[in] owner This stage owner
		*/
		Execute(SimCore& owner) : mf_is_branch(false), m_calculated_data(0), PipeStage(SIM_PIPELINE_DEPTH - 3, owner) {}
		
		/*! Execute::~Execute (virtual)
		Destructor
		*/
		virtual ~Execute() {}

		/*! Execute::Perform (virtual)
		Implements PipeStage::Perform abstract method:
		1. Forward values
		2. Get a reference to EXE command struct and src's values
		3. Operate according to the command
		*/
		virtual void Perform() {

			SimCore& core = core_owner;
			
			//forward values 
			core.m_forwarding_unit();
			
			SIM_cmd &EXE_cmd = core.m_machine_state.pipeStageState[m_pipe_stage].cmd;
			int32_t &EXE_srcVal1 = core.m_machine_state.pipeStageState[m_pipe_stage].src1Val, &EXE_srcVal2 = core.m_machine_state.pipeStageState[m_pipe_stage].src2Val;

			switch (EXE_cmd.opcode)
			{
			case CMD_ADD:	{	
							m_calculated_data = EXE_srcVal1 + EXE_srcVal2; }
							break;
			
			case CMD_SUB:	{	
							m_calculated_data = EXE_srcVal1 - EXE_srcVal2; }
							break;

			case CMD_BR:	{	
							mf_is_branch = true;
							m_calculated_data = m_current_dst_data + m_curr_cmd_pc; }
							break;
			
			case CMD_BREQ:	{
							mf_is_branch = (EXE_srcVal1 == EXE_srcVal2);
							m_calculated_data = m_current_dst_data + m_curr_cmd_pc; }
							break;

			case CMD_BRNEQ: {
							mf_is_branch = !(EXE_srcVal1 == EXE_srcVal2);
							m_calculated_data = m_current_dst_data + m_curr_cmd_pc; }
							break;

			case CMD_LOAD:	{
							m_calculated_data = EXE_srcVal1 + (EXE_cmd.isSrc2Imm ? EXE_cmd.src2 : EXE_srcVal2); }
							break;
			
			case CMD_STORE:	{
							m_calculated_data = m_current_dst_data + (EXE_cmd.isSrc2Imm ? EXE_cmd.src2 : EXE_srcVal2); }
							break;
			default:
				break;
			}
		}

		/*! Execute::Propagate (virtual)
		Overrides PipeStage::Propagate virtual base method:
		If the machine can be updated:
			1. Get a pointer to the InstructionDecode stage
			2. Get the destination value from ID stage
			3. Get the current command pc (via PipeStage::Propagate)
		*/
		virtual void Propagate() {
			if (core_owner.m_update_flag) {
				SimCore::InstructionDecode* ID_pipe_stage = dynamic_cast<InstructionDecode*>(core_owner.m_stages[m_pipe_stage - 1]);
			
				//get the dst value from the register value, which isn't propagated with the SIM_coreState (only the dst index is propagated)
				m_current_dst_data = ID_pipe_stage->m_dst_value;

				PipeStage::Propagate();
				return;
			}
			
		}

	private:
		/*! Execute::mf_is_branch
		A flag control of branch condition
		Calculated via Execute:::Perform
		*/
		bool mf_is_branch;

		/*! Execute::m_calculated_data
		Value calculated by this stage
		Calculated via Execute::Perform
		*/
		int32_t m_calculated_data;

		/*! Execute::m_current_dst_data
		Current destination value propagated from ID stage
		*/
		int32_t m_current_dst_data;
	};

	/*! InstructionDecode (PipeStage)
	A -final- and complete class derived from PipeStage abstract class.
	Implements 'Perform' method.
	*/
	class InstructionDecode : public PipeStage
	{
		friend class SimCore;

	public:
		/*! InstructionDecode::InstructionDecode
		\param[in] owner This stage owner
		*/
		InstructionDecode(SimCore& owner) : PipeStage(SIM_PIPELINE_DEPTH - 4, owner) {}
		
		/*! InstructionDecode::~InstructionDecode (virtual)
		Destructor
		*/
		virtual ~InstructionDecode() {}

		/*! InstructionDecode::Perform (virtual)
		Implements PipeStage::Perform abstract method:
		1. Get a reference the ID command struct 
		2. Get the destination value from the register file
		3. Get the values of src1 and src2 value from the register file (in case src2 is immediate put it in src2Val instead of the value from the register file)
		*/
		virtual void Perform() {
			
			SimCore& core = core_owner;
			const SIM_cmd& ID_cmd = core.m_machine_state.pipeStageState[m_pipe_stage].cmd;
			int32_t &ID_src1Val = core.m_machine_state.pipeStageState[m_pipe_stage].src1Val, &ID_src2Val = core.m_machine_state.pipeStageState[m_pipe_stage].src2Val;

			//save the destination register for EXE and MEM usage
			m_dst_value = core.m_machine_state.regFile[ID_cmd.dst];

			//update the propagated struct's src1 and src2 values
			ID_src1Val = core.m_machine_state.regFile[ID_cmd.src1];
			
			//update src2Val with appropriate values: the value of the src2 if src2 is immidiete and regs[ID_cmd.src2] otherwise
			ID_src2Val = (ID_cmd.isSrc2Imm ? ID_cmd.src2 : core.m_machine_state.regFile[ID_cmd.src2]);

		}

		//notice that InstructionDecode doesn't override PipeStage::Propagate
	private:
		/*! InstructionDecode::m_dst_value (virtual)
		Implements PipeStage::Perform abstract method:
		Value of the destination register
		*/
		int32_t m_dst_value;
	};

	/*! InstructionFetch (PipeStage)
	A -final- and complete class derived from PipeStage abstract class.
	Implements 'Perform' method and overrides PipeStage::Propagate
	*/
	class InstructionFetch : public PipeStage
	{
		friend class Memory;
	public:
		/*! InstructionFetch::InstructionFetch
		\param[in] owner This stage owner
		*/
		InstructionFetch(SimCore& owner) : PipeStage(0 ,owner) {}
		
		/*! InstructionFetch::~InstructionFetch (virtual)
		Destructor
		*/
		virtual ~InstructionFetch() {}

		/*! InstructionDecode::Perform (virtual)
		Implements PipeStage::Perform abstract method:
		Essentially does nothing
		*/
		virtual void Perform() {	
		}

		/*! InstructionFetch::Propagate (virtual)
		Overrides PipeStage::Propagate virtual base method:
		Since PipeStage::Propagate pulls the values from the previous stage, 
		trying to invoke PipeStage::Propagte under this class will cause a memory access violation, so it has to overriden

		Load the next command from the instruction memory
		*/
		virtual void Propagate() {
			/*	PipeStage::Propagate pulls m_curr_cmd_pc from the prev PipeStage
				Since IF is at stage 0, invoking PipeStage::Propagate will cause a memory access violation
				so we have to override PipeStage::Propagate, so it will read from the instruction memory
			*/
			SimCore& core = core_owner;
			SIM_cmd& IF_cmd = core.m_machine_state.pipeStageState[0].cmd;
			SIM_MemInstRead(core.m_machine_state.pc, &IF_cmd);
			m_curr_cmd_pc = core.m_machine_state.pc;
			;
		}
	};

	/*! Forward
	A class that represents a forwarding unit.
	Implements value forwarding via operator() overloading
	*/
	class Forward
	{

	public:
		/*! Forward::Forward
		\param[in] owner This stage owner
		*/
		Forward(SimCore& owner) : m_core_owner(owner) {}

		/*! Forward::~Forward
		Destructor
		*/
		~Forward() {}
		
		//allways, ALLWAYS!!!, check MEM stage BEFORE WB stage
		
		/*! Forward::operator()
		Overload operator() for this class, here we implement the forwarding algorithm as follows:
			1. Get references to EXE stage srcs' and dst's values
			2. Get references to EXE srcs' and dst's and indices (register indicators)
			3. Get references to MEM and WB dst's indices
			4. Get references to WB, MEM and EXE SIM_cmd struct

			The following is relevant for src1 and src2:
				If MEM command opcode is ADD or SUB and EXE_src register equals MEM dst register then assign the value of EXE calculation
				that was propagated to MEM stage to EXEsrcVal
				
				Else if WB command opcode is SUB or ADD or LOAD and EXE_src register equals WB dst register then assign the written value of WB stage to EXEsrcVal
				

			The following is relevant for dst:
				If EXE command opcode is BR or BREQ or BRNEQ or STORE and MEM command opcode is SUB or ADD and EXEdst register equals MEMdst register
				assign EXEdstVal with MEM calculated data

				Else if EXE command opcode is BR or BREQ or BRNEQ or STORE and WB command opcode is SUB or ADD or LOAD and EXEdst register equals WBdst register
				assign written value of WB to EXEdstVal
			
			(NOTE: if MEM command opcode is LOAD and we have to forward values we have a hazard, this is detected by HDU and not dealt with here)
		*/
		void operator ()() {
			SimCore& core = m_core_owner;
			
			//get the values of EXE src1, src2 and dst
			int32_t &EXEsrc1Val = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].src1Val,
					&EXEsrc2Val = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].src2Val,
					&EXEdstVal	= dynamic_cast<Execute*>(core.m_stages[SIM_PIPELINE_DEPTH - 3])->m_current_dst_data;
					
			//get the indices of EXE, MEM and WB
			const int32_t	&EXEsrc1Index	= core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].cmd.src1,
							&EXEsrc2Index	= core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].cmd.src2,
							&EXEdstIndex	= core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].cmd.dst,
							&MEMdstIndex	= core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 2].cmd.dst,
							&WBdstIndex		= core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 1].cmd.dst;
			
			//get the commands in the pipe of EXE and MEM
			SIM_cmd const &MEM_cmd = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 2].cmd;
			SIM_cmd const &EXE_cmd = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].cmd;
			SIM_cmd const &WB_cmd = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 1].cmd;
			const bool is_MEMsrc2IsImm = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 2].cmd.isSrc2Imm;

			//take care of src1
			//not checking CMD_LOAD for MEM stage because that means there's a hazard in the pipe
			if ((MEM_cmd.opcode == CMD_ADD || MEM_cmd.opcode == CMD_SUB) && EXEsrc1Index == MEMdstIndex)
				EXEsrc1Val = dynamic_cast<Memory*>(core.m_stages[SIM_PIPELINE_DEPTH - 2])->m_EXE_calculations.EXE_calculation;

			else if ((WB_cmd.opcode == CMD_LOAD || WB_cmd.opcode == CMD_ADD || WB_cmd.opcode == CMD_SUB) && EXEsrc1Index == WBdstIndex)
				EXEsrc1Val = dynamic_cast<WriteBack*>(core.m_stages[SIM_PIPELINE_DEPTH - 1])->WrittenData();

			//take care of src2
			if (!EXE_cmd.isSrc2Imm){
				if ((MEM_cmd.opcode == CMD_ADD || MEM_cmd.opcode == CMD_SUB) && EXEsrc2Index == MEMdstIndex )
					EXEsrc2Val = dynamic_cast<Memory*>(core.m_stages[SIM_PIPELINE_DEPTH - 2])->m_EXE_calculations.EXE_calculation;

				else if ((WB_cmd.opcode == CMD_LOAD || WB_cmd.opcode == CMD_ADD || WB_cmd.opcode == CMD_SUB) && EXEsrc2Index == WBdstIndex)
					EXEsrc2Val = dynamic_cast<WriteBack*>(core.m_stages[SIM_PIPELINE_DEPTH - 1])->WrittenData();
			}

			//take care of dst value
			if ((EXE_cmd.opcode == CMD_BR || EXE_cmd.opcode == CMD_BREQ || EXE_cmd.opcode == CMD_BRNEQ || EXE_cmd.opcode == CMD_STORE) && 
				(MEM_cmd.opcode == CMD_ADD || MEM_cmd.opcode == CMD_SUB) && 
				EXEdstIndex == MEMdstIndex){
				
				dynamic_cast<Execute*>(core.m_stages[SIM_PIPELINE_DEPTH - 3])->m_current_dst_data = 
					dynamic_cast<Memory*>(core.m_stages[SIM_PIPELINE_DEPTH - 2])->m_EXE_calculations.EXE_calculation;
			}
			
			else if ((EXE_cmd.opcode == CMD_BR || EXE_cmd.opcode == CMD_BREQ || EXE_cmd.opcode == CMD_BRNEQ || EXE_cmd.opcode == CMD_STORE) &&
					 (WB_cmd.opcode == CMD_LOAD || WB_cmd.opcode == CMD_ADD || WB_cmd.opcode == CMD_SUB) && 
					 WBdstIndex == EXEdstIndex){

				dynamic_cast<Execute*>(core.m_stages[SIM_PIPELINE_DEPTH - 3])->m_current_dst_data =
					dynamic_cast<WriteBack*>(core.m_stages[SIM_PIPELINE_DEPTH - 1])->WrittenData();
			}
				
		}

	private:
		/*! Forward::m_core_owner
		A reference to a SimCore class, the owner that holds this forwaring unit
		*/
		SimCore& m_core_owner;
	};

	/*! HDU
	A class that represents a hazard detection unit.
	Implements hazard detection via operator() overloading
	*/
	class HDU
	{
	public:
		/*! HDU::HDU
		\param[in] owner This unit owner
		*/
		HDU(SimCore& owner) : m_core_owner(owner) {}
		
		/*! HDU::~HDU
		Destructor
		*/
		~HDU() {}

		/*! HDU::operator()
		Overload operator() for this class, here we implement the hazard detection algorithm as follows:
			1.	Get a reference to EXE and ID command struct
			2.	If EXE command opcode is LOAD
					If ID command opcode is not NOP nor BR and EXE destination register equals ID src1 register or ID src2 register then return true

					Else If ID command opcode is BR or BREQ or BRNEQ and ID destination regsiter equals EXE destination register then reutrn true
					
					Else return false

				Else return false (EXE command is not LOAD)
		\return true if there's a hazard the pipe, false otherwise
		*/
		bool operator()() {
			SimCore& core = m_core_owner;
			//if there's a load command in EXE and a dependent command on ID, raise a flag, next clock cycle will insert NOP in EXE
			SIM_cmd const& EXE_cmd = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].cmd;
			SIM_cmd	const& ID_cmd = core.m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 4].cmd;
			
			//the only way that there's a hazard is when EXE stage opcode is CMD_LOAD
			if (EXE_cmd.opcode == CMD_LOAD) {
			
				//if ID opcode is not NOP or BR (check src1 and src2 indices)
				if (!(ID_cmd.opcode == CMD_NOP || ID_cmd.opcode == CMD_BR) && 
					(EXE_cmd.dst == ID_cmd.src1 || EXE_cmd.dst == ID_cmd.src2)) {
					return true;
				}

				else if ((ID_cmd.opcode == CMD_BR || ID_cmd.opcode == CMD_BREQ || ID_cmd.opcode == CMD_BRNEQ) &&
						 ID_cmd.dst == EXE_cmd.dst){
						return true;
				}

				return false;
			}

			//else EXE opcode is not CMD_LOAD, return false
			
			return false;
		}

	private:
		/*! Forward::m_core_owner
		A reference to a SimCore class, the owner that holds this hazard detection unit
		*/
		SimCore& m_core_owner;
	};

public:
	/*! SimCore::Simcore
	Allocates enough space for the container to hold 5 pipe stages, and reset all flags
	*/
	SimCore() : m_update_flag(true), m_forwarding_unit(*this), m_hazard_detection_unit(*this), mf_is_hazard(false){
		m_stages.resize(SIM_PIPELINE_DEPTH);
		m_stages[0] = new InstructionFetch(*this);
		m_stages[1] = new InstructionDecode(*this);
		m_stages[2] = new Execute(*this);
		m_stages[3] = new Memory(*this);
		m_stages[4] = new WriteBack(*this);
	}

	/*! SimCore::~Simcore
	Delete all pipe stage pointers and release the memory
	*/
	~SimCore() {
		for (size_t i = 0; i < m_stages.size(); i++)
			delete m_stages[i];

		m_stages.clear();
	}

	/*! SimCore::GetMachineState
	\return SIM_coreState machine state struct
	*/
	SIM_coreState & GetMachineState() {
		return this->m_machine_state;
	}

	/*! SimCore::Operate
	Implements the pipe operation as follows:
		1.	Perform WB stage
		2.	If the machine can be updated (a.k.a there's no memory stall in the pipe) then 
				for all pipe stage from Memory downto Fetch (WB has already perform) invoke Perform() member function
			
			Else (there's a memory stall) 
				flush WB stage and operate on MEM stage again

		NOTE: Two phase register read-write is implemented by first operate on WB stage and only then on ID stage
	*/
	void Operate() {
		//all stages operate in parallel, although WB stage has to write first to the register file before decode stage gets the values
		m_stages[SIM_PIPELINE_DEPTH - 1]->Perform();
		//if the memory read hasn't stalled, execute all pipeline stages
		if (m_update_flag){
			for (size_t i = SIM_PIPELINE_DEPTH - 1; i > 0; i--)
				m_stages[i - 1]->Perform();
		}
		//else execute only MEM stage
		else {
			Flush(SIM_PIPELINE_DEPTH - 1);
			m_stages[SIM_PIPELINE_DEPTH - 2]->Perform();
		}
	}

	/*! SimCore::Flush
	Flush a pipe stage according to an index.
	\param[in] stage Index of a stage to be flushed. If FLUSH_ALL is passed, flush the whole pipe
	*/
	void Flush(int stage = FLUSH_ALL) {
		if (FLUSH_ALL == stage)
			memset(m_machine_state.pipeStageState, 0x0, SIM_PIPELINE_DEPTH * sizeof(m_machine_state.pipeStageState[0]));

		else memset(&m_machine_state.pipeStageState[stage], 0x0, sizeof(m_machine_state.pipeStageState[0]));

	}

	/*! SimCore::Flush
	Flush the pipe from IF until a certain pipe stage
	\param[in] stage Index of an end stage to be flushed.
	*/
	void FlushUntil(int stage) {
		//check for memory access violation
		if (stage < 0 || stage >= SIM_PIPELINE_DEPTH) return;
		

		memset(m_machine_state.pipeStageState, 0x0, (stage) * sizeof(m_machine_state.pipeStageState[0]));
		return;
	}

	/*! SimCore::UpdateMachineState
	Updates the machine state according to the control values, as follows:
	
	If the machine can be updated:
		If we have a hazard in the pipe and we don't branch:
			1. Propagate EXE and MEM struct values to MEM and WB accordingly
			2. Invoke WB, MEM, EXE Propagate
			3. Insert a NOP instruction in EXE stage and reset the hazard flag

		Else 
			1. If there's a branch:
				1.	Flush all stages until (including) EXE stage
				2.	Set the program counter with the address calculated from EXE stage
				3.	Reset the branch flag

			2. Update the program counter (pc += 4)
			3. Update all the stages in SIM_coreState struct and fetch another instruction from the instruction memory
			4. Propagate all the values in the stages
			5. Invoke the hazard detection unit and save the result in the hazard flag
			6. Set the update flag
			
	*/
	void UpdateMachineState() {
		typedef struct {
			SIM_cmd cmd;
			int32_t src1Val;
			int32_t src2Val;
		} PipeStage;


		if (m_update_flag ){
			//Branch resolution only occurs in MEM stage, so check it's branch flag
			bool& is_branch = dynamic_cast<Memory*>(m_stages[SIM_PIPELINE_DEPTH - 2])->m_EXE_calculations.EXE_is_branch;
				
			if (mf_is_hazard && !is_branch){
				//save EXE stage in a temporary struct
				PipeStage this_stage_dat = { m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].cmd,
					m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].src1Val,
					m_machine_state.pipeStageState[SIM_PIPELINE_DEPTH - 3].src2Val };

				for (size_t i = SIM_PIPELINE_DEPTH - 3; i < SIM_PIPELINE_DEPTH - 1; i++) {
					PipeStage next_stage_dat = { m_machine_state.pipeStageState[i + 1].cmd,
						m_machine_state.pipeStageState[i + 1].src1Val,
						m_machine_state.pipeStageState[i + 1].src2Val };

					m_machine_state.pipeStageState[i + 1].cmd = this_stage_dat.cmd;
					m_machine_state.pipeStageState[i + 1].src1Val = this_stage_dat.src1Val;
					m_machine_state.pipeStageState[i + 1].src2Val = this_stage_dat.src2Val;


					this_stage_dat = next_stage_dat;
				}

					//propagate calculated data from MEM stage to WB stage
				m_stages[SIM_PIPELINE_DEPTH - 1]->Propagate();

					//update WB and MEM values with last MEM and EXE values
				for (size_t i = SIM_PIPELINE_DEPTH - 1; i > SIM_PIPELINE_DEPTH - 3; i--)
					m_stages[i]->Propagate();

				Flush(SIM_PIPELINE_DEPTH - 3);

				mf_is_hazard = false;
			}

			else{
				if (is_branch) {
					FlushUntil(SIM_PIPELINE_DEPTH - 2);

					//use the old values of memory stage to set the pc before it's updated
					SetProgramCounter(dynamic_cast<Memory*>(m_stages[SIM_PIPELINE_DEPTH - 2])->m_EXE_calculations.EXE_calculation);
						
					//put down the flag
					is_branch = false;
				}
					//update the machine
				PipeStage this_stage_dat = { m_machine_state.pipeStageState[0].cmd,
					m_machine_state.pipeStageState[0].src1Val,
					m_machine_state.pipeStageState[0].src2Val };

				for (size_t i = 0; i < SIM_PIPELINE_DEPTH - 1; i++) {
					PipeStage next_stage_dat = { m_machine_state.pipeStageState[i + 1].cmd,
						m_machine_state.pipeStageState[i + 1].src1Val,
						m_machine_state.pipeStageState[i + 1].src2Val };

					m_machine_state.pipeStageState[i + 1].cmd = this_stage_dat.cmd;
					m_machine_state.pipeStageState[i + 1].src1Val = this_stage_dat.src1Val;
					m_machine_state.pipeStageState[i + 1].src2Val = this_stage_dat.src2Val;


					this_stage_dat = next_stage_dat;
				}

					//update the program counter
				UpdateProgramCounter();

					//read another command from the instructon memory
				SIM_MemInstRead(m_machine_state.pc, &m_machine_state.pipeStageState[0].cmd);


				for (size_t i = SIM_PIPELINE_DEPTH; i > 0; i--)
					m_stages[i - 1]->Propagate();

				//detect hazards
				//a hazard is detected only if there's a load dependency in ID-EXE stages
				//The next update routine will handle the appropriate propagation
				mf_is_hazard = m_hazard_detection_unit();
			}
			m_update_flag = true;
		}

			//there's a memory stall, Flush WB stage
		else{
			Flush(SIM_PIPELINE_DEPTH - 1);
			return;
		}
	}
	
	/*! SimCore::UpdateProgramCounter
	If the machine can be updated, increase the pc by 4.
	*/
	void UpdateProgramCounter() {
		if (m_update_flag) m_machine_state.pc += 4;
	}

	/*! SimCore::SetProgramCounter
	Sets the program counter to a new pc;
	\param[in] new_pc The new pc to be assigned
	*/
	void SetProgramCounter(int32_t new_pc) {
		m_machine_state.pc = new_pc;
	}

private:
	/*! SimCore::m_stages
	A container that holds 5 pointers represting the PipeStages.
	Since PipeStage is abstract the pointers should point to one of PipeStage's derived classes
	The order of the data inside the container is as follows:
	0. InstructionFetch*
	1. InstructionDecode*
	2. Execute*
	3. Memory*
	4. WriteBack*

	The need for polymorphism arises at SimCore::UpdateMachineState and SimCore::Operate where all the stages operate and update one after another under a loop,
	and virtual functions come-in handy.
	*/
	std::vector<PipeStage*> m_stages;

	/*! SimCore::m_forwardin_unit
	Forwarding unit held by this core
	*/
	Forward m_forwarding_unit;

	/*! SimCore::m_hazard_detection_unit
	Hazard detection unit held by this core
	*/
	HDU m_hazard_detection_unit;

	/*! SimCore::m_machine_state
	SIM_coreState struct that holds all the SIM_cmd and srcs' values of all the 5 pipe-stages
	*/
	SIM_coreState m_machine_state;

	/*! SimCore::mf_is_hazard
	A flag that indicates a hazard in the pipe.
	*/
	bool mf_is_hazard;
	
	/*! SimCore::m_update_flag
	A flag that indicates if the machine can be updated. If false, this means we have a memory stall in the pipe.
	*/
	bool m_update_flag;
};

/*! machine_core
Static object representing the simulator
*/
static SimCore machine_core;



int SIM_CoreReset(void)
{
	SIM_coreState & machine_state =  machine_core.GetMachineState();
	void* res = memset((void*)&machine_state, 0x0, sizeof(SIM_coreState));
	SIM_MemInstRead(machine_state.pc, &machine_state.pipeStageState[0].cmd);
	return res != NULL ? 0 : -1;
}

void SIM_CoreClkTick(void)
{
	machine_core.UpdateMachineState();
	machine_core.Operate();
}

void SIM_CoreGetState(SIM_coreState *curState)
{    
	if (NULL != curState){
		const SIM_coreState& machine_state = machine_core.GetMachineState();
		memcpy(curState, &machine_state, sizeof(SIM_coreState));
	}

	return;
}
