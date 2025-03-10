#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <queue>

using namespace std;

struct Instruction {
    string opcode;
    int dest_reg = -1;    // Destination register (rd)
    int src_reg1 = -1;    // Source register 1 (rs1)
    int src_reg2 = -1;    // Source register 2 (rs2)
    int imm = 0;          // Immediate value
    int mem_addr = -1;    // Memory address
    string label;         // Branch/jump label
    int issue_cycle = -1; // Cycle when instruction was issued
};

class Core
{
public:
    array<array<int, 32>, 4> registers{}; // 4 compute units, each with 32 registers
    
    int pc; // Program Counter (IF stage)
    bool forwarding; // Forwarding flag (for data forwarding in pipeline)
    unordered_map<string, int> latencies; // Latencies for instructions (ADD, SUB, etc.)
    int stalls = 0; // Count of pipeline stalls
    int executed_instructions = 0; // Count of executed instructions
    int clock_cycles = 0; // Total number of clock cycles
    int CID; // Compute Unit ID
    
    // Track pipeline registers and stages
    vector<Instruction> pipeline; // Instructions in the pipeline
    unordered_map<int, int> reg_ready_cycle; // Maps register to cycle when it will be ready
    int current_cycle = 0; // Current clock cycle

    // Constructor to initialize the core properties
    Core(int id, bool enable_forwarding)
    {
        pc = 0;  // Initialize program counter (PC) to 0 (IF stage)
        forwarding = enable_forwarding;  // Set forwarding flag based on input
        CID = id;  // Assign core ID (to identify the core)

        // Initialize registers for the core (32 registers)
        for (int i = 0; i < 32; ++i)
        {
            registers[CID][i] = 0;  // Set all registers to 0
        }
    }

    // Convert register name (e.g., x1) to index (1 for x1)
    int reg_index(const string &r) { return stoi(r.substr(1, r.size() - 1)); }

    // Set instruction latencies for the core
    void set_latencies(const unordered_map<string, int>& latencies_map)
    {
        latencies = latencies_map;
    }

    // Check for RAW hazards
    int check_raw_hazard(const Instruction& instr) {
        int stall_cycles = 0;
        
        // Check source registers against destination registers in pipeline
        if (instr.src_reg1 != -1) {
            if (reg_ready_cycle.count(instr.src_reg1)) {
                int wait_cycles = reg_ready_cycle[instr.src_reg1] - current_cycle;
                if (wait_cycles > 0) {
                    if (!forwarding) {
                        stall_cycles = max(stall_cycles, wait_cycles);
                    } else {
                        // With forwarding, stall only if the value isn't ready in EX stage
                        if (wait_cycles > 1) {
                            stall_cycles = max(stall_cycles, wait_cycles - 1);
                        }
                    }
                }
            }
        }
        
        if (instr.src_reg2 != -1) {
            if (reg_ready_cycle.count(instr.src_reg2)) {
                int wait_cycles = reg_ready_cycle[instr.src_reg2] - current_cycle;
                if (wait_cycles > 0) {
                    if (!forwarding) {
                        stall_cycles = max(stall_cycles, wait_cycles);
                    } else {
                        // With forwarding, stall only if the value isn't ready in EX stage
                        if (wait_cycles > 1) {
                            stall_cycles = max(stall_cycles, wait_cycles - 1);
                        }
                    }
                }
            }
        }
        
        return stall_cycles;
    }

    // Execute the instructions for the core
    void execute(const vector<string> &program, vector<vector<int>> &memory, unordered_map<string, int> &labels)
    {
        if (pc >= static_cast<int>(program.size()))
            return;

        // Parse the instruction
        Instruction instr;
        istringstream iss(program[pc]);
        iss >> instr.opcode;

        int latency = latencies.count(instr.opcode) ? latencies[instr.opcode] : 1;
        
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

        // Check for RAW hazards
        int raw_stalls = check_raw_hazard(instr);
        
        // This is critical - increment stalls properly
        if (raw_stalls > 0) {
            stalls += raw_stalls;
            current_cycle += raw_stalls;
        }
        
        // Mark when the destination register will be ready
        if (instr.dest_reg != -1) {
            // For pipelined execution with forwarding, the result is available after EX stage
            if (forwarding) {
                reg_ready_cycle[instr.dest_reg] = current_cycle + 2; // Available after EX stage
            } else {
                // Without forwarding, result is available after WB stage
                reg_ready_cycle[instr.dest_reg] = current_cycle + latency;
            }
        }
        
        // Execute the instruction
        executed_instructions++;
        instr.issue_cycle = current_cycle;
        
        // Process branching
        if (instr.opcode == "BNE") {
            if (registers[CID][instr.src_reg1] != registers[CID][instr.src_reg2] && labels.count(instr.label)) {
                stalls += 2; // Branch penalty (2 cycles)
                current_cycle += 2;
                pc = labels[instr.label];
                return;
            }
        } 
        else if (instr.opcode == "J") {
            if (labels.count(instr.label)) {
                stalls += 2; // Jump penalty (2 cycles)
                current_cycle += 2;
                pc = labels[instr.label];
                return;
            }
        }
        
        // Perform the actual operation
        if (instr.opcode == "ADD") {
            registers[CID][instr.dest_reg] = registers[CID][instr.src_reg1] + registers[CID][instr.src_reg2];
        } 
        else if (instr.opcode == "SUB") {
            registers[CID][instr.dest_reg] = registers[CID][instr.src_reg1] - registers[CID][instr.src_reg2];
        } 
        else if (instr.opcode == "MUL") {
            registers[CID][instr.dest_reg] = registers[CID][instr.src_reg1] * registers[CID][instr.src_reg2];
        } 
        else if (instr.opcode == "ADDI") {
            registers[CID][instr.dest_reg] = registers[CID][instr.src_reg1] + instr.imm;
        } 
        else if (instr.opcode == "ARR") {
            for (int i = 0; i < instr.imm; ++i) {
                memory[i / 25][i % 25] = i + 1;
                registers[i / 25][i % 25] = i + 1;
            }
        } 
        else if (instr.opcode == "LD") {
            registers[CID][instr.dest_reg] = memory[CID][instr.mem_addr];
        } 
        else if (instr.opcode == "LDC2") {
            if (CID == 0)
                registers[CID][instr.dest_reg] = memory[CID+1][instr.mem_addr];
        } 
        else if (instr.opcode == "LDC3") {
            if (CID == 0)
                registers[CID][instr.dest_reg] = memory[CID+2][instr.mem_addr];
        } 
        else if (instr.opcode == "LDC4") {
            if (CID == 0)
                registers[CID][instr.dest_reg] = memory[CID+3][instr.mem_addr];
        } 
        else if (instr.opcode == "SW") {
            memory[CID][instr.mem_addr] = registers[CID][instr.src_reg1];
        }
        
        // Update pipeline state
        pipeline.push_back(instr);
        
        // Calculate stalls for non-pipelined execution (instruction latency)
        if (!forwarding && latency > 1) {
            stalls += (latency - 1);  // Count additional cycles as stalls for non-pipelined
        }
        
        // Update cycle count based on execution model
        if (forwarding) {
            current_cycle++; // One cycle per instruction (pipelined)
        } else {
            current_cycle += latency; // Full latency per instruction (non-pipelined)
        }
        
        pc++; // Increment program counter
    }
    
    // Get final cycle count
    int get_total_cycles() {
        return current_cycle;
    }
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
    Simulator(int num_cores, bool enable_forwarding) : memory(4, vector<int>(1024, 0))  // Memory initialized for 4 cores
    {
        for (int i = 0; i < num_cores; ++i)
        {
            cores.emplace_back(i, enable_forwarding); // Initialize cores with forwarding flag
        }
    }

    // Set instruction latencies for the cores
    void set_instruction_latencies()
    {
        cout << "Enter latencies for ADD, SUB, MUL, DIV: ";
        cin >> cores[0].latencies["ADD"] >> cores[0].latencies["SUB"] >> cores[0].latencies["MUL"] >> cores[0].latencies["DIV"];
        for (int i = 1; i < cores.size(); ++i)
        {
            cores[i].latencies = cores[0].latencies; // Copy latencies to other cores
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
                labels[first_word] = line_num; // Label found, map it to the line number
            }
            else
            {
                program.push_back(line); // Instruction found, add to program
                line_num++;
            }
        }
        file.close();
        return true;
    }

    // Display core statistics (stalls, IPC, register states)
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
                if (j % 8 == 7) cout << endl;  // Line break every 8 registers for readability
            }
            cout << endl;
        }
    }

    // Run the program on all cores
    void run()
    {
        while (true)
        {
            bool all_done = true;
            for (auto &core : cores)
            {
                if (core.pc < static_cast<int>(program.size())) // If there are still instructions to execute
                {
                    core.execute(program, memory, labels); // Execute instruction for the core
                    all_done = false;
                }
            }
            if (all_done) // If all cores are done, stop execution
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
    
    if (!sim.load_program("input.asm"))
        return 1;

    sim.run();
    sim.display(); // Display simulation results
    return 0;
}