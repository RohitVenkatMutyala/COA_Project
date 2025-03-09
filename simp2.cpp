#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>

using namespace std;

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
    int EX_MEM = -1, MEM_WB = -1, ID_EX = -1; // Pipeline registers (EX/MEM, MEM/WB, ID/EX stages)

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

        // Initialize the pipeline registers (stages)
        EX_MEM = -1;  // No value in EX/MEM register initially
        MEM_WB = -1;  // No value in MEM/WB register initially
        ID_EX = -1;   // No value in ID/EX register initially
    }

    // Convert register name (e.g., x1) to index (1 for x1)
    int reg_index(const string &r) { return stoi(r.substr(1, r.size() - 1)); }

    // Set instruction latencies for the core
    void set_latencies(const unordered_map<string, int>& latencies_map)
    {
        latencies = latencies_map;
    }

    // Execute the instructions for the core
    void execute(const vector<string> &program, vector<vector<int>> &memory, unordered_map<string, int> &labels)
    {
        if (pc >= static_cast<int>(program.size()))
            return;

        istringstream iss(program[pc]);
        string opcode;
        iss >> opcode;

        int latency = latencies.count(opcode) ? latencies[opcode] : 1; // Get latency for opcode
        executed_instructions++; // Increment executed instruction count
        
        // Pipeline handling based on forwarding
        if (forwarding)
        {
            clock_cycles++; // Pipeline execution allows overlapping (1 cycle for each instruction)
        }
        else
        {
            clock_cycles += latency; // Non-pipelined execution accumulates full latency
            stalls += (latency - 1); // Each additional cycle counts as a stall (without forwarding)
        }

        // IF/ID (Fetch and Decode stage)
        if (opcode == "ADD" || opcode == "SUB" || opcode == "MUL")
        {
            string rd, rs1, rs2;
            iss >> rd >> rs1 >> rs2; // Get destination and source registers
            int rd_idx = reg_index(rd), rs1_idx = reg_index(rs1), rs2_idx = reg_index(rs2);

            // Execute the arithmetic operation
            if (opcode == "ADD")
                registers[CID][rd_idx] = registers[CID][rs1_idx] + registers[CID][rs2_idx];
            else if (opcode == "SUB")
                registers[CID][rd_idx] = registers[CID][rs1_idx] - registers[CID][rs2_idx];
            else if (opcode == "MUL")
                registers[CID][rd_idx] = registers[CID][rs1_idx] * registers[CID][rs2_idx];

            ID_EX = rd_idx; // Move to EX stage
        }
        // ID/EX (Decode and Execute stage)
        else if (opcode == "ADDI")
        {
            string rd, rs1;
            int imm;
            iss >> rd >> rs1 >> imm;
            registers[CID][reg_index(rd)] = registers[CID][reg_index(rs1)] + imm;
            ID_EX = reg_index(rd); // Move to EX stage
        }
        // EX/MEM (Execute and Memory stage)
        else if (opcode == "LD")
        {
            string rd, address;
            iss >> rd >> address;
            registers[CID][reg_index(rd)] = memory[CID][stoi(address)]; // Load from memory to register
            ID_EX = reg_index(rd); // Move to EX stage
        }
        else if (opcode == "LDC2")
        {
            string rd, address;
            iss >> rd >> address;
            if(CID==0)
            registers[CID][reg_index(rd)] = memory[CID+1][stoi(address)]; // Accessing memory of other cores
            ID_EX = reg_index(rd);
        }
        // MEM/WB (Memory and Write-back stage)
        else if (opcode == "LDC3")
        {
            string rd, address;
            iss >> rd >> address;
            if(CID==0)
            registers[CID][reg_index(rd)] = memory[CID+2][stoi(address)]; // Accessing memory of other cores
            ID_EX = reg_index(rd);
        }
        // Write-back (final result stored to register)
        else if (opcode == "LDC4")
        {
            string rd, address;
            iss >> rd >> address;
            if(CID==0)
            registers[CID][reg_index(rd)] = memory[CID+3][stoi(address)]; // Accessing memory of other cores
            ID_EX = reg_index(rd);
        }
        // Store Word instruction
        else if (opcode == "SW")
        {
            string rs, address;
            iss >> rs >> address;
            memory[CID][stoi(address)] = registers[CID][reg_index(rs)]; // Store value from register to memory
        }
        // Branch instruction with control hazard
        else if (opcode == "BNE")
        {
            string rs1, rs2, label;
            iss >> rs1 >> rs2 >> label;
            if (registers[CID][reg_index(rs1)] != registers[CID][reg_index(rs2)] && labels.count(label))
            {
                stalls += 2; // Control hazard stall (branch misprediction)
                pc = labels[label]; // Jump to branch target
                return;
            }
        }
        // Jump instruction with control hazard
        else if (opcode == "J")
        {
            string label;
            iss >> label;
            if (labels.count(label))
            {
                stalls += 2; // Control hazard stall (jump misprediction)
                pc = labels[label]; // Jump to target address
                return;
            }
        }
        pc++; // Increment program counter (IF stage)
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

    // Assign memory values to different cores
    void assign_memory_to_cores()
    {
        for (int i = 0; i < 100; ++i)
        {
            memory[i / 25][i % 25] = i + 1;  // Distribute across the 4 cores' memory sections
            cores[i / 25].registers[i / 25][i % 25] = i + 1; // Initialize core registers
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
        for (int i = 0; i < cores.size()-3; ++i)
        {
            cout << "Core " << i << " Stats:" << endl;
            cout << "Number of stalls: " << cores[i].stalls << endl;
            cout << "Instructions per cycle (IPC): " << (static_cast<double>(cores[i].executed_instructions) / cores[i].clock_cycles) << endl;
            cout << "Number of clock cycles: " << cores[i].clock_cycles << endl;
            
            cout << "Register States:\n";
            for (int j = 0; j < 32; ++j)
            {
                cout << "x" << j << ": " << cores[i].registers[i][j] << "  ";
            }
            cout << endl;
        }
    }

    // Run the program on all cores
    void run()
    {
        assign_memory_to_cores(); // Assign memory to cores
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
    int num_cores;
    cout << "Enter number of compute units: ";
    cin >> num_cores;
    cout << "Enable data forwarding? (1 for Yes, 0 for No): ";
    cin >> enable_forwarding;

    Simulator sim(num_cores, enable_forwarding);
    sim.set_instruction_latencies();
    
    if (!sim.load_program("ro.asm"))
        return 1;

    sim.run();
    sim.display(); // Display simulation results
    return 0;
}
