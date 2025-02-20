#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include<fstream>
#include<unordered_map>

using namespace std;

class Cores {
public:
    vector<int> registers;
    vector<int>memo;
    int pc;
    int coreid;

    Cores(int cid) : registers(32, 0),memo(1024,0), pc(0), coreid(cid) {}

    void execute(const vector<string>& pgm, vector<int>&memory, unordered_map<string ,int>&labels  ) {
        if (pc >= pgm.size()) return;

        stringstream ss(pgm[pc]);
        int num;
        string opcode, rd_str, rs1_str, rs2_str,label;
        ss >> opcode ;
       if (opcode == "ADD") {
            ss >>  rd_str >> rs1_str >> rs2_str;
            int rd = stoi(rd_str.substr(1,rd_str.size()-1));
            int rs1 = stoi(rs1_str.substr(1,rs1_str.size()-1));
            int rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1));
            
            registers[rd] = registers[rs1] + registers[rs2];
            memory[rd]=registers[rd];
            memo[rd]=registers[rd];
        }
        else if(opcode=="ADDI"){
            ss >>  rd_str >> rs1_str >> num;
            int rd = stoi(rd_str.substr(1,rd_str.size()-1));
            int rs1 = stoi(rs1_str.substr(1,rs1_str.size()-1));
            int rs2 = num;
    
            registers[rd] = registers[rs1] + num;
            memory[rd]=registers[rd];
          memo[rd]=registers[rd];
        }
         else if (opcode == "LD") {
            ss>>rd_str>>num>>rs2_str;
            int  rd = stoi(rd_str.substr(1,rd_str.size()-1));
          int rs1 = num;
          int  rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1));  
           registers[rd]=memory[rs1+rs2];
            //registers[rd]=memo[rs1+rs2];
            //memo[rd]=registers[rd];
          
        }
        else if (opcode =="SUB"){
            ss>> rd_str >> rs1_str >> rs2_str;
            int rd = stoi(rd_str.substr(1,rd_str.size()-1));
            int rs1 = stoi(rs1_str.substr(1,rs1_str.size()-1));
            int rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1));
    
            registers[rd] = registers[rs1] - registers[rs2];
            memo[rd]=registers[rd];
        }
        else if( opcode == "SW"){
            ss>>rd_str>>num>>rs2_str;
            int  rd = stoi(rd_str.substr(1,rd_str.size()-1));
          int rs1=num;
          int  rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1)); 
          memory[rs1+rs2]=registers[rd]; 
           memo[rs1+rs2]=registers[rd];
            registers[rs1+rs2]=memory[rs1+rs2];

        }
        else if(opcode == "BNE"){
            ss>>rs1_str>>rs2_str>>label;
          int  rs1 = stoi(rs1_str.substr(1,rs1_str.size()-1));
          int  rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1));
          if(registers[rs1]!=registers[rs2]&& labels.find(label) != labels.end() ){
           pc = labels[label];
           return;
          }
        }
        else if(opcode == "BEQ"){
            ss>>rs1_str>>rs2_str>>label;
          int  rs1 = stoi(rs1_str.substr(1,rs1_str.size()-1));
          int  rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1));
          if(registers[rs1]==registers[rs2]&& labels.find(label) != labels.end() ){
           pc = labels[label];
           return;
          }

        }
        else if(opcode == "BLE"){
            ss>>rs1_str>>rs2_str>>label;
          int  rs1 = stoi(rs1_str.substr(1,rs1_str.size()-1));
          int  rs2 = stoi(rs2_str.substr(1,rs2_str.size()-1));
          if(registers[rs1]<=registers[rs2]&& labels.find(label) != labels.end() ){
           pc = labels[label];
           return;
          }

        }

        else if(opcode =="JAL"){
            ss>>rd_str>>label;
            int rd = stoi(rd_str.substr(1,rd_str.size()-1));
            if (labels.find(label) != labels.end()) {
                registers[rd] = pc + 1;  
                pc = labels[label];     
                return;                 
            } else {
                cerr << "Error: Undefined label " << label << endl;
            }
        }
       

        else if(opcode =="J"){
            ss>>label;
           
            if (labels.find(label) != labels.end()) {
                pc = labels[label];      // Jump to label
                return;                  // Prevent additional increment
            } else {
                cerr << "Error: Undefined label " << label << endl;
            }
        }
        pc++;
    }
};

class Simulator {
public:
    
    int clock;
    vector<int>memory;
    vector<Cores> cores;
    vector<string> program;
    unordered_map<string,int > labels;

    Simulator() {
       memory.resize(4096, 0);
        clock = 0;
        for (int i = 0; i < 4; i++) {
            cores.push_back(Cores(i));
        }
    }
    bool loadprogram(const string& filename ){
        ifstream file(filename);
        if (!file) {
            cerr << "Error: Could not open file " << filename << endl;
            return false;
        }
        string line;
        int lineNumber = 0;
        while(getline(file,line)){
            if(line.empty()){
                continue;
            }
            stringstream ss(line);
            string fristword;
            ss>> fristword;
          //  int length_of_first_word = fristword.size();
            if(fristword.back()==':'){
                labels[fristword.substr(0,fristword.size()-1)]=lineNumber;

            }
            else{
                program.push_back(line);
                lineNumber++;
            }

        }
        file.close();
        return true;

    }

    void run() {
        while (clock < program.size()) {
            for (int i = 0; i < 4; i++) {
                cores[i].execute(program,memory,labels);
            }
            clock++;
        }
    }

    void display() {
        ofstream outfile("output.txt");
        cout << "Register States:\n";
        for (int i = 0; i < 4; i++) {
            cout << "Core " << i << ": ";
            outfile << "Core " << i << ": ";
            for (int j = 0; j < 32; j++) {
                cout << setw(3) << cores[i].registers[j] << " ";
                outfile << setw(3) << cores[i].registers[j] << " ";
            }
            cout << endl;
            outfile << endl;
        }
        cout << "memory States:\n";
        for (int i = 0; i < 4; i++) {
            cout << "Core " << i << ": ";
           
            for (int j = 0; j < 9; j++) {
                cout << setw(3) << cores[i].memo[j] << " ";
               
            }
            cout << endl;
            
        }
    }
   
};

int main() {
    Simulator sim;
    if (!sim.loadprogram("in.asm")) {
        return 1;
    }
   sim.run();
    sim.display();

    cout << "Number of clock cycles: " << sim.clock << endl;
    return 0;
}
