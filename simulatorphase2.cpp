#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <queue>
#include <deque>

using namespace std;

// Pipeline stages
enum PipelineStage {
    NONE = -1,
    IF = 0,   // Instruction Fetch
    ID = 1,   // Instruction Decode
    EX = 2,   // Execute
    MEM = 3,  // Memory Access
    WB = 4    // Write Back
};

struct Instruction {
    string opcode;
    int dest_reg = -1;    // Destination register (rd)
    int src_reg1 = -1;    // Source register 1 (rs1)
    int src_reg2 = -1;    // Source register 2 (rs2)
    int imm = 0;          // Immediate value
    int mem_addr = -1;    // Memory address
    string label;         // Branch/jump label
    int issue_cycle = -1; // Cycle when instruction was issued
    
    // Pipeline tracking information
    PipelineStage current_stage = NONE;
    int stage_complete_cycle[5] = {0}; // Cycle when each stage completes
    bool completed = false;            // Instruction has finished execution
    int result_value = 0;              // Result of computation (for forwarding)
};

class Core
{
public:
    array<array<int, 32>, 4> registers{}; // 4 compute units, each with 32 registers
    
    int pc; // Program Counter
    bool forwarding; // Forwarding flag (for data forwarding in pipeline)
    unordered_map<string, int> latencies; // Latencies for instructions (ADD, SUB, etc.)
    int stalls = 0; // Count of pipeline stalls
    int executed_instructions = 0; // Count of executed instructions
    int CID; // Compute Unit ID
    
    // Pipeline structures
    deque<Instruction> pipeline_stages[5]; // One queue for each pipeline stage
    unordered_map<int, pair<int, int>> register_status; // Maps reg -> (producing_instr_idx, ready_cycle)
    vector<Instruction> completed_instructions; // History of all executed instructions
    
    int current_cycle = 0; // Current clock cycle
    bool branch_taken = false; // Flag for taken branches/jumps
    int instr_count = 0; // Counter for instruction IDs

    // Constructor to initialize the core properties
    Core(int id, bool enable_forwarding)
    {
        pc = 0;
        forwarding = enable_forwarding;
        CID = id;

        // Initialize registers for the core
        for (int i = 0; i < 32; ++i)
        {
            registers[CID][i] = 0;
        }
    }

    // Convert register name (e.g., x1) to index (1 for x1)
    int reg_index(const string &r) { return stoi(r.substr(1, r.size() - 1)); }

    // Set instruction latencies for the core
    void set_latencies(const unordered_map<string, int>& latencies_map)
    {
        latencies = latencies_map;
    }

    // Check for data hazards in the ID stage
    bool check_hazards(const Instruction& instr, int& stall_cycles) {
        stall_cycles = 0;
        
        // Check RAW hazards
        if (instr.src_reg1 != -1) {
            if (register_status.count(instr.src_reg1)) {
                auto [instr_idx, ready_cycle] = register_status[instr.src_reg1];
                
                // If the value isn't ready yet
                if (ready_cycle > current_cycle) {
                    if (!forwarding) {
                        // Without forwarding, stall until the value is in register file
                        stall_cycles = max(stall_cycles, ready_cycle - current_cycle);
                        return stall_cycles > 0;
                    } else {
                        // With forwarding, check if we can forward from EX or MEM stages
                        // Find the instruction that produces the value
                        bool can_forward = false;
                        
                        // Look through all pipeline stages to find the producer instruction
                        for (int stage = EX; stage <= MEM; stage++) {
                            for (const auto& pipeline_instr : pipeline_stages[stage]) {
                                if (pipeline_instr.dest_reg == instr.src_reg1) {
                                    // If in EX, and will complete this cycle, we can forward
                                    if (stage == EX && pipeline_instr.stage_complete_cycle[EX] == current_cycle) {
                                        can_forward = true;
                                    }
                                    // If in MEM, we can always forward
                                    else if (stage == MEM) {
                                        can_forward = true;
                                    }
                                }
                            }
                        }
                        
                        if (!can_forward) {
                            stall_cycles = max(stall_cycles, 1); // Stall for at least 1 cycle
                            return true;
                        }
                    }
                }
            }
        }
        
        if (instr.src_reg2 != -1) {
            if (register_status.count(instr.src_reg2)) {
                auto [instr_idx, ready_cycle] = register_status[instr.src_reg2];
                
                // If the value isn't ready yet
                if (ready_cycle > current_cycle) {
                    if (!forwarding) {
                        // Without forwarding, stall until the value is in register file
                        stall_cycles = max(stall_cycles, ready_cycle - current_cycle);
                        return stall_cycles > 0;
                    } else {
                        // With forwarding, check if we can forward from EX or MEM stages
                        bool can_forward = false;
                        
                        // Look through all pipeline stages to find the producer instruction
                        for (int stage = EX; stage <= MEM; stage++) {
                            for (const auto& pipeline_instr : pipeline_stages[stage]) {
                                if (pipeline_instr.dest_reg == instr.src_reg2) {
                                    // If in EX, and will complete this cycle, we can forward
                                    if (stage == EX && pipeline_instr.stage_complete_cycle[EX] == current_cycle) {
                                        can_forward = true;
                                    }
                                    // If in MEM, we can always forward
                                    else if (stage == MEM) {
                                        can_forward = true;
                                    }
                                }
                            }
                        }
                        
                        if (!can_forward) {
                            stall_cycles = max(stall_cycles, 1); // Stall for at least 1 cycle
                            return true;
                        }
                    }
                }
            }
        }
        
        return stall_cycles > 0;
    }

    // Process the fetch stage - get next instruction
    void stage_fetch(const vector<string>& program) {
        // Don't fetch if there was a branch taken or we're at end of program
        if (branch_taken || pc >= static_cast<int>(program.size())) {
            branch_taken = false; // Reset branch flag
            return;
        }
        
        // Parse the instruction
        Instruction instr;
        istringstream iss(program[pc]);
        iss >> instr.opcode;
        
        // Parse operands based on instruction type
        if (instr.opcode == "ADD" || instr.opcode == "SUB" || instr.opcode == "MUL") {
            string rd, rs1, rs2;
            iss >> rd >> rs1 >> rs2;
            instr.dest_reg = reg_index(rd);
            instr.src_reg1 = reg_index(rs1);
            instr.src_reg2 = reg_index(rs2);
        } 
        else if (instr.opcode == "ADDI") {
            string rd, rs1;
            iss >> rd >> rs1 >> instr.imm;
            instr.dest_reg = reg_index(rd);
            instr.src_reg1 = reg_index(rs1);
        }
        else if (instr.opcode == "ARR") {
            iss >> instr.imm;
        }
        else if (instr.opcode == "LD" || instr.opcode == "LDC2" || instr.opcode == "LDC3" || instr.opcode == "LDC4") {
            string rd, address;
            iss >> rd >> address;
            instr.dest_reg = reg_index(rd);
            instr.mem_addr = stoi(address);
        }
        else if (instr.opcode == "SW") {
            string rs, address;
            iss >> rs >> address;
            instr.src_reg1 = reg_index(rs);
            instr.mem_addr = stoi(address);
        }
        else if (instr.opcode == "BNE") {
            string rs1, rs2, label;
            iss >> rs1 >> rs2 >> label;
            instr.src_reg1 = reg_index(rs1);
            instr.src_reg2 = reg_index(rs2);
            instr.label = label;
        }
        else if (instr.opcode == "J") {
            iss >> instr.label;
        }
        
        // Set up pipeline info
        instr.current_stage = IF;
        instr.issue_cycle = current_cycle;
        instr.stage_complete_cycle[IF] = current_cycle;
        
        // Add to IF stage and advance PC
        pipeline_stages[IF].push_back(instr);
        pc++;
    }
    
    // Process the decode stage - check for hazards
    void stage_decode(unordered_map<string, int>& labels) {
        if (pipeline_stages[IF].empty()) return;
        
        Instruction instr = pipeline_stages[IF].front();
        pipeline_stages[IF].pop_front();
        
        int stall_cycles = 0;
        bool hazard = check_hazards(instr, stall_cycles);
        
        if (hazard) {
            // Put instruction back in IF and stall
            instr.current_stage = IF;
            pipeline_stages[IF].push_front(instr);
            stalls += stall_cycles;
            return;
        }
        
        // Process control hazards (branches and jumps)
        if (instr.opcode == "BNE") {
            // Execute branch in decode stage to minimize branch penalty
            bool branch_condition = (registers[CID][instr.src_reg1] != registers[CID][instr.src_reg2]);
            
            if (branch_condition && labels.count(instr.label)) {
                // Branch taken - flush the pipeline and adjust PC
                pipeline_stages[IF].clear();
                pc = labels[instr.label];
                branch_taken = true;
                stalls += 2; // Branch penalty (2 cycles)
                
                // Track branch as completed (no need to go further in pipeline)
                instr.completed = true;
                instr.current_stage = ID;
                instr.stage_complete_cycle[ID] = current_cycle;
                completed_instructions.push_back(instr);
                executed_instructions++;
                return;
            }
        } 
        else if (instr.opcode == "J") {
            // Execute jump in decode stage
            if (labels.count(instr.label)) {
                // Jump taken - flush the pipeline and adjust PC
                pipeline_stages[IF].clear();
                pc = labels[instr.label];
                branch_taken = true;
                stalls += 2; // Jump penalty (2 cycles)
                
                // Track jump as completed
                instr.completed = true;
                instr.current_stage = ID;
                instr.stage_complete_cycle[ID] = current_cycle;
                completed_instructions.push_back(instr);
                executed_instructions++;
                return;
            }
        }
        
        // Mark dest register as being written by this instruction
        if (instr.dest_reg != -1) {
            int latency = latencies.count(instr.opcode) ? latencies[instr.opcode] : 1;
            
            // The register will be ready after MEM or WB stage depending on forwarding
            int ready_stage = forwarding ? EX : WB;
            int ready_cycle = current_cycle + (ready_stage - ID) + latency - 1;
            
            register_status[instr.dest_reg] = {executed_instructions, ready_cycle};
        }
        
        // Advance to EX stage
        instr.current_stage = ID;
        instr.stage_complete_cycle[ID] = current_cycle;
        pipeline_stages[ID].push_back(instr);
    }
    
    // Process the execute stage - perform computation
    void stage_execute(vector<vector<int>>& memory) {
        if (pipeline_stages[ID].empty()) return;
        
        Instruction instr = pipeline_stages[ID].front();
        pipeline_stages[ID].pop_front();
        
        // Get instruction latency
        int latency = latencies.count(instr.opcode) ? latencies[instr.opcode] : 1;
        
        // Perform the computation
        if (instr.opcode == "ADD") {
            instr.result_value = registers[CID][instr.src_reg1] + registers[CID][instr.src_reg2];
        } 
        else if (instr.opcode == "SUB") {
            instr.result_value = registers[CID][instr.src_reg1] - registers[CID][instr.src_reg2];
        } 
        else if (instr.opcode == "MUL") {
            instr.result_value = registers[CID][instr.src_reg1] * registers[CID][instr.src_reg2];
        } 
        else if (instr.opcode == "ADDI") {
            instr.result_value = registers[CID][instr.src_reg1] + instr.imm;
        }
        else if (instr.opcode == "ARR") {
            // Special case for initializing array
            for (int i = 0; i < instr.imm; ++i) {
                memory[i / 25][i % 25] = i + 1;
                registers[i / 25][i % 25] = i + 1;
            }
        }
        // Load/Store values are handled in MEM stage
        
        // Multi-cycle instructions would stall the EX stage
        if (latency > 1 && !forwarding) {
            stalls += (latency - 1);
        }
        
        // Advance to MEM stage
        instr.current_stage = EX;
        instr.stage_complete_cycle[EX] = current_cycle;
        pipeline_stages[EX].push_back(instr);
    }
    
    // Process the memory stage - perform memory operations
    void stage_memory(vector<vector<int>>& memory) {
        if (pipeline_stages[EX].empty()) return;
        
        Instruction instr = pipeline_stages[EX].front();
        pipeline_stages[EX].pop_front();
        
        // Handle memory operations
        if (instr.opcode == "LD") {
            instr.result_value = memory[CID][instr.mem_addr];
        } 
        else if (instr.opcode == "LDC2") {
            if (CID == 0)
                instr.result_value = memory[CID+1][instr.mem_addr];
        } 
        else if (instr.opcode == "LDC3") {
            if (CID == 0)
                instr.result_value = memory[CID+2][instr.mem_addr];
        } 
        else if (instr.opcode == "LDC4") {
            if (CID == 0)
                instr.result_value = memory[CID+3][instr.mem_addr];
        } 
        else if (instr.opcode == "SW") {
            memory[CID][instr.mem_addr] = registers[CID][instr.src_reg1];
        }
        
        // Advance to WB stage
        instr.current_stage = MEM;
        instr.stage_complete_cycle[MEM] = current_cycle;
        pipeline_stages[MEM].push_back(instr);
    }
    
    // Process the writeback stage - write results to registers
    void stage_writeback() {
        if (pipeline_stages[MEM].empty()) return;
        
        Instruction instr = pipeline_stages[MEM].front();
        pipeline_stages[MEM].pop_front();
        
        // Write result to register file
        if (instr.dest_reg != -1) {
            // For most instructions, write the computed result
            if (instr.opcode == "ADD" || instr.opcode == "SUB" || instr.opcode == "MUL" || 
                instr.opcode == "ADDI" || instr.opcode == "LD" || 
                instr.opcode == "LDC2" || instr.opcode == "LDC3" || instr.opcode == "LDC4") {
                registers[CID][instr.dest_reg] = instr.result_value;
            }
        }
        
        // Mark instruction as completed
        instr.current_stage = WB;
        instr.stage_complete_cycle[WB] = current_cycle;
        instr.completed = true;
        
        // Add to completed instructions
        completed_instructions.push_back(instr);
        executed_instructions++;
    }
    
    // Execute one cycle of all pipeline stages
    void execute_cycle(const vector<string>& program, vector<vector<int>>& memory, unordered_map<string, int>& labels) {
        // Execute pipeline stages in reverse order to prevent data conflicts
        stage_writeback();
        stage_memory(memory);
        stage_execute(memory);
        stage_decode(labels);
        stage_fetch(program);
        
        current_cycle++;
    }
    
    // Check if there are any instructions in the pipeline
    bool pipeline_active() {
        for (int i = 0; i < 5; i++) {
            if (!pipeline_stages[i].empty()) return true;
        }
        return pc < program_size;
    }
    
    // Get final cycle count
    int get_total_cycles() {
        return current_cycle;
    }
    
    // Set program size for checking completion
    int program_size = 0;
};

// Simulator class to manage multiple cores and their interactions
class Simulator
{
public:
    vector<vector<int>> memory;  // Memory sections for each core
    vector<Core> cores; // Vector of compute units (cores)
    vector<string> program; // Program to execute
    unordered_map<string, int> labels; // Label to line number mapping

    // Constructor to initialize the simulator with cores
    Simulator(int num_cores, bool enable_forwarding) : memory(4, vector<int>(1024, 0))
    {
        for (int i = 0; i < num_cores; ++i)
        {
            cores.emplace_back(i, enable_forwarding);
        }
    }

    // Set instruction latencies for the cores
    void set_instruction_latencies()
    {
        cout << "Enter latencies for ADD, SUB, MUL, DIV: ";
        cin >> cores[0].latencies["ADD"] >> cores[0].latencies["SUB"] >> cores[0].latencies["MUL"] >> cores[0].latencies["DIV"];
        for (int i = 1; i < cores.size(); ++i)
        {
            cores[i].latencies = cores[0].latencies;
        }
    }

    // Load program from file and set up labels
    bool load_program(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
        {
            cerr << "Error opening file: " << filename << endl;
            return false;
        }

        string line;
        int line_num = 0;
        while (getline(file, line))
        {
            istringstream iss(line);
            string first_word;
            iss >> first_word;

            if (!first_word.empty() && first_word.back() == ':')
            {
                first_word.pop_back();
                labels[first_word] = line_num;
            }
            else
            {
                program.push_back(line);
                line_num++;
            }
        }
        file.close();
        
        // Set program size for all cores
        for (auto& core : cores) {
            core.program_size = program.size();
        }
        
        return true;
    }

    // Display pipeline stage information
    void display_pipeline_info() {
        for (int i = 0; i < cores.size(); ++i) {
            cout << "\nCore " << i << " Pipeline Information:" << endl;
            cout << "Cycle: " << cores[i].current_cycle << endl;
            
            // Show instructions in each stage
            string stage_names[5] = {"IF", "ID", "EX", "MEM", "WB"};
            for (int s = 0; s < 5; s++) {
                cout << stage_names[s] << " Stage: ";
                if (cores[i].pipeline_stages[s].empty()) {
                    cout << "Empty" << endl;
                } else {
                    auto& instr = cores[i].pipeline_stages[s].front();
                    cout << instr.opcode;
                    if (instr.dest_reg != -1) cout << " x" << instr.dest_reg;
                    if (instr.src_reg1 != -1) cout << " x" << instr.src_reg1;
                    if (instr.src_reg2 != -1) cout << " x" << instr.src_reg2;
                    if (!instr.label.empty()) cout << " " << instr.label;
                    cout << endl;
                }
            }
        }
    }

    // Display core statistics
    void display()
    {
        for (int i = 0; i < cores.size(); ++i)
        {
            cout << "Core " << i << " Stats:" << endl;
            cout << "Number of stalls: " << cores[i].stalls << endl;
            
            int total_cycles = cores[i].get_total_cycles();
            cout << "Number of clock cycles: " << total_cycles << endl;
            
            double ipc = static_cast<double>(cores[i].executed_instructions) / total_cycles;
            cout << "Instructions per cycle (IPC): " << ipc << endl;
            
            cout << "Register States:\n";
            for (int j = 0; j < 32; ++j)
            {
                cout << "x" << j << ": " << cores[i].registers[i][j] << "  ";
                if (j % 8 == 7) cout << endl;
            }
            cout << endl;
            
           
         
        }
    }

    // Run the program on all cores
    void run()
    {
        bool fivestage = false;
        cout << "Enable fivestage pipeline display? (1 for Yes, 0 for No): ";
        cin >> fivestage;
        
        while (true)
        {
            bool all_done = true;
            for (auto &core : cores)
            {
                if (core.pipeline_active()) {
                    core.execute_cycle(program, memory, labels);
                    all_done = false;
                }
            }
            
            if (fivestage) {
                display_pipeline_info();
            }
            
            if (all_done)
                break;
        }
    }
};

int main()
{
    bool enable_forwarding;
    int num_cores = 4;
    cout << "Enable data forwarding? (1 for Yes, 0 for No): ";
    cin >> enable_forwarding;

    Simulator sim(num_cores, enable_forwarding);
    sim.set_instruction_latencies();
    
    if (!sim.load_program("ro.asm"))
        return 1;

    sim.run();
    sim.display();
    return 0;
}