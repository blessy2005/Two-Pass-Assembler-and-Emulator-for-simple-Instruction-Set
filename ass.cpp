#include <bits/stdc++.h>
using namespace std;

struct Instruction { 
// Holds ASM instructions with components such as label, command, operand, etc.
    string label;
    string command;
    string argument;
    int argType;
    bool hasLabel;
};

vector<Instruction> instructions;  // Instructions vector
map<string, pair<string, int>> opcodeMap;  // Mnemonics and OPCodes
vector<pair<int, string>> errorLog;  // Error tracking
vector<string> formattedCode;  // Code stripped of spaces and comments
map<string, int> labelMap;  // Label mapping with line positions
vector<pair<int, string>> machineInstructions;  // Machine code and metadata for listing
vector<int> pcValues;  // Program counter for each line
bool hasHalt = false;  // Flag for HALT command presence
string inputFileName;  // Input file name

bool isOctalNum(string s);
bool isHexadecimalNum(string s);
bool isDecimalNum(string s);
bool isValidLabel(string name);
string decToHexadecimal(int number, int bits = 24);

void initializeOpcodes() {
    // Initialization of the mnemonic-opcode mappings
    opcodeMap["data"] = {"", 1};
    opcodeMap["ldc"] = {"00", 1}; 
    opcodeMap["adc"] = {"01", 1}; 
    opcodeMap["ldl"] = {"02", 2}; 
    opcodeMap["stl"] = {"03", 2};
    opcodeMap["ldnl"] = {"04", 2}; 
    opcodeMap["stnl"] = {"05", 2}; 
    opcodeMap["add"] = {"06", 0};
    opcodeMap["sub"] = {"07", 0}; 
    opcodeMap["shl"] = {"08", 0}; 
    opcodeMap["shr"] = {"09", 0}; 
    opcodeMap["adj"] = {"0A", 1};
    opcodeMap["a2sp"] = {"0B", 0}; 
    opcodeMap["sp2a"] = {"0C", 0}; 
    opcodeMap["call"] = {"0D", 2};
    opcodeMap["return"] = {"0E", 0}; 
    opcodeMap["brz"] = {"0F", 2}; 
    opcodeMap["brlz"] = {"10", 2};
    opcodeMap["br"] = {"11", 2}; 
    opcodeMap["HALT"] = {"12", 0}; 
    opcodeMap["SET"] = {"", 1};
}

void logError(int line, string errorType) {
    errorLog.push_back({line + 1, "Error at line: " + to_string(line) + " -- Type: " + errorType});
}

string trimSpaces(string line, int lineNum) {
    // Trims spaces and processes comments
    line.erase(remove(line.begin(), line.end(), '\t'), line.end());  // remove tabs
    line.erase(find(line.begin(), line.end(), ';'), line.end());  // strip comments
    line.erase(0, line.find_first_not_of(" "));
    line.erase(line.find_last_not_of(" ") + 1);

    int spaces = count(line.begin(), line.end(), ' ');
    if (spaces > 2) logError(lineNum, "Invalid syntax");
    return line;
}

void handleSET(vector<string>& tempInstructions, string label, string content, int position) {
    // Handle SET pseudo-instruction by translating to appropriate commands
    if (content.size() <= position + 5) return;
    
    tempInstructions.push_back("adj 10000");
    tempInstructions.push_back("stl -1");
    tempInstructions.push_back("stl 0");
    tempInstructions.push_back("ldc " + content.substr(position + 6));
    tempInstructions.push_back("ldc " + label.substr(0, position));
    tempInstructions.push_back("stnl 0");
    tempInstructions.push_back("ldl 0");
    tempInstructions.push_back("ldl -1");
    tempInstructions.push_back("adj -10000");
}

void resolveSET() {
    vector<string> processedInstructions;
    for (const auto& line : formattedCode) {
        string current;
        bool modified = false;
        for (size_t j = 0; j < line.size(); ++j) {
            current += line[j];
            if (line[j] == ':') {
                current.pop_back();
                if (line.size() > j + 5 && line.substr(j + 2, 3) == "SET") {
                    modified = true;
                    handleSET(processedInstructions, current, line, j);
                    break;
                }
            }
        }
        if (!modified) processedInstructions.push_back(line);
    }
    formattedCode = processedInstructions;
}

void labelProcessor() {
    // Process and track label declarations in the code
    for (size_t i = 0; i < formattedCode.size(); ++i) {
        string currentLabel;
        for (char c : formattedCode[i]) {
            if (c == ':') {
                if (!isValidLabel(currentLabel)) {
                    logError(i, "Invalid label name");
                    break;
                }
                labelMap[currentLabel] = i;
                break;
            }
            currentLabel += c;
        }
    }
}

void addToTable(int idx, string lbl, string cmd, string arg, int type) {
    instructions[idx].label = lbl;
    instructions[idx].command = cmd;
    instructions[idx].argument = arg;
    instructions[idx].argType = type;
}

void makeSegments() {
    vector<string> codeSeg, dataSeg;
    for (const auto& line : formattedCode) {
        if (line.find("data") != string::npos || line.find(":") != string::npos) {
            dataSeg.push_back(line);
        } else {
            codeSeg.push_back(line);
        }
    }
    codeSeg.insert(codeSeg.end(), dataSeg.begin(), dataSeg.end());
    formattedCode = codeSeg;
}

int detectArgType(string s) {
    if (s.empty()) return 0;
    if (s[0] == '+' || s[0] == '-') s.erase(0, 1);
    if (s.empty()) return -1;
    if (isDecimalNum(s)) return 10;
    if (isOctalNum(s)) return 8;
    if (isHexadecimalNum(s)) return 16;
    if (isValidLabel(s)) return 1;
    return -1;
}

void initialPass() {
    cout << "Enter ASM file name to assemble:" << endl;
    cin >> inputFileName;
    ifstream file(inputFileName);
    if (!file) {
        cout << "File not found!" << endl;
        exit(1);
    }

    string line;
    while (getline(file, line)) {
        string trimmedLine = trimSpaces(line, formattedCode.size());
        formattedCode.push_back(trimmedLine);
    }

    initializeOpcodes();
    labelProcessor();
    if (errorLog.empty()) resolveSET();
    instructions.resize(formattedCode.size());
    pcValues.resize(formattedCode.size());
    makeSegments();
}

bool reportErrors() {
    ofstream errFile("logFile.log");
    if (errorLog.empty()) {
        cout << "No errors found!" << endl;
        if (!hasHalt) errFile << "Warning: HALT not found!" << endl;
        return true;
    }
    for (const auto& err : errorLog) errFile << err.second << endl;
    return false;
}

void secondPass() {
    for (size_t i = 0; i < instructions.size(); ++i) {
        if (formattedCode[i].empty()) continue;
        string address = decToHexadecimal(pcValues[i]);
        
        if (instructions[i].command == "") {
            machineInstructions.push_back({i, "        "});
            continue;
        }

        string machineCode;
        if (instructions[i].argType == 1) {
            int value = labelMap[instructions[i].argument];
            int decValue = value - (pcValues[i] + 1);
            machineCode = decToHexadecimal(decValue) + opcodeMap[instructions[i].command].first;
        } else if (instructions[i].argType == 0) {
            machineCode = "000000" + opcodeMap[instructions[i].command].first;
        } else {
            int decValue = stoi(instructions[i].argument, nullptr, instructions[i].argType);
            machineCode = decToHexadecimal(decValue, 32) + opcodeMap[instructions[i].command].first;
        }
        machineInstructions.push_back({i, machineCode});
    }
}

void writeOutput() {
    ofstream listFile("listCode.l");
    for (const auto& inst : machineInstructions) {
        listFile << decToHexadecimal(pcValues[inst.first]) << " " << inst.second << " " << formattedCode[inst.first] << endl;
    }
    ofstream machineFile("machineCode.o", ios::binary);
    for (const auto& inst : machineInstructions) {
        if (inst.second.empty()) continue;
        unsigned int value = stoul(inst.second, nullptr, 16);
        machineFile.write(reinterpret_cast<const char*>(&value), sizeof(unsigned int));
    }
}

int main
