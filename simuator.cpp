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
    array<int, 32> registers{};
    array<int, 1024> memo{};
    int pc;
    int core_id;

    Core(int id) : pc(0), core_id(id)
    {
        registers.fill(0);
        memo.fill(0);
    }

    int reg_index(const string &r) { return stoi(r.substr(1,r.size()-1)); }

    void execute(const vector<string> &program, vector<int> &memory, unordered_map<string, int> &labels)
    {
        if (pc >= static_cast<int>(program.size()))
            return;

        istringstream iss(program[pc]);
        string opcode;
        iss >> opcode;

        if (opcode == "ADD")
        {
            string rd, rs1, rs2;
            iss >> rd >> rs1 >> rs2;
            int rd_idx = reg_index(rd), rs1_idx = reg_index(rs1), rs2_idx = reg_index(rs2);
            registers[rd_idx] = registers[rs1_idx] + registers[rs2_idx];
            memory[rd_idx] = memo[rd_idx] = registers[rd_idx];
        }
        else if (opcode == "ADDI")
        {
            string rd, rs1;
            int imm;
            iss >> rd >> rs1 >> imm;
            int rd_idx = reg_index(rd), rs1_idx = reg_index(rs1);
            registers[rd_idx] = registers[rs1_idx] + imm;
            memory[rd_idx] = memo[rd_idx] = registers[rd_idx];
        }
        else if (opcode == "LD")
        {
            string rd, rs2;
            int imm;
            iss >> rd >> imm >> rs2;
            registers[reg_index(rd)] = memory[imm + reg_index(rs2)];
        }
        else if (opcode == "SUB")
        {
            string rd, rs1, rs2;
            iss >> rd >> rs1 >> rs2;
            int rd_idx = reg_index(rd), rs1_idx = reg_index(rs1), rs2_idx = reg_index(rs2);
            registers[rd_idx] = registers[rs1_idx] - registers[rs2_idx];
            memo[rd_idx] = registers[rd_idx];
        }
        else if (opcode == "SW")
        {
            string rd, rs2;
            int imm;
            iss >> rd >> imm >> rs2;
            memory[imm + reg_index(rs2)] = registers[reg_index(rd)];
            memo[imm + reg_index(rs2)] = registers[reg_index(rd)];
        }
        else if (opcode == "BNE" || opcode == "BEQ" || opcode == "BLE")
        {
            string rs1, rs2, label;
            iss >> rs1 >> rs2 >> label;
            bool condition = (opcode == "BNE" && registers[reg_index(rs1)] != registers[reg_index(rs2)]) ||
                             (opcode == "BEQ" && registers[reg_index(rs1)] == registers[reg_index(rs2)]) ||
                             (opcode == "BLE" && registers[reg_index(rs1)] <= registers[reg_index(rs2)]);
            if (condition && labels.count(label))
            {
                pc = labels[label];
                return;
            }
        }
        else if (opcode == "JAL")
        {
            string rd, label;
            iss >> rd >> label;
            if (labels.count(label))
            {
                registers[reg_index(rd)] = pc + 1;
                pc = labels[label];
                return;
            }
        }
        else if (opcode == "J")
        {
            string label;
            iss >> label;
            if (labels.count(label))
            {
                pc = labels[label];
                return;
            }
        }

        pc++;
    }
};

class Simulator
{
public:
    int clock;
    vector<int> memory;
    vector<Core> cores;
    vector<string> program;
    unordered_map<string, int> labels;

    Simulator() : clock(0), memory(4096, 0), cores{Core(0), Core(1), Core(2), Core(3)} {}

    bool load_program(const string &filename)
    {
        ifstream infile(filename);
        if (!infile.is_open())
        {
            cerr << "Error: Could not open file " << filename << endl;
            return false;
        }

        string line;
        int line_number = 0;
        while (getline(infile, line))
        {
            istringstream iss(line);
            string first_word;
            iss >> first_word;

            if (first_word.empty())
                continue;
            if (first_word.back() == ':')
            {
                labels[first_word.substr(0, first_word.size() - 1)] = line_number;
            }
            else
            {
                program.push_back(line);
                line_number++;
            }
        }
        return true;
    }

    void run()
    {
        while (clock < static_cast<int>(program.size()))
        {
            for (auto &core : cores)
                core.execute(program, memory, labels);
            clock++;
        }
    }

    void display()
    {
        ofstream outfile("output.txt");

        cout << "Register States:\n";
        for (size_t i = 0; i < cores.size(); ++i)
        {
            cout << "Core " << i << ": ";
            //outfile << "Core " << i << ": ";
            for (auto reg : cores[i].registers)
            {
                cout << reg << ' ';
               // outfile << reg << ' ';
            }
            cout << '\n';
           
        }

        cout << "\nMemory States:\n";
        for (size_t i = 0; i < cores.size(); ++i)
        {
            cout << "Core " << i << ": ";
            outfile << "Core " << i << ": ";
            for (int j = 0; j < 9; ++j){
                cout << cores[i].memo[j] << ' ';
                outfile << cores[i].memo[j] << ' ';
            
            }
            cout << '\n';
            outfile << '\n';
        }
    }
};

int main()
{
    Simulator sim;
    if (!sim.load_program("in.asm"))
        return 1;

    sim.run();
    sim.display();

    cout << "Number of clock cycles: " << sim.clock << endl;
    return 0;
}