 # Roll No 21 Shouryanga parvam
 ## Team Members
 ### M.Rohith Venkat     CS23B032
 ### L.Narshima          CS23B028
# Multi-Core Processor Simulator

## Project Overview
This project aims to develop a simulator similar to Ripes that can simulate a multi-core environment with four processors (cores). The simulator supports the execution of RISC-V instructions and simulates the execution of the same assembly instructions across all cores.
## How To Use Our Simulator 
- Downlaod the Zip file that was present there .
- Please Extract That File .
- You Can find simulator.cpp.
- Please Run The File .
- For The input of the simulator I have a written a assembly code for bubble sort name as (in.asm) it contains around 150 lines of comands .
- After sucessfully running the code You get see the output in the memory states .
- Then After please Run run the draw.py then you can get to see the output in the memory in the by using the Graph.
- Here I am using Python for Plotting because CPP doesnt contain any Matplotlib .
- If want To use it You should want to import some Python files That's i have used python for only Plotting .
- I belive that my assembly code for the bubble sort taking minimum no of clock cycles .
- Which makes the CORE more optimized .
- This is the End for the Project phase 1 .
- Regards Team Shouryanga Parvam .
- Expecting the Best Output .

## Features
- Simulates a 4-core processor environment.
- Each core can access a shared 4kB memory (1kB per core).
- Supports the following RISC-V instructions:
  - ADD/SUB  
  - BNE
  - BLE (This was main thing we have used in writing the assembly code)
  - JAL (jump)  
  - LW/SW  
  - Additional instruction: J 
- Each core contains a special-purpose, read-only register to store its core number.
- Executes instructions from an input assembly file.
- Displays final register contents and memory state post-execution.

## Meeting Minutes
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
  








## Future Work
- Extend support for GPU cores in subsequent phases.  

- Implement single-step execution and breakpoints.  
- Expand the instruction set further.  




