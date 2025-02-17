#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

void processFile(const string& inputFile, const string& outputFile) {
    ifstream inFile(inputFile);
    ofstream outFile(outputFile);
    
    if (!inFile || !outFile) {
        cerr << "Error opening file!" << endl;
        return;
    }
    
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string word, result;
        
        while (ss >> word) {
            size_t openParen = word.find('(');
            size_t closeParen = word.find(')');
            
            if (openParen != string::npos && closeParen != string::npos) {
                string offset = word.substr(0, openParen);
                string reg = word.substr(openParen + 1, closeParen - openParen - 1);
                result += offset + " " + reg + " ";
            } else {
                result += word + " ";
            }
        }
        
        outFile << result << endl;
    }
    
    inFile.close();
    outFile.close();
}

int main() {
    processFile("pro.asm", "out.asm");
    cout << "File processing complete. Check input.asm" << endl;
    return 0;
}
