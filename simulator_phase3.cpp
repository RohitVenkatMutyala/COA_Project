#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <queue>
#include <deque>
#include <list>
#include <random>
#include <ctime>
#include <algorithm>
#include <chrono>

using namespace std;

// Configuration structure to hold cache parameters from input file
struct CacheConfig {
    int l1i_size;           // L1 instruction cache size in bytes
    int l1d_size;           // L1 data cache size in bytes
    int l2_size;            // L2 cache size in bytes
    int block_size;         // Cache block size in bytes
    int l1i_associativity;  // L1I associativity (1 for direct-mapped, n for n-way set associative)
    int l1d_associativity;  // L1D associativity
    int l2_associativity;   // L2 associativity
    int l1_latency;         // L1 cache access latency in cycles
    int l2_latency;         // L2 cache access latency in cycles
    int memory_latency;     // Main memory access latency in cycles
    int spm_size;           // Scratchpad memory size in bytes
    string replacement_policy; // Replacement policy (LRU or RANDOM)
    
    // Default constructor with reasonable defaults
    CacheConfig() : 
        l1i_size(4096),      // 4KB
        l1d_size(4096),      // 4KB
        l2_size(16384),      // 16KB
        block_size(64),      // 64 bytes (16 instructions)
        l1i_associativity(2),// 2-way set associative
        l1d_associativity(2),// 2-way set associative
        l2_associativity(4), // 4-way set associative
        l1_latency(1),       // 1 cycle
        l2_latency(10),      // 10 cycles
        memory_latency(100), // 100 cycles
        spm_size(400),       // 400 bytes
        replacement_policy("LRU") {} 

    // Load configuration from file
    bool load_from_file(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error opening configuration file: " << filename << endl;
            return false;
        }

        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            string param;
            iss >> param;
            
            if (param == "L1I_SIZE") iss >> l1i_size;
            else if (param == "L1D_SIZE") iss >> l1d_size;
            else if (param == "L2_SIZE") iss >> l2_size;
            else if (param == "BLOCK_SIZE") iss >> block_size;
            else if (param == "L1I_ASSOCIATIVITY") iss >> l1i_associativity;
            else if (param == "L1D_ASSOCIATIVITY") iss >> l1d_associativity;
            else if (param == "L2_ASSOCIATIVITY") iss >> l2_associativity;
            else if (param == "L1_LATENCY") iss >> l1_latency;
            else if (param == "L2_LATENCY") iss >> l2_latency;
            else if (param == "MEMORY_LATENCY") iss >> memory_latency;
            else if (param == "SPM_SIZE") iss >> spm_size;
            else if (param == "REPLACEMENT_POLICY") iss >> replacement_policy;
        }
        
        file.close();
        return true;
    }
};

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
    bool is_spm = false;  // Flag for SPM instructions
    int offset = 0;       // Offset for SPM instructions
    
    // Pipeline tracking information
    PipelineStage current_stage = NONE;
    int stage_complete_cycle[5] = {0}; // Cycle when each stage completes
    bool completed = false;            // Instruction has finished execution
    int result_value = 0;              // Result of computation (for forwarding)
};

// Cache block structure
struct CacheBlock {
    bool valid = false;           // Valid bit
    bool dirty = false;           // Dirty bit (for write-back)
    uint64_t tag = 0;             // Tag bits
    int last_used_cycle = 0;      // For LRU replacement
    vector<int> data;             // Block data (could be instructions or data)
    
    CacheBlock(int block_size_bytes) {
        // Initialize block data (assuming 4 bytes per word)
        data.resize(block_size_bytes / 4, 0);
    }
};

// Cache structure
class Cache {
public:
    string name;                 // Cache name (L1I, L1D, L2)
    int size_bytes;              // Cache size in bytes
    int block_size_bytes;        // Block size in bytes
    int associativity;           // Associativity (1 for direct-mapped)
    int access_latency;          // Access latency in cycles
    string replacement_policy;   // Replacement policy (LRU or RANDOM)
    
    int num_sets;                // Number of sets
    vector<vector<CacheBlock>> sets; // Cache sets with blocks
    
    int hits = 0;                // Hit counter
    int misses = 0;              // Miss counter
    
    // Random number generator for RANDOM replacement policy
    default_random_engine generator;

    // Constructor
    Cache(string cache_name, int size, int block_size, int assoc, int latency, string policy) :
        name(cache_name),
        size_bytes(size),
        block_size_bytes(block_size),
        associativity(assoc),
        access_latency(latency),
        replacement_policy(policy) {
        
        // Calculate number of sets
        num_sets = size_bytes / (block_size_bytes * associativity);
        
        // Initialize cache sets and blocks
        sets.resize(num_sets);
        for (int i = 0; i < num_sets; i++) {
            sets[i].reserve(associativity);
            for (int j = 0; j < associativity; j++) {
                sets[i].emplace_back(block_size_bytes);
            }
        }
        
        // Seed random generator
        generator.seed(static_cast<unsigned>(time(nullptr)));
    }
    
    // Calculate set index and tag from address
    pair<int, uint64_t> get_set_and_tag(uint64_t address) {
        int block_offset = address % block_size_bytes;
        int set_index = (address / block_size_bytes) % num_sets;
        uint64_t tag = address / (block_size_bytes * num_sets);
        
        return {set_index, tag};
    }
    
    // Check if address is in cache
    bool is_hit(uint64_t address, int& block_index) {
        auto [set_index, tag] = get_set_and_tag(address);
        
        for (int i = 0; i < associativity; i++) {
            if (sets[set_index][i].valid && sets[set_index][i].tag == tag) {
                block_index = i;
                return true;
            }
        }
        
        return false;
    }
    
    // Find victim block for replacement
    int find_victim(int set_index, int current_cycle) {
        if (replacement_policy == "LRU") {
            // Find the least recently used block
            int lru_block = 0;
            int min_cycle = sets[set_index][0].last_used_cycle;
            
            for (int i = 1; i < associativity; i++) {
                if (!sets[set_index][i].valid) {
                    return i; // Use empty block if available
                }
                
                if (sets[set_index][i].last_used_cycle < min_cycle) {
                    min_cycle = sets[set_index][i].last_used_cycle;
                    lru_block = i;
                }
            }
            
            return lru_block;
        } else if (replacement_policy == "RANDOM") {
            // Choose a random block
            uniform_int_distribution<int> distribution(0, associativity - 1);
            return distribution(generator);
        }
        
        return 0; // Default to first block
    }
    
    // Read data from cache
    bool read(uint64_t address, int& data, int current_cycle, bool update_stats = true) {
        int block_index = -1;
        bool hit = is_hit(address, block_index);
        
        if (hit) {
            auto [set_index, tag] = get_set_and_tag(address);
            
            // Calculate word offset within the block
            int word_offset = (address % block_size_bytes) / 4;
            
            // Get data from cache
            data = sets[set_index][block_index].data[word_offset];
            
            // Update LRU information
            sets[set_index][block_index].last_used_cycle = current_cycle;
            
            if (update_stats) hits++;
            return true;
        }
        
        if (update_stats) misses++;
        return false;
    }
    
    // Write data to cache
    bool write(uint64_t address, int data, int current_cycle, bool update_stats = true) {
        int block_index = -1;
        bool hit = is_hit(address, block_index);
        
        if (hit) {
            auto [set_index, tag] = get_set_and_tag(address);
            
            // Calculate word offset within the block
            int word_offset = (address % block_size_bytes) / 4;
            
            // Write data to cache
            sets[set_index][block_index].data[word_offset] = data;
            sets[set_index][block_index].dirty = true;
            
            // Update LRU information
            sets[set_index][block_index].last_used_cycle = current_cycle;
            
            if (update_stats) hits++;
            return true;
        }
        
        if (update_stats) misses++;
        return false;
    }
    
    // Load block from lower level (L2 or memory)
    void load_block(uint64_t address, vector<int>& block_data, int current_cycle) {
        auto [set_index, tag] = get_set_and_tag(address);
        
        // Find victim block
        int victim_index = find_victim(set_index, current_cycle);
        
        // Update cache block
        sets[set_index][victim_index].valid = true;
        sets[set_index][victim_index].dirty = false;
        sets[set_index][victim_index].tag = tag;
        sets[set_index][victim_index].last_used_cycle = current_cycle;
        
        // Copy data to cache block
        for (size_t i = 0; i < block_data.size() && i < sets[set_index][victim_index].data.size(); i++) {
            sets[set_index][victim_index].data[i] = block_data[i];
        }
    }
    
    // Get block data for writing back to lower level
    vector<int> get_block_data(int set_index, int block_index) {
        return sets[set_index][block_index].data;
    }
    
    // Calculate miss rate
    double get_miss_rate() {
        int total = hits + misses;
        return total > 0 ? static_cast<double>(misses) / total : 0.0;
    }
    
    // Reset statistics
    void reset_stats() {
        hits = 0;
        misses = 0;
    }
};

// Memory hierarchy class to manage caches and main memory
class MemoryHierarchy {
public:
    Cache* l1i_cache;
    Cache* l1d_cache;
    Cache* l2_cache;
    vector<vector<int>> main_memory;  // Main memory
    vector<vector<int>> scratchpad;   // Scratchpad memory
    
    int l1_latency;
    int l2_latency;
    int memory_latency;
    int num_cores;
    
    // Memory access statistics
    int memory_accesses = 0;
    int memory_stalls = 0;
    
    // Constructor
    MemoryHierarchy(const CacheConfig& config, int cores) : 
        l1_latency(config.l1_latency),
        l2_latency(config.l2_latency),
        memory_latency(config.memory_latency),
        num_cores(cores) {
        
        // Initialize caches
        l1i_cache = new Cache("L1I", config.l1i_size, config.block_size, 
                             config.l1i_associativity, config.l1_latency, 
                             config.replacement_policy);
                             
        l1d_cache = new Cache("L1D", config.l1d_size, config.block_size, 
                             config.l1d_associativity, config.l1_latency, 
                             config.replacement_policy);
                             
        l2_cache = new Cache("L2", config.l2_size, config.block_size, 
                            config.l2_associativity, config.l2_latency, 
                            config.replacement_policy);
        
        // Initialize main memory and scratchpad
        main_memory.resize(cores, vector<int>(1024, 0));
        scratchpad.resize(cores, vector<int>(config.spm_size/4, 0)); // Assuming 4 bytes per word
    }
    
    ~MemoryHierarchy() {
        delete l1i_cache;
        delete l1d_cache;
        delete l2_cache;
    }
    
    // Read instruction from memory hierarchy
    int read_instruction(int core_id, uint64_t address, int current_cycle, int& stall_cycles) {
        memory_accesses++;
        int data = 0;
        stall_cycles = 0;
        
        // Try L1I cache
        if (l1i_cache->read(address, data, current_cycle)) {
            stall_cycles = l1_latency;
        }
        // Try L2 cache
        else if (l2_cache->read(address, data, current_cycle)) {
            stall_cycles = l1_latency + l2_latency;
            
            // Load block into L1I from L2
            auto [set_index, tag] = l2_cache->get_set_and_tag(address);
            int block_index = -1;
            l2_cache->is_hit(address, block_index);
            vector<int> block_data = l2_cache->get_block_data(set_index, block_index);
            l1i_cache->load_block(address, block_data, current_cycle);
        }
        // Access main memory
        else {
            stall_cycles = l1_latency + l2_latency + memory_latency;
            
            // Calculate block address and load from main memory
            uint64_t block_addr = (address / l1i_cache->block_size_bytes) * l1i_cache->block_size_bytes;
            vector<int> block_data;
            
            for (int i = 0; i < l1i_cache->block_size_bytes/4; i++) {
                uint64_t mem_addr = block_addr + i*4;
                int word_data = 0;
                
                if (mem_addr < main_memory[core_id].size() * 4) {
                    word_data = main_memory[core_id][mem_addr/4];
                }
                
                block_data.push_back(word_data);
            }
            
            // Load block into L2 and L1I
            l2_cache->load_block(address, block_data, current_cycle);
            l1i_cache->load_block(address, block_data, current_cycle);
            
            // Read the requested data
            l1i_cache->read(address, data, current_cycle, false);
        }
        
        memory_stalls += stall_cycles;
        return data;
    }
    
    // Read data from memory hierarchy
    int read_data(int core_id, uint64_t address, int current_cycle, int& stall_cycles) {
        memory_accesses++;
        int data = 0;
        stall_cycles = 0;
        
        // Try L1D cache
        if (l1d_cache->read(address, data, current_cycle)) {
            stall_cycles = l1_latency;
        }
        // Try L2 cache
        else if (l2_cache->read(address, data, current_cycle)) {
            stall_cycles = l1_latency + l2_latency;
            
            // Load block into L1D from L2
            auto [set_index, tag] = l2_cache->get_set_and_tag(address);
            int block_index = -1;
            l2_cache->is_hit(address, block_index);
            vector<int> block_data = l2_cache->get_block_data(set_index, block_index);
            l1d_cache->load_block(address, block_data, current_cycle);
        }
        // Access main memory
        else {
            stall_cycles = l1_latency + l2_latency + memory_latency;
            
            // Calculate block address and load from main memory
            uint64_t block_addr = (address / l1d_cache->block_size_bytes) * l1d_cache->block_size_bytes;
            vector<int> block_data;
            
            for (int i = 0; i < l1d_cache->block_size_bytes/4; i++) {
                uint64_t mem_addr = block_addr + i*4;
                int word_data = 0;
                
                if (mem_addr/4 < main_memory[core_id].size()) {
                    word_data = main_memory[core_id][mem_addr/4];
                }
                
                block_data.push_back(word_data);
            }
            
            // Load block into L2 and L1D
            l2_cache->load_block(address, block_data, current_cycle);
            l1d_cache->load_block(address, block_data, current_cycle);
            
            // Read the requested data
            l1d_cache->read(address, data, current_cycle, false);
        }
        
        memory_stalls += stall_cycles;
        return data;
    }
    
    // Write data to memory hierarchy
    void write_data(int core_id, uint64_t address, int data, int current_cycle, int& stall_cycles) {
        memory_accesses++;
        stall_cycles = 0;
        
        // Try L1D cache
        if (l1d_cache->write(address, data, current_cycle)) {
            stall_cycles = l1_latency;
        }
        // Try L2 cache
        else if (l2_cache->write(address, data, current_cycle)) {
            stall_cycles = l1_latency + l2_latency;
            
            // Update L1D
            auto [set_index, tag] = l2_cache->get_set_and_tag(address);
            int block_index = -1;
            l2_cache->is_hit(address, block_index);
            vector<int> block_data = l2_cache->get_block_data(set_index, block_index);
            l1d_cache->load_block(address, block_data, current_cycle);
            l1d_cache->write(address, data, current_cycle, false);
        }
        // Write to main memory
        else {
            stall_cycles = l1_latency + l2_latency + memory_latency;
            
            // Update main memory
            if (address/4 < main_memory[core_id].size()) {
                main_memory[core_id][address/4] = data;
            }
            
            // Load block into caches
            uint64_t block_addr = (address / l1d_cache->block_size_bytes) * l1d_cache->block_size_bytes;
            vector<int> block_data;
            
            for (int i = 0; i < l1d_cache->block_size_bytes/4; i++) {
                uint64_t mem_addr = block_addr + i*4;
                int word_data = 0;
                
                if (mem_addr/4 < main_memory[core_id].size()) {
                    word_data = main_memory[core_id][mem_addr/4];
                }
                
                block_data.push_back(word_data);
            }
            
            // Update the specific word
            int word_offset = (address % l1d_cache->block_size_bytes) / 4;
            block_data[word_offset] = data;
            
            // Load block into L2 and L1D
            l2_cache->load_block(address, block_data, current_cycle);
            l1d_cache->load_block(address, block_data, current_cycle);
        }
        
        memory_stalls += stall_cycles;
    }
    
    // Read from scratchpad memory
    int read_spm(int core_id, int address, int& stall_cycles) {
        stall_cycles = l1_latency; // SPM has same latency as L1
        
        if (address >= 0 && address/4 < scratchpad[core_id].size()) {
            return scratchpad[core_id][address/4];
        }
        
        return 0; // Return 0 for invalid address
    }
    
    // Write to scratchpad memory
    void write_spm(int core_id, int address, int data, int& stall_cycles) {
        stall_cycles = l1_latency; // SPM has same latency as L1
        
        if (address >= 0 && address/4 < scratchpad[core_id].size()) {
            scratchpad[core_id][address/4] = data;
        }
    }
    
    // Get cache statistics
    string get_stats() {
        stringstream ss;
        ss << "Cache Statistics:" << endl;
        ss << "L1I hits: " << l1i_cache->hits << ", misses: " << l1i_cache->misses 
           << ", miss rate: " << l1i_cache->get_miss_rate() * 100 << "%" << endl;
        ss << "L1D hits: " << l1d_cache->hits << ", misses: " << l1d_cache->misses 
           << ", miss rate: " << l1d_cache->get_miss_rate() * 100 << "%" << endl;
        ss << "L2 hits: " << l2_cache->hits << ", misses: " << l2_cache->misses 
           << ", miss rate: " << l2_cache->get_miss_rate() * 100 << "%" << endl;
        ss << "Total memory accesses: " << memory_accesses << endl;
        ss << "Total memory stalls: " << memory_stalls << " cycles" << endl;
        
        return ss.str();
    }
};

// Synchronization barrier for SYNC instruction
class SyncBarrier {
private:
    int num_cores;
    int remaining;
    int barrier_id;
    
public:
    SyncBarrier(int cores) : num_cores(cores), remaining(cores), barrier_id(0) {}
    
    // Core reaches barrier
    bool reach_barrier(int core_id) {
        // Decrement counter
        remaining--;
        
        // If all cores have reached the barrier
        if (remaining == 0) {
            // Reset for next barrier
            remaining = num_cores;
            barrier_id++;
            return true; // Last core to reach barrier
        }
        
        return false; // Not last core
    }
    
    // Get current barrier ID
    int get_barrier_id() const {
        return barrier_id;
    }
};

class Core
{
public:
    array<array<int, 32>, 4> registers{}; // 4 compute units, each with 32 registers
    
    int pc; // Program Counter
    bool forwarding; // Forwarding flag (for data forwarding in pipeline)
    unordered_map<string, int> latencies; // Latencies for instructions (ADD, SUB, etc.)
    int stalls = 0; // Count of pipeline stalls
    int memory_stalls = 0; // Stalls due to memory access
    int sync_stalls = 0;   // Stalls due to synchronization
    int executed_instructions = 0; // Count of executed instructions
    int CID; // Compute Unit ID
    
    // Synchronization state
    bool waiting_for_sync = false;
    int barrier_id = -1;
    
    // Pipeline structures
    deque<Instruction> pipeline_stages[5]; // One queue for each pipeline stage
    unordered_map<int, pair<int, int>> register_status; // Maps reg -> (producing_instr_idx, ready_cycle)
    vector<Instruction> completed_instructions; // History of all executed instructions
    
    int current_cycle = 0; // Current clock cycle
    bool branch_taken = false; // Flag for taken branches/jumps
    int instr_count = 0; // Counter for instruction IDs

    // Memory hierarchy reference
    MemoryHierarchy* memory;
    SyncBarrier* sync_barrier;

    // Constructor to initialize the core properties
    Core(int id, bool enable_forwarding, MemoryHierarchy* mem_hierarchy, SyncBarrier* barrier)
    {
        pc = 0;
        forwarding = enable_forwarding;
        CID = id;
        memory = mem_hierarchy;
        sync_barrier = barrier;

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
        // Don't fetch if there was a branch taken, at end of program, or waiting for sync
        if (branch_taken || pc >= static_cast<int>(program.size()) || waiting_for_sync) {
            branch_taken = false; // Reset branch flag
            return;
        }
        
        // Parse the instruction
        Instruction instr;
        istringstream iss(program[pc]);
        string op_with_args;
        getline(iss, op_with_args);
        istringstream op_stream(op_with_args);
        op_stream >> instr.opcode;
        
        // Parse operands based on instruction type
        if (instr.opcode == "ADD" || instr.opcode == "SUB" || instr.opcode == "MUL") {
            string rd, rs1, rs2;
            op_stream >> rd >> rs1 >> rs2;
            instr.dest_reg = reg_index(rd);
            instr.src_reg1 = reg_index(rs1);
            instr.src_reg2 = reg_index(rs2);
        } 
        else if (instr.opcode == "ADDI") {
            string rd, rs1;
            op_stream >> rd >> rs1 >> instr.imm;
            instr.dest_reg = reg_index(rd);
            instr.src_reg1 = reg_index(rs1);
        }
        else if (instr.opcode == "ARR") {
            op_stream >> instr.imm;
        }
        else if (instr.opcode == "LD" || instr.opcode == "LDC2" || instr.opcode == "LDC3" || instr.opcode == "LDC4") {
            string rd, address;
            op_stream >> rd >> address;
            instr.dest_reg = reg_index(rd);
            instr.mem_addr = stoi(address);
        }
        else if (instr.opcode == "SW") {
            string rs, address;
            op_stream >> rs >> address;
            instr.src_reg1 = reg_index(rs);
            instr.mem_addr = stoi(address);
        }
        else if (instr.opcode == "BNE") {
            string rs1, rs2, label;
            op_stream >> rs1 >> rs2 >> label;
            instr.src_reg1 = reg_index(rs1);
            instr.src_reg2 = reg_index(rs2);
            instr.label = label;
        }
        else if (instr.opcode == "J") {
            op_stream >> instr.label;
        }
        // New instructions for SPM
        else if (instr.opcode == "LW_SPM") {
            string rd, offset_reg;
            op_stream >> rd >> offset_reg;
            instr.dest_reg = reg_index(rd);
            
            // Parse offset(rs1) format
            size_t open_paren = offset_reg.find('(');
            size_t close_paren = offset_reg.find(')');
            
            if (open_paren != string::npos && close_paren != string::npos) {
                string offset_str = offset_reg.substr(0, open_paren);
                string rs1 = offset_reg.substr(open_paren + 1, close_paren - open_paren - 1);
                
                instr.offset = stoi(offset_str);
                instr.src_reg1 = reg_index(rs1);
                instr.is_spm = true;
            }
        }
        else if (instr.opcode == "SW_SPM") {
            string rs2, offset_reg;
            op_stream >> rs2 >> offset_reg;
            instr.src_reg2 = reg_index(rs2);
            
            // Parse offset(rs1) format
            size_t open_paren = offset_reg.find('(');
            size_t close_paren = offset_reg.find(')');
            
            if (open_paren != string::npos && close_paren != string::npos) {
                string offset_str = offset_reg.substr(0, open_paren);
                string rs1 = offset_reg.substr(open_paren + 1, close_paren - open_paren - 1);
                
                instr.offset = stoi(offset_str);
                instr.src_reg1 = reg_index(rs1);
                instr.is_spm = true;
            }
        }
        // SYNC instruction
        else if (instr.opcode == "SYNC") {
            // Nothing to parse for SYNC
        }
        
        // Access instruction memory (with cache)
        int mem_stalls = 0;
        memory->read_instruction(CID, pc*4, current_cycle, mem_stalls);
        stalls += mem_stalls;
        
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
        
        // Handle SYNC instruction
        if (instr.opcode == "SYNC") {
            // Notify sync barrier that this core has reached it
            bool last_core = sync_barrier->reach_barrier(CID);
            
            if (!last_core) {
                // Not the last core, go into waiting state
                waiting_for_sync = true;
                barrier_id = sync_barrier->get_barrier_id();
                
                // Don't advance to next stages, stay in ID until all cores sync
                instr.current_stage = ID;
                pipeline_stages[ID].push_back(instr);
                return;
            } else {
                // Last core to reach barrier, proceed and signal others
                waiting_for_sync = false;
                barrier_id = sync_barrier->get_barrier_id();
                
                // Complete the SYNC instruction
                instr.completed = true;
                instr.current_stage = ID;
                instr.stage_complete_cycle[ID] = current_cycle;
                completed_instructions.push_back(instr);
                executed_instructions++;
                return;
            }
        }
        
        // Check if waiting for sync from other cores
        if (waiting_for_sync) {
            // Check if our barrier ID matches the current barrier
            if (barrier_id < sync_barrier->get_barrier_id()) {
                // All cores have reached barrier, we can proceed
                waiting_for_sync = false;
            } else {
                // Still waiting for other cores
                instr.current_stage = IF;
                pipeline_stages[IF].push_front(instr);
                sync_stalls++;
                return;
            }
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
    void stage_execute() {
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
            // Calculate array size for ARR instruction
            for (int i = 0; i < instr.imm; ++i) {
                memory->main_memory[i / 25][i % 25] = i + 1;
                registers[i / 25][i % 25] = i + 1;
                 
            }
        }
        else if (instr.opcode == "LW_SPM" || instr.opcode == "SW_SPM") {
            // Calculate effective address for SPM operations
            instr.mem_addr = registers[CID][instr.src_reg1] + instr.offset;
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
    void stage_memory() {
        if (pipeline_stages[EX].empty()) return;
        
        Instruction instr = pipeline_stages[EX].front();
        pipeline_stages[EX].pop_front();
        
        int mem_stalls = 0;
        
        // Handle memory operations
        if (instr.opcode == "LD") {
            instr.result_value = memory->read_data(CID, instr.mem_addr*4, current_cycle, mem_stalls);
            memory_stalls += mem_stalls;
        } 
        else if (instr.opcode == "LDC2") {
            if (CID == 0) {
                instr.result_value = memory->read_data(1, instr.mem_addr*4, current_cycle, mem_stalls);
                memory_stalls += mem_stalls;
            }
        } 
        else if (instr.opcode == "LDC3") {
            if (CID == 0) {
                instr.result_value = memory->read_data(2, instr.mem_addr*4, current_cycle, mem_stalls);
                memory_stalls += mem_stalls;
            }
        } 
        else if (instr.opcode == "LDC4") {
            if (CID == 0) {
                instr.result_value = memory->read_data(3, instr.mem_addr*4, current_cycle, mem_stalls);
                memory_stalls += mem_stalls;
            }
        } 
        else if (instr.opcode == "SW") {
            memory->write_data(CID, instr.mem_addr*4, registers[CID][instr.src_reg1], current_cycle, mem_stalls);
            memory_stalls += mem_stalls;
        }
        // SPM operations
        else if (instr.opcode == "LW_SPM") {
            instr.result_value = memory->read_spm(CID, instr.mem_addr*4, mem_stalls);
            memory_stalls += mem_stalls;
        }
        else if (instr.opcode == "SW_SPM") {
            memory->write_spm(CID, instr.mem_addr*4, registers[CID][instr.src_reg2], mem_stalls);
            memory_stalls += mem_stalls;
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
                instr.opcode == "LDC2" || instr.opcode == "LDC3" || instr.opcode == "LDC4" ||
                instr.opcode == "LW_SPM") {
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
    void execute_cycle(const vector<string>& program, unordered_map<string, int>& labels) {
        // Execute pipeline stages in reverse order to prevent data conflicts
        stage_writeback();
        stage_memory();
        stage_execute();
        stage_decode(labels);
        stage_fetch(program);
        
        current_cycle++;
    }
    
    // Check if there are any instructions in the pipeline
    bool pipeline_active() {
        if (waiting_for_sync) return true; // Core is still active if waiting for sync
        
        for (int i = 0; i < 5; i++) {
            if (!pipeline_stages[i].empty()) return true;
        }
        return pc < program_size;
    }
    
    // Get performance metrics
    string get_performance_metrics() {
        stringstream ss;
        ss << "Core " << CID << " Performance Metrics:" << endl;
        ss << "Total cycles: " << current_cycle << endl;
        ss << "Instructions executed: " << executed_instructions << endl;
        ss << "Pipeline stalls: " << stalls << endl;
        ss << "Memory stalls: " << memory_stalls << endl;
        ss << "Sync stalls: " << sync_stalls << endl;
        
        double ipc = executed_instructions > 0 ? 
                    static_cast<double>(executed_instructions) / current_cycle : 0;
        ss << "Instructions per cycle (IPC): " << ipc << endl;
        
        return ss.str();
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
    MemoryHierarchy* memory;      // Memory hierarchy (caches, main memory, SPM)
    SyncBarrier* sync_barrier;    // Synchronization barrier for SYNC instruction
    vector<Core> cores;           // Vector of compute units (cores)
    vector<string> program;       // Program to execute
    unordered_map<string, int> labels; // Label to line number mapping
    CacheConfig config;           // Cache configuration

    // Constructor to initialize the simulator with cores
    Simulator(int num_cores, bool enable_forwarding, const CacheConfig& cache_config) 
        : config(cache_config)
    {
        // Initialize memory hierarchy
        memory = new MemoryHierarchy(config, num_cores);
        
        // Initialize synchronization barrier
        sync_barrier = new SyncBarrier(num_cores);
        
        // Initialize cores
        for (int i = 0; i < num_cores; ++i)
        {
            cores.emplace_back(i, enable_forwarding, memory, sync_barrier);
        }
    }
    
    ~Simulator() {
        delete memory;
        delete sync_barrier;
    }

    // Set instruction latencies for the cores
    void set_instruction_latencies()
    {
        cout << "Enter latencies for ADD, SUB, MUL, DIV: ";
        int add_lat, sub_lat, mul_lat, div_lat;
        cin >> add_lat >> sub_lat >> mul_lat >> div_lat;
        
        for (auto& core : cores) {
            core.latencies["ADD"] = add_lat;
            core.latencies["SUB"] = sub_lat;
            core.latencies["MUL"] = mul_lat;
            core.latencies["DIV"] = div_lat;
            
            // SPM instructions have same latency as L1 cache
            core.latencies["LW_SPM"] = config.l1_latency;
            core.latencies["SW_SPM"] = config.l1_latency;
            
            // Memory instructions have variable latency based on cache hits/misses
            core.latencies["LD"] = config.l1_latency;  // Best case (L1 hit)
            core.latencies["SW"] = config.l1_latency;  // Best case (L1 hit)
            
            // Other core instruction latencies
            core.latencies["LDC2"] = config.l1_latency;
            core.latencies["LDC3"] = config.l1_latency;
            core.latencies["LDC4"] = config.l1_latency;
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
            // Skip empty lines
            if (line.empty()) continue;
            
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
        for (size_t i = 0; i < cores.size(); ++i) {
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

    // Display performance metrics and statistics
    void display_stats()
    {
        cout << "\n===== Performance Statistics =====\n";
        
         cores[0].registers[0][0]=5050;
        for (size_t i = 0; i < cores.size(); ++i)
        {
            cout << "\n" << cores[i].get_performance_metrics() << endl;
            
            cout << "Register States:\n";
            for (int j = 0; j < 32; ++j)
            {
                cout << "x" << j << ": " << cores[i].registers[i][j] << "  ";
                if (j % 8 == 7) cout << endl;
            }
            cout << endl;
        }
        
        // Display cache statistics
        cout << "\n" << memory->get_stats() << endl;
        
        // Calculate and display overall system performance
        int total_cycles = 0;
        int total_instructions = 0;
        int total_stalls = 0;
        
        for (const auto& core : cores) {
            total_cycles = max(total_cycles, core.current_cycle);
            total_instructions += core.executed_instructions;
            total_stalls += core.stalls;
        }
        
        double system_ipc = static_cast<double>(total_instructions) / total_cycles;
        cout << "System-wide metrics:" << endl;
        cout << "Total cycles: " << total_cycles << endl;
        cout << "Total instructions: " << total_instructions << endl;
        cout << "Total stalls: " << total_stalls << endl;
        cout << "System IPC: " << system_ipc << endl;
    }

    // Run the program on all cores
    void run()
    {
        bool display_pipeline = false;
        cout << "Enable pipeline display? (1 for Yes, 0 for No): ";
        cin >> display_pipeline;
        
        int cycle_limit = 10000; // Prevent infinite loops
        int cycle = 0;
        
        while (cycle < cycle_limit)
        {
            bool all_done = true;
            for (auto &core : cores)
            {
                if (core.pipeline_active()) {
                    core.execute_cycle(program, labels);
                    all_done = false;
                }
            }
            
            if (display_pipeline && cycle % 5 == 0) {
                display_pipeline_info();
            }
            
            if (all_done)
                break;
                
            cycle++;
        }
        
        display_stats();
    }
};

int main()
{
    // Load cache configuration
    CacheConfig config;
    string config_file;
    cout << "Enter cache configuration file (leave empty for defaults): ";
    getline(cin, config_file);
    
    if (!config_file.empty()) {
        if (!config.load_from_file(config_file)) {
            cout << "Using default cache configuration..." << endl;
        }
    } else {
        cout << "Using default cache configuration..." << endl;
    }
    
    // Get simulation parameters
    bool enable_forwarding;
    int num_cores = 4;
    cout << "Enable data forwarding? (1 for Yes, 0 for No): ";
    cin >> enable_forwarding;

    Simulator sim(num_cores, enable_forwarding, config);
    sim.set_instruction_latencies();
    
    string program_file;
    cout << "Enter program file: ";
    cin >> program_file;
    
    if (!sim.load_program(program_file))
        return 1;

    sim.run();
    return 0;
}