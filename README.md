# COA_Project
 
# Multi-Core Processor Simulator

## Project Overview
This project aims to develop a simulator similar to Ripes that can simulate a multi-core environment with four processors (cores). The simulator supports the execution of RISC-V instructions and simulates the execution of the same assembly instructions across all cores.

## Features
- Simulates a 4-core processor environment.
- Each core can access a shared 4kB memory (1kB per core).
- Supports the following RISC-V instructions:
  - ADD/SUB  
  - BNE  
  - JAL (jump)  
  - LW/SW  
  - Additional instruction: AND  
- Each core contains a special-purpose, read-only register to store its core number.
- Executes instructions from an input assembly file.
- Displays final register contents and memory state post-execution.

## Meeting Minutes
### Date: 10-Feb-2025  
**Members:** Rohith, Narshima  
**Decisions:**  
- Discussed project scope and objectives.  
- Decided to use RISC-V as the base architecture.  
- Reviewed existing tools like Ripes for reference.  
**Tasks Assigned:**  
- Rohith: Research RISC-V instruction set.  
- Narshima: Explore multi-core simulation techniques.  
**Previous Tasks Accomplished:**  
- Finalized the project topic and initial requirements.  
**Other Notes:** Had tea and biscuits during the meeting.
  
### Date: 12-Feb-2025  
**Members:** Rohith, Narshima  
**Decisions:**  
- Selected the RISC-V instructions: ADD, SUB, BNE, JAL, LW, SW, and AND.  
- Decided on 4kB total memory with equal division among cores.  
- Determined that all cores will execute the same instruction stream independently.  
**Tasks Assigned:**  
- Rohith: Start designing the memory module.  
- Narshima: Work on instruction fetch and decode units.  
**Previous Tasks Accomplished:**  
- Completed exploration of RISC-V instruction set.  
**Other Notes:** Discussed potential GUI integration in future phases.  


### Date: 15-Feb-2025  
**Members:** Rohith, Narshima  
**Decisions:**  
- Finalized memory division strategy (1kB per core).  
- Decided that each instruction will execute in one clock cycle.  
- Established the use of a read-only register for storing core IDs.  
**Tasks Assigned:**  
- Rohith: Implement the memory allocation methods. *(Deadline: 18-Feb-2025)*  
- Narshima: Finalize instruction set and core architecture. *(Deadline: 18-Feb-2025)*  
**Previous Tasks Accomplished:**  
- Rohith completed the initial memory module design.  
- Narshima completed preliminary instruction fetch and decode logic.  
**Other Notes:** Had samosas for snacks.  


### Date: 20-Feb-2025  
**Members:** Rohith, Narshima  
**Decisions:**  
- Completed the final integration of the memory and instruction modules.  
- Tested the simulator using the bubble sort program on a 9-element array.  
- Verified that each core accesses its designated memory region.  
- Reviewed register contents and memory outputs for consistency.  
**Tasks Assigned:**  
- Rohith: Final review and code clean-up before submission. *(Deadline: 20-Feb-2025)*  
- Narshima: Prepare README and ensure documentation completeness. *(Deadline: 20-Feb-2025)*  
**Previous Tasks Accomplished:**  
- Rohith implemented the memory model.  
- Narshima developed the instruction execution logic and parser.  
**Other Notes:** Celebrated submission with a quick coffee break.  




## Future Work
- Extend support for GPU cores in subsequent phases.  
- Add a graphical interface for better visualization.  
- Implement single-step execution and breakpoints.  
- Expand the instruction set further.  




