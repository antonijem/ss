#ifndef SECTION_HPP
#define SECTION_HPP

#include <string>
#include <iostream>
#include <list>
#include <sstream>
#include "symTable.hpp"
#include <iomanip>
#include <algorithm>
#include <stack>
#include <vector>

#define VALUE 5
#define ADDRESS 6 

#define CONCAT_SYMBOLS(symbol) (std::string(1, symbol[symbol.size() - 5]) + " " + std::string(1, symbol[symbol.size() - 3]) + std::string(1, symbol[symbol.size() - 2])) + " "

#define DISPL(displ) (string(1, displ[displ.size()-3])+ " " + string(1, displ[displ.size()-2]) + string(1, displ[displ.size()-1]) + " ") 
#define REGNUMBER(number) (string(1, number[number.size()-1]))

using namespace std;

struct Section{
    string name;
    int size;
    string instructions;
};


list<Section> table;

struct dataSection{
    string name;
    int size;
    string values;
};

list<dataSection> dataTable;


struct literal{
    string name;
    string value;
    long offset;
};

static int literalPoolOffset = 0;

list <literal> literalPool;


void addSection(string name){
    Section newSection;
    newSection.name = name;
    newSection.size = 0;
    table.push_back(newSection);
}

void addInstruction(string sectionName, string instruction){
    for (auto& section : table){
        if(section.name == sectionName){
            section.instructions.append(instruction);
            section.size += 4;
        }
    }
}


string removeSpaces(const string& input) {
    string result = input;
    result.erase(remove_if(result.begin(), result.end(), [](char c) {
        return isspace(c);
    }), result.end());
    return result;
}

string addSpaces(const string& input) {
    string result;
    for (size_t i = 0; i < input.length(); i += 2) {
        result += input.substr(i, 2) + " ";
    }
    return result;
}

string RegularHexToLittleEndia(const string& littleEndianHex) {
    
    string regularHex = removeSpaces(littleEndianHex);

    // Reverse the string
    reverse(regularHex.begin(), regularHex.end());

    // Swap adjacent characters to restore the original order
    for (size_t i = 0; i < regularHex.length(); i += 2) {
        swap(regularHex[i], regularHex[i + 1]);
    }

    return addSpaces(regularHex);
}

//Funkcije vezane vezabe za LITERAL POOL ----------------

string pomocna(const string& input) {
    string paddedInput = input;
    
    if (paddedInput.length() < 8) {
        paddedInput = string(8 - paddedInput.length(), '0') + paddedInput;
    }
    
    string output;
    int count = 0;
    bool previousSpace = false;

    for (char c : paddedInput) {
        if (c == ' ') {
            output += c;
            previousSpace = true;
        } else {
            if (!previousSpace && count % 2 == 0 && count > 0) {
                output += ' ';
            }
            output += c;
            count++;
            previousSpace = false;
        }
    }

    return output;
}



void addLiteralToLiteralPool(string value, string section, int offset){
    literal newLit;
    if(value.size() != 10){
        value = removeSpaces(value);
        value = pomocna(value);
    }
    newLit.value = RegularHexToLittleEndia(value);
    if(!literalPool.empty()){
        newLit.offset = literalPool.back().offset + 4;
    }else{     
        addSection("literalPool");
        newLit.offset = 0;
    }
    value.erase(remove(value.begin(), value.end(), ' '), value.end());
    newLit.name = value;
    literalPool.push_back(newLit);
    addSymbolToRelocationTable(newLit.name, section, offset, VALUE);
    addInstruction("literalPool", newLit.value + " ");
    
    
}

void printLiteralPool() {
    cout << "LiteralPool" << endl;
    cout << string(70, '-') << endl;
     // Write header line
    cout << left << setw(20) << "Name"
               << setw(20) << "Value"
               << setw(20) << "Offset" << endl;

    // Write symbol table data
    for (const auto& symbol : literalPool) {
        cout << left << setw(20) << symbol.name
                   << setw(20) << symbol.value 
                   << setw(20) << symbol.offset << endl;
    }
    cout << string(70, '-') << endl;
}


void writeLiteralPoolToFile(ofstream& outputFile) {
    outputFile << "LiteralPool" << endl;
    outputFile << string(70, '-') << endl;
     // Write header line
    outputFile << left << setw(20) << "Name"
               << setw(20) << "Value" 
               << setw(20) << "Offset" << endl;

    // Write symbol table data
    for (const auto& symbol : literalPool) {
        outputFile << left << setw(20) << symbol.name
                   << setw(20) << symbol.value 
                   << setw(20) << symbol.offset << endl;
    }
    outputFile << string(70, '-') << endl;
}

void readLiteralPoolFromFile(string line){
    istringstream iss(line);
    string name;
    string value;
    string pom;
    string offsetText;
    iss >> name;
    iss >> value;
    value += " ";
    for(int i=0; i<3 ; i++){ // 11 22 33 44 
        iss >> pom;
        value += pom;
        value += " ";
    }                  
    iss >> offsetText;
    iss >> offsetText;
    int offset = stoi(offsetText);
    if(!literalPool.empty()){
        offset = literalPool.back().offset + 4;
    }
    literal newLit;
    newLit.name = name;
    newLit.value = value;
    newLit.offset = offset;

    literalPool.push_back(newLit);

}

bool doesLiteralExistLiteralPool(string name){
    for(auto& lit : literalPool){
        if(lit.name == name){
            return true;
        }
    }
    return false;
}

long getOffsetLiteralPool(string name){
    for(auto& lit : literalPool){
        if(lit.name == name){
            return lit.offset;
        }
    }
    cout << "Greška prilikom dohvatanja offseta iz literal pool-a. " << name << endl;
    exit(-1);
}

//END za LITERAL POOL -----------------------------------


//Funkcije vezane za DATA SECTION -----------------------

string getFirstSection(){
    return table.front().name;
}

//Pronalazi symbol u data sekciji 
//Vraca true ako postoji false ako ne postoji simbol
bool findSymbolDataSection(string name){
    for(auto& symbol : dataTable){
        if(symbol.name == name){
            return true;
        }
    }
    return false;
}

void addSymbolDataSection(string name){
    dataSection symbol;
    symbol.name = name;
    symbol.size = 0;
    symbol.values = "";
    dataTable.push_back(symbol);
}

bool isHexValueLargerThan12Bits(const std::string& hexValue){
    int decimal = 0;
    int sign = 1;
    // Check if the value is negative (if the most significant bit is set)

    if (hexValue.length() > 2 && hexValue[0] >= '8' && hexValue[0] <= 'F') {
        sign = -1;
    }

    std::istringstream hexStream(hexValue);

    std::string hexDigit;
    while (hexStream >> hexDigit) {
        int digitValue;

        std::istringstream digitStream(hexDigit);
        digitStream >> std::hex >> digitValue;

        decimal = decimal * 256 + digitValue;
    }

    // Apply two's complement if the value is negative
    if (sign == -1) {
        decimal = (1 << (hexValue.size() * 4)) - decimal;
    }

    //cout << decimal << " " << sign << " " << endl;
    // Check if the value is larger than 12 bits
    return decimal > 0xfff; // 0xFFF represents 12 bits in hexadecimal
}

string getValueOfSymbolDataSection(string name){
    for(auto& symbol : dataTable){
        if(symbol.name == name){     
            if(isHexValueLargerThan12Bits(symbol.values)){
                cout << "Simbol " << symbol.name << " ima vrednost veću od 12bita. " << endl;
                exit(-1);
            }
            return symbol.values;
        }
    }
    cout << "Simbol nije pronađen! " << name <<  endl;
    exit(-1);
    return "";
}

void addValueToSymbolDataSection(string name, string values, int size){
    for(auto& symbol : dataTable){
        if(symbol.name == name){
            symbol.values += values;
            symbol.size += size;
            return;
        }
    }
}

void printDataTable() {
    cout << "DataTabela" << endl;
    cout << string(70, '-') << endl;
     // Write header line
    cout << left << setw(20) << "Name"
               << setw(12) << "Size"
               << setw(40) << "Value" << endl;

    // Write symbol table data
    for (const auto& symbol : dataTable) {
        cout << left << setw(20) << symbol.name
                   << setw(12) << symbol.size
                   << setw(40) << symbol.values << endl;
    }
    cout << string(70, '-') << endl;
}

//Offset sluzi da odaberem koji deo vrednosti ovog simbola zelim
//Ova f-ja podrazumeva da simbol postoji i vraca vrednost u obliku 00 0
string getSymbolValueDataSection(string name, int offset){
    string ret;
    for(auto& symbol : dataTable){
        if(symbol.name == name){
            ret = symbol.values.substr(offset, 4);
        }
    }
    return ret;
}

string getSymbolValueDataSection(string name){
    string ret;
    for(auto& symbol : dataTable){
        if(symbol.name == name){
            ret = symbol.values;
        }
    }
    return ret;
}

void writeDataTableToFile(ofstream& outputFile) {
    outputFile << "DataTabela" << endl;
    outputFile << string(70, '-') << endl;
     // Write header line
    outputFile << std::left << std::setw(20) << "Name"
               << std::setw(12) << "Size"
               << std::setw(40) << "Value" << endl;

    // Write symbol table data
    for (const auto& symbol : dataTable) {
        outputFile << left << setw(20) << symbol.name
                   << setw(12) << symbol.size
                   << setw(40) << symbol.values << endl;
    }
    outputFile << string(70, '-') << endl;
}

void readDataTableFromFile(string line){
    istringstream iss(line);
    string name;
    string textSize;
    string values;
    iss >> name;
    iss >> textSize;
    int size = stoi(textSize);
    for(int i=0; i< size; i++){
        string temp;
        iss >> temp;
        values += temp + " ";
    }
    
    //Ovde ne moram da proveravam da li je vec defnisan 
    //jer je to odrađeno u tabeli simbola
    addSymbolDataSection(name);
    addValueToSymbolDataSection(name, values, size);
}

//END DATA SECTION ---------------------------------------

void writeSectionTableToFile(ofstream& outputFile) {
    outputFile << "Sections" << endl;
    outputFile << string(70, '-') << endl;
    for (const auto& section : table) {
        outputFile << section.name << " " << section.size << endl;

        std::istringstream iss(section.instructions);
        std::string instruction;
        int count = 0;
        while (iss >> instruction) {
            if (count == 8) {
                outputFile << endl;
                count = 0;
            }
            outputFile << instruction << " ";
            count++;
        }

        outputFile << endl << endl;
    }
}



bool searchSection(string name){
    for(auto& section : table){
        if(section.name == name){
            return true;
        }
    }
    return false;
}

void printTable() {
    cout << "Sections" << endl;
    cout << string(70, '-') << endl;
    for (const auto& section : table) {
        cout << section.name << " " << section.size << endl;

        istringstream iss(section.instructions);
        string instruction;
        int count = 0;
        while (iss >> instruction) {
            if (count == 8) {
                cout << endl;
                count = 0;
            }
            cout << instruction << " ";
            count++;
        }

        cout << endl << endl;
    }
}

int getSectionSize(string name){
    for(auto& sec : table){
        if(sec.name == name){
            return sec.size;
        }
    }
    cout << "Greška prilikom dohvatanja veličine sekcije " << name << endl;
    exit(-1);
}


void addNonStandardSizeInstruction(string sectionName, string instruction, int size){
    for (auto& section : table){
        if(section.name == sectionName){
            section.instructions.append(instruction);
            section.size += size;
        }
    }
}

string getInstructionFromSectionBasedOnOffset(string sectionName, long offset){
    for(auto& section : table){
        if(section.name == sectionName){
            string result;
            int count = 0;

            for (size_t i = 0; i < section.instructions.length(); i++) {
            if (section.instructions[i] != ' ') {
                if (count >= offset * 2 && count < (offset + 4) * 2) {
                    result += section.instructions[i];
                    result += section.instructions[i + 1];
                    result += ' ';
                }
                    count += 2;
                    i++; // Skip the next character
                }
            }
            return result;
        }
    }
    cout << "Došlo je do greške prilikom dohvatanja instrukcije u sekciji " << sectionName << endl;
    exit(-1);
}

void fixOffsetInInstruction(std::string& name, int offset, const std::string& newValue) {
    string str;

    for(auto& section : table){
        if(section.name == name){
            str = section.instructions;
            str.replace(offset*3, 12, newValue);
            section.instructions = str;
        }
    }

}

string twosComplement(const string& hexNumber) {
    // Remove the "0x" prefix if present
    string number = hexNumber;

    number.erase(remove(number.begin(), number.end(), ' '), number.end());

    if (number.substr(0, 2) == "0x")
        number = number.substr(2);
    
    // Convert hex string to an unsigned long long integer
    unsigned long long decimal = stoull(number, nullptr, 16);
    
    // Compute the two's complement in 32 bits
    unsigned int complement = ~decimal + 1;
    
    // Create a string stream to format the complement as "00 00 01 02"
    stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0') << complement;
    
    // Insert a space between every two characters
    string complementHex = ss.str();
    for (size_t i = 2; i < complementHex.length(); i += 3)
        complementHex.insert(i, " ");

    // Return the two's complement as a hex string

    //cout << complementHex << " debug ch "<< endl;

    return complementHex + " ";
}

string stringToLittleEndianHexNoTwosComplement(const string& inputString) {
    // Check if the input string is already in the desired format

    if (inputString.substr(0, 2) == "0x" && inputString.size() == 10) {
        string pom = inputString.substr(2, inputString.length() - 2);
        string retValue;
        // Add spaces after every two characters
        for (int i = 0; i < pom.length(); i += 2) {
            retValue += pom.substr(i, 2) + " ";
        }
        // Assuming the input string is in little-endian hex format, just return it
        return retValue;
    }else if(inputString.substr(0, 2) == "0x"){ // Hex number just return it without 0x and wiht spaces
        string pom = inputString.substr(2, inputString.size()-2);
        for(size_t i=pom.length(); i<8 ; i++){
            pom.insert(0, "0");
        }
        for (size_t i = 2; i < pom.length(); i += 3)
            pom.insert(i, " ");
        return pom + " ";
    }

    stringstream ss;
    ss << hex << setfill('0');

    // Convert the string to a number
    int number = stoi(inputString);

    if(number > 4294967295){
        cout << "Preveliki skok " << endl;
        exit(-1);
    }

    // Convert the number to hex in little-endian order with spaces
    for (int i = sizeof(int) - 1; i >= 0; --i) {
        unsigned char byte = (number >> (8 * i)) & 0xFF;
        ss << setw(2) << static_cast<int>(byte) << " ";
    }

    // Remove the extra space at the end
    string hexString = ss.str();
    hexString.pop_back();
    hexString += " ";

    return hexString;
}


string stringToLittleEndianHex(const string& inputString) {
    // Check if the input string is already in the desired format

    if (inputString.substr(0, 2) == "0x" && inputString.size() == 10) {
        // Assuming the input string is in little-endian hex format, just return it
        //string pom;
        //pom = twosComplement(inputString);
        return inputString.substr(2, inputString.size() - 2);
    }else if(inputString.substr(0, 2) == "0x"){ // Hex number just return it without 0x and wiht spaces
        string pom = inputString.substr(2, inputString.size()-2);
        for(size_t i=pom.length(); i<8 ; i++){
            pom.insert(0, "0");
        }
        for (size_t i = 2; i < pom.length(); i += 3)
            pom.insert(i, " ");
        return pom + " ";
    }

    stringstream ss;
    ss << hex << setfill('0');

    // Convert the string to a number
    long number = stol(inputString);

    if(number > 4294967295){
        cout << "Preveliki skok " << endl;
        exit(-1);
    }

    // Convert the number to hex in little-endian order with spaces
    for (int i = sizeof(int) - 1; i >= 0; --i) {
        unsigned char byte = (number >> (8 * i)) & 0xFF;
        ss << setw(2) << static_cast<int>(byte) << " ";
    }

    // Remove the extra space at the end
    string hexString = ss.str();
    hexString.pop_back();
    hexString += " ";

    return hexString;
}

//Dodaj razmak izmedju dva char-a ako razmak već postoji ne radi nista
string addSpaces2(const string& input) {
    string output;
    int count = 0;
    bool previousSpace = false;

    for (char c : input) {
        if (c == ' ') {
            output += c;
            previousSpace = true;
        } else {
            if (!previousSpace && count % 2 == 0 && count > 0) {
                output += ' ';
            }
            output += c;
            count++;
            previousSpace = false;
        }
    }

    return output;
}

int littleEndianHexToString(const std::string& hexString) {
    // Remove any spaces in the input string
    string processedString = hexString;
    processedString.erase(remove(processedString.begin(), processedString.end(), ' '), processedString.end());

    // Convert the hex string to an integer
    int number = stoi(processedString, nullptr, 16);

    return number;
}

string stringToLittleEndianAsciiHex(const std::string& inputString) {
  stringstream ss;
  ss << std::hex << std::setfill('0');

  // Convert each character (including spaces) to ASCII code and append it in little-endian hex format
  for (char c : inputString) {
    unsigned int asciiCode = static_cast<unsigned int>(c);
    ss << std::setw(2) << asciiCode << " ";
  }

  return ss.str();
}

//FUNCTIONS FOR CALCULATING EXPRESSIONS------------------------------

int getPrecedence(char op) {
    if (op == '*' || op == '/')
        return 2;
    else if (op == '+' || op == '-')
        return 1;
    else
        return 0;
}

int performOperation(int a, int b, char op) {
    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            return a / b;
        default:
            return 0; // Handle invalid operator
    }
}

long hexToDecimal(const string& hex) {
    long decimal = 0;

    for (size_t i = 2; i < hex.length(); i++) {
        char c = hex[i];
        int digitValue;

        if (isdigit(c)) {
            digitValue = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            digitValue = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            digitValue = c - 'a' + 10;
        } else {
            // Handle invalid hex digit
            return 0;
        }

        decimal = decimal * 16 + digitValue;
    }

    return decimal;
}


int calculateExpression(const string& expression) {
    // Check if the expression is a single number or symbol
    bool isSingleNumberOrSymbol = true;
    for (char c : expression) {
        if ( c == '-' || (!isdigit(c) && c != 'x' && c != 'X' && c != '-' && !isalpha(c) && c != '(' && c != ')')) {
            isSingleNumberOrSymbol = false;
            break;
        }
    }

    if (isSingleNumberOrSymbol) {
        if (expression.length() >= 2 && (expression[0] == '0' && (expression[1] == 'x' || expression[1] == 'X'))) {
            string hexNumber(expression.begin() + 2, expression.end());
            return hexToDecimal("0x" + hexNumber);
        } else if (isdigit(expression[0])) {
            stringstream ss(expression);
            int number;
            ss >> number;
            return number;
        } else {
            long symbolValue = getSymbolOffset(expression);
            if (symbolValue == -1) {
                cout << "Unknown symbol: " << expression << endl;
                exit(-1);
            }
            return symbolValue;
        }
    }

    stack<int> operands;
    stack<char> operators;

    string symSection = "";

    for (size_t i = 0; i < expression.length(); i++) {
        char c = expression[i];

        if (isalpha(c)) {
            string symbolName;
            while (i < expression.length() && (isalnum(expression[i]) || expression[i] == '_')) {
                symbolName += expression[i];
                i++;
            }
            i--; // Decrement to account for the last character in the symbol name
            long symbolValue = getSymbolOffset(symbolName);
            string sec = getSymbolFromSymbolTable(symbolName).section;
            if(symSection == ""){
                if( sec == "?"){
                    cout << "Simbol je eksteran ne može se koristiti u .equ naredbi. " << symbolName << endl;
                    exit(-1);
                }else if(sec != "dataSection"){
                    symSection = sec;
                }
            }else{
                if(symSection != sec){
                    cout << "Da bi mogli da izračunamo .equ naredbu svi simboli moraju da budu iz iste sekcije. " << symbolName << endl;
                    exit(-1);
                }
            }
            if (symbolValue == -1) {
                cout << "Unknown symbol: " << symbolName << endl;
                exit(-1);
            }
            operands.push(symbolValue);
        }else if (isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            string hexNumber;

            if (c == '0' && (i + 1) < expression.length() && (expression[i + 1] == 'x' || expression[i + 1] == 'X')) {
                i += 2; // Skip '0x' or '0X'
                while (i < expression.length() && isxdigit(expression[i])) {
                    hexNumber += expression[i];
                    i++;
                }
                i--; // Decrement to account for the last character in the hexadecimal number
                int decimal = hexToDecimal("0x" + hexNumber);
                operands.push(decimal);
            } else {
                stringstream ss;
                ss << c;
                int number;
                ss >> number;
                operands.push(number);
            }
        } else if (c == '(') {
            operators.push(c);
        } else if (c == ')') {
            while (!operators.empty() && operators.top() != '(') {
                int b = operands.top();
                operands.pop();
                int a = operands.top();
                operands.pop();
                char op = operators.top();
                operators.pop();
                int result = performOperation(a, b, op);
                operands.push(result);
            }

            if (operators.empty() || operators.top() != '(') {
                cout << "Invalid expression: unbalanced parentheses" << endl;
                exit(-1);
            }

            operators.pop(); // Discard the opening parenthesis
        } else if (c == '+' || c == '-' || c == '*' || c == '/') {
            while (!operators.empty() && getPrecedence(c) <= getPrecedence(operators.top()) && operators.top() != '(') {
                int b = operands.top();
                operands.pop();
                int a = operands.top();
                operands.pop();
                char op = operators.top();
                operators.pop();
                int result = performOperation(a, b, op);
                operands.push(result);
            }
            operators.push(c);
        }
    }

    while (!operators.empty()) {
        int b = operands.top();
        operands.pop();
        int a = operands.top();
        operands.pop();
        char op = operators.top();
        operators.pop();
        int result = performOperation(a, b, op);
        operands.push(result);
    }

    if (operands.size() != 1) {
        cout << "Invalid expression" << endl;
        exit(-1);
    }

    return operands.top();
}


//END EXPRESION CALCULATION-------------------------------

string getNumberOfRegister(const string& inputString) {
    string xy;
    // Check if the input string has at least 3 characters
    if (inputString.find("pc") != string::npos){
        return "f"; 
    }else if(inputString.find("sp") != string::npos){
        return "e";
    }else if (inputString.size() >= 3) {
        // Check if the string starts with '%r' and the last two characters are digits
        if (inputString[0] == '%' && inputString[1] == 'r' && isdigit(inputString[2])) {
            xy += inputString[2];

            // Check if the string has a third digit
            if (inputString.size() > 3 && isdigit(inputString[3])) {
                xy += inputString[3];
            }
        }
    }
    
    xy = stringToLittleEndianHex(xy);
    xy = xy.substr(xy.size()-2,1);

    return xy;
}

string getNumberOfRegisterSpecial(const string& inputString, bool& isGPR) {
    string xy;
    // Check if the input string has at least 3 characters
    if (inputString.find("pc") != string::npos){
        return "f"; 
    }else if(inputString.find("sp") != string::npos){
        return "e";
    }else if(inputString.find("status") != string::npos){
        isGPR = false;
        return "0";
    }else if(inputString.find("handler") != string::npos){
        isGPR = false;
        return "1";
    }else if(inputString.find("cause") != string::npos){
        isGPR = false;
        return "2";
    }else if (inputString.size() >= 3) {
        // Check if the string starts with '%r' and the last two characters are digits
        if (inputString[0] == '%' && inputString[1] == 'r' && isdigit(inputString[2])) {
            xy += inputString[2];

            // Check if the string has a third digit
            if (inputString.size() > 3 && isdigit(inputString[3])) {
                xy += inputString[3];
            }
        }
    }
    
    xy = stringToLittleEndianHex(xy);
    xy = xy.substr(xy.size()-2,1);

    return xy;
}


//Proverava da li simbol postoji i sve vezano za nekorektnost simbola
//U slucaju uspeha vratiće ili ? ?? ili x yz kao string
//x,y,z će biti neki brojevi
string returnSymbolOffset(string& sym, string section, int offset, int type){
    string text;
    if(searchSymbol(sym)){
        if(findSymbolDataSection(sym)){
            addSymbolToRelocationTable(sym, section, offset, type);
            text = "? ??";
        }else if(isSymbolExtern(sym)){
            text = "? ?? ";
            addSymbolToRelocationTable(sym, section, offset, type);
        }else if(!sameSection(sym, section)){
            //cout << "Simbol ne pripada istoj sekciji nije moguće izračunati pomeraj. " << sym << endl;
            //exit(-1);
            text = "? ?? ";
            addSymbolToRelocationTable(sym, section, offset, type);
        }else{ //U istoj su sekciji izracunaj gde da učita
            addSymbolToRelocationTable(sym, section, offset, type);
            int pom = getSymbolOffset(sym);
            //cout << pom << " debug symbol offset " << sym << endl;
            string displ = stringToLittleEndianHex(to_string(pom));
            if(isHexValueLargerThan12Bits(displ)){
                cout << "Pomeraj je veći od 12 bita " << displ << endl;
                exit(-1);
            }
            text ="" + string(1,displ[displ.size()-5]) + " " + string(1,displ[displ.size()-3]) + string(1,displ[displ.size()-2]) + " ";
        }
    }else{
        cout << "Nedefinisan simbol! "<< sym <<endl;
        exit(-1);
    }
    return text;
}

string returnSymbolOffsetForJump(string& sym, string section, int offset){
    string text;
    if(searchSymbol(sym)){
        if(!sameSection(sym, section)){
            addSymbolToRelocationTable(sym, section, offset, VALUE);
            return "? ?? ";
        }else{ //U istoj su sekciji izracunaj gde da učita
            addSymbolToRelocationTable(sym, section, offset, VALUE);
            int pom = getSymbolOffset(sym);
            pom -= offset;
            string displ = stringToLittleEndianHex(to_string(pom));
            if(isHexValueLargerThan12Bits(displ)){
                cout << "Pomeraj je veći od 12 bita " << displ << endl;
                exit(-1);
            }
            text ="" + string(1,displ[displ.size()-5]) + " " + string(1,displ[displ.size()-3]) + string(1,displ[displ.size()-2]) + " ";
        }
    }else{
        cout << "Nedefinisan simbol! "<< sym <<endl;
        exit(-1);
    }
    return text;
}


string getNumberOfCSRRegister(const string& inputString){
    string csrReg = inputString.substr(1, inputString.size()-1);
    if(csrReg[csrReg.size()-1] == ',')
        csrReg = csrReg.substr(0, csrReg.size()-1);
    string ret;
    if(csrReg == "status"){
        ret = "0";
    }else if(csrReg == "handler"){
        ret = "1";
    }else if(csrReg == "cause"){  
        ret = "2";
    }
    return ret;
}







void transferSymbolTableIntoSection(){
    addSection("symbolTable");
    for(auto& sym : symTable){
        //cout << sym.name << " ime " << sym.offset << endl;
        if(findSymbolDataSection(sym.name)){
            string val = getSymbolValueDataSection(sym.name);
            //cout << val << " data regular" << endl;
            val = RegularHexToLittleEndia(val);
            //cout << val << " data " << endl;
            addInstruction("symbolTable", val);
        }else{
            addInstruction("symbolTable", RegularHexToLittleEndia(stringToLittleEndianHex(to_string(sym.offset))));
        }
        
    }
}


void addSymbolTableSectionPositionOfSection(){
    long prevOffset = sectionPosition.back().offset + sectionPosition.back().size;
    for(auto& sec : table){
        if(sec.name == "symbolTable"){
            PositionOfSection section;
            section.name = sec.name;
            section.size = sec.size;
            section.offset = prevOffset;
            sectionPosition.push_back(section);
        }
    }
}


bool isValueLargerThan12Bits(const string& value) {
    if (value.empty()) {
        return false;  // Empty value is not larger than 12 bits
    }

    try {
        if (value.substr(0, 2) == "0x") {
            // Hex value
            string hexValue = value.substr(2);  // Remove "0x" prefix

            // Reverse byte order (little-endian)
            stringstream ss;
            for (int i = hexValue.length() - 2; i >= 0; i -= 2) {
                ss << hexValue.substr(i, 2);
            }
            unsigned long long hexNum;
            ss >> hex >> hexNum;

            return hexNum > 0xFFF;  // 0xFFF is the maximum value for 12 bits
        } else {
            // Decimal value
            long long decimalValue = stoll(value);
            return decimalValue > 0xFFF;  // 0xFFF is the maximum value for 12 bits
        }
    } catch (const invalid_argument& e) {
        return false;  // Unable to parse value, assume it is not larger than 12 bits
    } catch (const out_of_range& e) {
        return true;  // Value is larger than the maximum for 12 bits
    }
}

bool isValueLargerThan12Bits2(const string& value) {
    if (value.empty()) {
        return false;  // Empty value is not larger than 12 bits
    }

    try {
        if (value.substr(0, 2) == "0x") {
            // Hex value
            string hexValue = value.substr(2);  // Remove "0x" prefix

            stringstream ss;
            ss << hex << hexValue;
            unsigned long long hexNum;
            ss >> hexNum;

            return hexNum > 0xFFF;  // 0xFFF is the maximum value for 12 bits
        } else {
            // Decimal value
            long long decimalValue = stoll(value);
            return decimalValue > 0xFFF;  // 0xFFF is the maximum value for 12 bits
        }
    } catch (const invalid_argument& e) {
        return false;  // Unable to parse value, assume it is not larger than 12 bits
    } catch (const out_of_range& e) {
        return true;  // Value is larger than the maximum for 12 bits
    }
}


string hexToLittleEndian(const string& hexString) {
    istringstream iss(hexString);
    vector<string> hexValues;
    
    // Tokenize the input string by spaces
    string hexValue;
    while (iss >> hexValue) {
        hexValues.push_back(hexValue);
    }
    
    // Reverse the vector to obtain little endian order
    reverse(hexValues.begin(), hexValues.end());
    
    // Concatenate the hex values with spaces
    string littleEndianHex;
    for (const auto& hex : hexValues) {
        littleEndianHex += hex + " ";
    }
    
    // Remove trailing space
    littleEndianHex.pop_back();
    
    return littleEndianHex + " ";
}

string decimalToHex(const string& decimal) {
    unsigned int decValue;
    stringstream ss(decimal);
    ss >> decValue;

    stringstream hexStream;
    hexStream << hex << decValue;

    string hex = hexStream.str();

    // Pad the hex string with leading zeros if necessary
    while (hex.length() < 8) {
        hex = "0" + hex;
    }

    return hex;
}

string toUpperCase(const string& str) {
    string result;
    result.reserve(str.length());

    for (char c : str) {
        result += toupper(c);
    }

    return result;
}

struct equCalculate{
    string name;
    string expression;
};

list<equCalculate> calculateLater;

int dataSecCnt = 0;

void addEquCalculate(string name, string expression){
    equCalculate eq;
    eq.name = name;
    eq.expression = expression;
    calculateLater.push_back(eq);
}

void calculateAllEquExpressions(){
    string dataSec = "dataSection";
    for(auto& eq : calculateLater){
        int value = calculateExpression(eq.expression);
        string word = to_string(value);
        
       
        addValueToSymbolDataSection(eq.name, stringToLittleEndianHex(word), 4);
        addInstruction(dataSec, hexToLittleEndian(stringToLittleEndianHex(word)));
        addSectionSize(dataSec, dataSecCnt); 
    }
}



void extractInformationLD(long& gprs1, long& gprs2, long& gprd, long& numberV, long& numberA, string& symbol, string& adrSymbol, bool& zagrada ,const std::string& inputString) {
    int cnt = 0;
    bool jReg = false;
    bool comma = false;
    while(cnt < inputString.size()){
        if(inputString[cnt] == ','){
            comma = true;
        }else if(inputString[cnt] == '['){
            zagrada = true;
        }
        if(inputString[cnt] == '$'){
            cnt += 1;
            if(isdigit(inputString[cnt])){
                if(inputString[cnt+1] == 'x'){ // Radim sa hex ciframa
                    string hex = "";
                    while(inputString[cnt] != ' ' && inputString[cnt] != ',' && inputString[cnt] != '[' && inputString[cnt] != ']' && inputString[cnt] != '+'){
                        hex += string(1, inputString[cnt]);
                        cnt++;
                    }
                    numberV = hexToDecimal(hex);
                }else{ //Radim sa decimalnim ciframa
                    string dec = "";
                    while(isdigit(inputString[cnt])){
                        dec += string(1, inputString[cnt]);
                        cnt++;
                    }
                    numberV = stoi(dec);
                }
            }else{
                while(inputString[cnt] != ' ' && inputString[cnt] != '+' && inputString[cnt] != ',' && inputString[cnt] != ']' && inputString[cnt] != '+'){
                    symbol += inputString[cnt];
                    cnt++;
                }
            }
        }else if(isdigit(inputString[cnt])){
            string num = "";
            if(inputString[cnt+1] == 'x'){ // Radim sa hex ciframa
                string hex = "";
                while(inputString[cnt] != ' ' && inputString[cnt] != ',' && inputString[cnt] != '[' && inputString[cnt] != ']' && inputString[cnt] != '+'){
                    hex += string(1, inputString[cnt]);
                    cnt++;
                }
                if(zagrada){
                    numberV = hexToDecimal(hex);
                }else{
                    numberA = hexToDecimal(hex);
                }
            }else{
                while(isdigit(inputString[cnt])){
                    num += string(1, inputString[cnt]);
                    cnt++;
                }
                if(zagrada){
                    numberV = stoi(num);
                }else{
                    numberA = stoi(num);
                }
            }
        }else if(inputString[cnt] == '%' && inputString[cnt+1] == 'r'){
            string num = string(1,inputString[cnt+2]);
            if(isdigit(inputString[cnt+3])){
                num +=string(1,inputString[cnt+3]);
                cnt += 3;
            }else{
                cnt +=2;
            }
            if(comma){
                gprd = stoi(num);
            }else if(!jReg){
                gprs1 = stoi(num);
                jReg = true;
            }else{
                gprs2 = stoi(num);
            }
            
        }else if(inputString[cnt] == '%' && inputString[cnt+1] == 's' && inputString[cnt+2] == 'p' ){
            if(comma){
                gprd = 14;
            }else if(!jReg){
                gprs1 = 14;
                jReg = true;
            }else{
                gprs2 = 14;
            }
        }else if(inputString[cnt] == '%' && inputString[cnt+1] == 'p' && inputString[cnt+2] == 'c' ){
            if(comma){
                gprd = 15;
            }else if(!jReg){
                gprs1 = 15;
                jReg = true;
            }else{
                gprs2 = 15;
            }
        }else if(inputString[cnt] != ' ' && inputString[cnt] != '+' && inputString[cnt] != '[' && inputString[cnt] != ']' && inputString[cnt] != ','){
            while(inputString[cnt] != ' ' && inputString[cnt] != '+' && inputString[cnt] != '[' && inputString[cnt] != ']' && inputString[cnt] != ','){
                adrSymbol += inputString[cnt];
                cnt++;
            }
            //cout<<  adrSymbol;
        }
        if(inputString[cnt] == ','){
            comma = true;
        }
        cnt++;
    }
}

//Instrukcije su little endian tako da je viši bajt na višoj adresi tj 01 00 00 00 -> 1000 0000
int convertToMachine(string section, string line, int &offset, string &curSymbol){
    istringstream iss(line);
    string instruction;
    iss >> instruction;
    if(instruction == "halt"){//Instrukcija prekida
        addInstruction(section, "00 00 00 00 ");
        offset += 4;
        curSymbol = "";
        return 0;
    }else if(instruction == "int"){//Instrukcija softverskog prekida
        addInstruction(section, "10 00 00 00 ");
        offset += 4;
        curSymbol = "";
        return 0;    
    }else if(instruction == ".word"){
        string word;
        iss >> word;
        if(curSymbol == ""){
            cout << "ERROR word instrukcija nije vezana za nikakav simbol!" << endl;
            return -1;
        }
        if(!findSymbolDataSection(word)){ //Prvi put ubacuje neku vrednost u ovaj simbol
            //addSymbolDataSection(word);
        }
        string pom;
        iss >> pom;
        offset += 4;
        while((word[word.size()-1] == ',' || pom == ",")){ //Imam vise symbola
            offset += 4;
            if(word[word.size()-1] == ',')
                word = word.substr(0, word.length()-1);
            if(findSymbolDataSection(curSymbol)){ //Prvi put ubacuje neku vrednost u ovaj simbol
                //addValueToSymbolDataSection(curSymbol, stringToLittleEndianHex(word), 4); // Mogu fiksno 4 da šaljem jer word uvek alocira 4 bajta
            }
            addInstruction(section, stringToLittleEndianHex(word));
            if(pom != ","){
                word = pom;
                if(!(iss >> pom))
                    break;
            }else{
                if(!(iss >> word))
                    break;
                if(!(iss >> pom))
                    break;
            }
        }
        if(findSymbolDataSection(curSymbol)){ //Prvi put ubacuje neku vrednost u ovaj simbol
            //addValueToSymbolDataSection(curSymbol, stringToLittleEndianHex(word), 4); // Mogu fiksno 4 da šaljem jer word uvek alocira 4 bajta
        }
        addInstruction(section, stringToLittleEndianHex(word));
        return 0;
    }else if(instruction == ".skip"){
        if(curSymbol == ""){
            cout << "ERROR skip instrukcija nije vezana za nikakav simbol!" << endl;
            return -1;
        }
        string word;
        iss >> word;
        int number = stoi(word);
        offset += number;
        string text = "";
        for(int i=0; i<number; i++){
            text += "00 ";
        }
        addNonStandardSizeInstruction(section, text, number);
        return 0;
    }else if(instruction == ".ascii"){
        if(curSymbol == ""){
            cout << "ERROR ascii instrukcija nije vezana za nikakav simbol!" << endl;
            return -1;
        }
        if(!findSymbolDataSection(curSymbol)){ //Prvi put ubacuje neku vrednost u ovaj simbol
            //addSymbolDataSection(curSymbol);
        }
        string word;
        string text = "";
        int size = 0;
        while(iss >> word){
            text += word;
            text +=" ";
            size += word.size()-1;
            size++;
        }
        size--;
        size--;
        offset += size;
        text = text.substr(1, text.size() - 3);
        if(findSymbolDataSection(curSymbol)){ //Prvi put ubacuje neku vrednost u ovaj simbol
            //addValueToSymbolDataSection(curSymbol, stringToLittleEndianAsciiHex(text), size); 
        }
        addNonStandardSizeInstruction(section,stringToLittleEndianAsciiHex(text), size);
        return 0;
    }else if(instruction == ".equ") {
        string word;
        iss >> word; // U word se sada nalazi novi simbol
        string dataSec = "dataSection";
        if(!searchSymbol(dataSec)){
            addSymbol(dataSec, SECTION, dataSec, 0, true);
            addSection(dataSec);
        }
        if(word[word.size()-1] == ','){//Ukoliko se uz simbol nalazi zapeta ukloni je
            word = word.substr(0, word.length()-1); 
            addSymbol(word, SYMBOL, dataSec, dataSecCnt, true);
            dataSecCnt += 4;
        }else{
            addSymbol(word, SYMBOL, dataSec, dataSecCnt, true);
            iss >> word;
            dataSecCnt += 4;
        }
        addSymbolDataSection(word);
        string symName = word;
        string text = "";
        while(iss >> word){
            if(word == "#"){
                break;
            }
            text += word;
        }
        addEquCalculate(symName, text);
        //int value = calculateExpression(text);
        //word = to_string(value);
        
       
        //addValueToSymbolDataSection(symName, stringToLittleEndianHex(word), 4);
        //addInstruction(dataSec, hexToLittleEndian(stringToLittleEndianHex(word)));
        //addSectionSize(dataSec, dataSecCnt);
        return 0;
    }else if(instruction == "call"){
        string symbol;
        iss >> symbol;
        string text = "2";
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            text += "0 ";
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                if(symbol.back() == ']'){
                    symbol = getNumberOfRegister(symbol);
                    text += symbol + "0 00 00";
                    addInstruction(section, text);
                    offset += 4;
                    return 0;
                }
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi naredni registar ili simbol         !!!
                if(pom[0] == '%'){ //Imamo još jedan registar
                    if(pom[pom.size()-1] == ']'){ // Nemamo više elemenata
                        pom = getNumberOfRegister(pom.substr(0, pom.size() - 1));
                        text += symbol + pom + " 00 00 ";
                    }else{
                        pom = getNumberOfRegister(pom);
                        string pom2;
                        iss >> pom2; // Tu se sada nalazi ili + ili ]
                        if(pom2 != "]"){//Imamo jos i simbol
                            iss >> pom2;
                            if(pom2[pom2.size()-1] == ']'){
                                pom2 = pom2.substr(0, pom2.size() - 1);   
                            }
                            pom2 = returnSymbolOffset(pom2, section, offset, VALUE);
                            
                            text += symbol + pom + " 0" + pom2;
                        }else{
                            text += symbol + pom + " 00 00 ";
                        }
                    }                    
                }else{ // Imamo simbol
                    if(pom[pom.size() - 1] == ']'){ // Imamo jos ovaj simbol
                        pom = pom.substr(0, pom.size()-1);
                        if(isdigit(pom[0])){
                            pom = stringToLittleEndianHex(pom);
                        }else{
                            pom = returnSymbolOffset(pom, section, offset, VALUE);
                        }
                        if(isHexValueLargerThan12Bits(pom)){
                            cout << "Neposredna vrednost u ld naredbi je veća od 12 bita " << pom << endl;
                            exit(-1); 
                        }
                        text += symbol + "0 0" + pom;
                    }else{
                        if(isdigit(pom[0])){
                            pom = stringToLittleEndianHex(pom);
                        }else{
                            pom = returnSymbolOffset(pom, section, offset, VALUE);
                        }
                        if(isHexValueLargerThan12Bits(pom)){
                            cout << "Neposredna vrednost u ld naredbi je veća od 12 bita " << pom << endl;
                            exit(-1); 
                        }
                        string pom2;
                        iss >> pom2;
                        if(pom2 != "]"){ // Imam jos i registar pored simbola       !!!
                            iss >> pom2;
                            if(pom2[pom2.size() - 1] == ']'){
                                pom2 = pom2.substr(0, pom2.size() - 1);
                            }
                            
                            pom2 = getNumberOfRegister(pom2);
                            text += symbol + pom2 + " 0" + pom;
                        }else{
                            text += symbol + "0 0" + pom;
                        }
                    }    
                }
            }else{//Imamo neki simbol
                if(symbol.back() == ']'){
                    symbol = symbol.substr(0, symbol.size() - 1);
                    if(isdigit(symbol[0])){
                        symbol = stringToLittleEndianHex(symbol);
                    }else{
                        symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                    }
                    text += "00 0"+ symbol;
                    addInstruction(section, text);
                    offset += 4;
                    return 0;
                }
                if(isdigit(symbol[0])){
                    symbol = stringToLittleEndianHex(symbol);
                }else{
                    symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                }
                if(isHexValueLargerThan12Bits(symbol)){
                    cout << "Neposredna vrednost u ld naredbi je veća od 12 bita " << symbol << endl;
                    exit(-1); 
                }
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                if(pom[pom.size() - 1] == ']'){ // Nemamo više ništa
                    pom = pom.substr(0, pom.size() - 1);
                    pom = getNumberOfRegister(pom);
                    text += pom + "0 0" + symbol;
                }else{ //Potencijalno imamo jos elemenata
                    pom = getNumberOfRegister(pom);
                    string pom2;
                    iss >> pom2;
                    if(pom2 == "]"){//Nema vise elemenata                           !!!
                        text += pom + "0 0" + symbol;
                    }else{ // Imamo jos jedan registar
                        iss >> pom2;
                        if(pom2[pom2.size() - 1] == ']'){
                            pom2 = pom2.substr(0, pom2.size() - 1);
                        }
                        pom2 = getNumberOfRegister(pom2);
                        text += pom + pom2 + " 0" + symbol;
                    }
                }
            }
        }else {
            if (symbol[0] != '%'){ // smeštamo u neki simbol
                if(isSymbolExtern(symbol)){
                    text += "0 ";
                    addSymbolToRelocationTable(symbol, section, offset, ADDRESS);
                    symbol = "0 00";
                }else{
                    if(isdigit(symbol[0])){
                        symbol = stringToLittleEndianHex(symbol);
                        //cout << symbol << " call debug " << endl;
                        if(isHexValueLargerThan12Bits(symbol)){ // Ako je adresa poziva veća od 12bita koristimo bazen literala
                            addLiteralToLiteralPool(symbol, section, offset);
                            symbol = "? ??";
                            text += "1 ";
                        }else{
                            text += "0 ";
                            symbol = string(1,symbol[2]) + " " +string(1,symbol[0]) + string(1,symbol[1]) + " ";
                        }
                    }else{
                        text += "0 ";
                        symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
                    }
                }
                text += "00 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
            }else{ // adresa ce se nalaziti u registru
                symbol = getNumberOfRegister(symbol);
                text +=symbol + "0 00 00";
            }
        }
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "xchg"){
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string regS;
        iss >> regS;
        if(regS == ",")
            iss >> regS;
        regS = getNumberOfRegister(regS);
        string word = "40 ";
        word += regD + "0 " + regS + "0 00 ";
        addInstruction(section, word);
        offset += 4;
        return 0;
    }else if(instruction == "add"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "50 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "sub"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "51 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "mul"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "52 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "div"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "53 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "not"){
        string reg;
        iss >> reg;
        reg = getNumberOfRegister(reg);
        string text = "60 " + reg + reg + " 00 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "and"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "61 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "or"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "62 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "xor"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "63 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "shl"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "70 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "shr"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string regD;
        iss >> regD;
        regD = getNumberOfRegister(regD);
        string text = "71 " + regD + regD + " " + regS + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "st"){
        string regS;
        iss >> regS;
        regS = getNumberOfRegister(regS);
        string symbol;
        iss >> symbol;
        string text = "8";
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                if(pom == ""){ // mozda imamo samo registar
                    text += "1 ";
                    text += symbol + "0 " + regS + "0 00 ";
                    addInstruction(section, text);
                    offset += 4;
                    return 0;
                }
                text += "2 ";
                iss >> pom; // Sada se u pom nalazi naredni registar ili simbol         !!!
                if(pom[0] == '%'){ //Imamo još jedan registar
                    if(pom.back() == ']'){ // Nemamo više elemenata
                        pom = getNumberOfRegister(pom.substr(0, pom.size() - 1));
                        text += symbol + pom + " " + regS + "0 00 ";
                    }else{
                        pom = getNumberOfRegister(pom.substr(0, pom.size()));
                        string pom2;
                        iss >> pom2; // Tu se sada nalazi ili + ili ]
                        if(pom2 != "]"){//Imamo jos i simbol
                            iss >> pom2;
                            if(pom2.back() == ']'){
                                pom2 = pom2.substr(0, pom2.size() - 1);   
                            }
                            pom2 = returnSymbolOffset(pom2, section, offset, VALUE);
                            //cout << pom2 << " debug st " << endl;
                            text += symbol + pom + " " + regS + pom2; 
                        }else{
                            text += symbol + pom + " " + regS + "0 00 " ;
                        }
                    }                    
                }else{ // Imamo simbol
                    if(pom.back() == ']'){ // Imamo jos ovaj simbol
                        pom = pom.substr(0, pom.size()-1);
                        pom = returnSymbolOffset(pom, section, offset, VALUE);
                        text +=  symbol + "0 " + regS + pom;
                    }else{
                        pom = returnSymbolOffset(pom, section, offset, VALUE);
                        string pom2;
                        iss >> pom2;
                        if(pom2 != "]"){ // Imam jos i registar pored simbola       !!!
                            iss >> pom2;
                            if(pom2.back() == ']'){
                                pom2 = pom2.substr(0, pom2.size() - 1);
                            }
                            pom2 = getNumberOfRegister(pom2);
                            text += symbol + pom2 + " " + regS + pom;
                        }else{
                            text += symbol + "0 " + regS + pom;
                        }
                    }    
                }
            }else{//Imamo neki simbol
                if(symbol.back() == ']'){ // Imam samo simbol
                    symbol = symbol.substr(0, symbol.size() - 1);
                    symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                    text += "00 " + regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                }else{
                    symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                    string pom;
                    iss >> pom;
                    if(pom != "]"){ //Proveravam da li imam i registar
                        iss >> pom;
                        if(pom.back() == ']'){ // Nemamo više ništa
                            pom = pom.substr(0, pom.size() - 1);
                            pom = getNumberOfRegister(pom);
                            text +=  pom + "0 " + regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                        }else{
                            pom = getNumberOfRegister(pom);
                            string pom2;
                            iss >> pom2;
                            if(pom2 == "]"){ // Proveravam da li imam i drugi registar
                                text +=  pom + "0 " + regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                            }else{
                                iss >> pom2;
                                if(pom2.back() == ']'){
                                    pom2 = pom2.substr(0, pom2.size() - 1);
                                }
                                pom2 = getNumberOfRegister(pom2);
                                text +=  pom  + pom2 + " " + regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                            }
                            
                        }
                    }
                }
            }
        }else if (symbol[0] != '%'){ // smeštamo u neki simbol
            if(isdigit(symbol[0])){
                if(symbol[1] == 'x'){
                        symbol = symbol.substr(2,symbol.size() - 2);
                }else{
                    symbol = decimalToHex(symbol);
                    
                }
                symbol = addSpaces2(symbol) + "";
                addLiteralToLiteralPool(symbol, section, offset);
                text += "2 00 " + regS + "? ?? "; 
            }else{
                symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                string pom;
                iss >> pom; // Ako ima jos elemenata ovo ce biti +
                iss >> pom;
                if(pom[0] == '%'){
                    pom = getNumberOfRegister(pom);
                    string pom2;
                    iss >> pom2;
                    iss >> pom2;
                    if(pom2[0] == '%'){ // Imam 2 registra i simbol koristim modifikator 0
                        pom2 = getNumberOfRegister(pom2);
                        text += "2 " + pom + pom2 + " " + regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                    }else{ //Imam jedan registar i simbol koristim modifikator 1
                        text += "1 " + pom + "0 " + regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                    }
                }else{ // Imam samo simbol koristim modifikator 1?
                    text += "2 00 "+ regS + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                }
            }
        }else{ 
            symbol = getNumberOfRegister(symbol);
            string pom;
            iss >> pom;
            iss >> pom;
            if(pom[0] == '%'){
                pom = getNumberOfRegister(pom);
                string pom2;
                iss >> pom2;
                iss >> pom2;
                if(pom2[0] != '%' && pom2 != ""){ // Imam 2 registra i simbol koristim modifikator 0
                    pom2 = returnSymbolOffset(pom2, section, offset, ADDRESS);
                    text += "0 " + symbol + pom + " " + regS + (pom2.size() > 5 ? CONCAT_SYMBOLS(pom2) : pom2) + " ";
                }else{ //Imam  2 registra i 0 simbol koristim modifikator 0
                    text += "0 " + symbol + pom + " " + regS + "0 00 ";
                }
            }else if(pom != ""){ // Imam samo simbol koristim modifikator 1?
                pom = returnSymbolOffset(pom, section, offset, ADDRESS);
                string pom2;
                iss >> pom2;
                iss >> pom2;
                if(pom2[0] == '%'){ // Imam 2 registra i simbol koristim modifikator 0
                    pom2 = getNumberOfRegister(pom2);
                    text += "0 " + symbol + pom2 + " " + regS + (pom.size() > 5 ? CONCAT_SYMBOLS(pom) : pom) + " ";
                }else{ //Imam jedan  registra i  simbol koristim modifikator 0
                    text += "1 " + symbol + "0 " + regS + (pom.size() > 5 ? CONCAT_SYMBOLS(pom) : pom) + " ";
                }
            }else{
                text +="1 " + symbol + "0 00 00 ";
            }
        }
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "ld"){
        string inst = "";
        string word;
        while(iss >> word){
            if(word == "#"){
                break;
            }
            inst += word;
        }
        //cout << inst << endl;
        long gprs1 = 0, gprs2 = 0, gprd = 0, numV = 0, numA = 0;
        bool zagrada = false;
        string symbolV = "";
        string symbolA = "";
        string text;
        extractInformationLD(gprs1, gprs2, gprd, numV, numA, symbolV, symbolA, zagrada, inst);
        string gprD = decimalToHex(to_string(gprd));
        if(zagrada){
            if(numV > 4095){
                cout << "Neposredna vrednost u ld naredbi je veća od 12bita " << numV << endl;
                exit(-1);
            }
            string gprS1 = decimalToHex(to_string(gprs1));
            string gprS2 = decimalToHex(to_string(gprs2));
            string displ = decimalToHex(to_string(numV));
            text = "92 " + REGNUMBER(gprD) + REGNUMBER(gprS1) + " " + REGNUMBER(gprS2) + DISPL(displ);
            addInstruction(section, text);
            offset += 4;
        }else{
            if(numV != 0){
                string displ = decimalToHex(to_string(numV));   
                if(numV > 4095){
                    displ = addSpaces2(displ);
                    displ = toUpperCase(displ);
                    addLiteralToLiteralPool(displ, section, offset);
                    text = "92 " + REGNUMBER(gprD) + "0 0? ?? ";
                    addInstruction(section, text);
                    offset += 4;
                }else{
                    text = "91 " + REGNUMBER(gprD) + "0 0" + DISPL(displ); 
                    addInstruction(section, text);
                    offset += 4;
                }
            }else if(symbolV != ""){ // sa $ 
                if(!searchSymbol(symbolV)){
                    cout << "Nedefinisan simbol " << symbolV << " Greška u ld naredbi." << endl;
                    exit(-1);
                }
                if(isSymbolExtern(symbolV)){
                    addSymbolToRelocationTable(symbolV, section, offset, VALUE);
                    text = "92 " + REGNUMBER(gprD) + "0 0? ?? ";
                    addInstruction(section, text);
                    offset += 4; 
                }else{
                    addSymbolToRelocationTable(symbolV, section, offset, VALUE);
                    text = "92 " + REGNUMBER(gprD) + "0 0? ?? ";
                    addInstruction(section, text);
                    offset += 4;
                }
            }else if(symbolA != ""){ // bez $
                if(!searchSymbol(symbolA)){
                    cout << "Nedefinisan simbol " << symbolA << " Greška u ld naredbi." << endl;
                    exit(-1);
                }
                if(!findSymbolDataSection(symbolA)){
                    symbolA = returnSymbolOffset(symbolA, section, offset, ADDRESS);
                    text = "92 " + REGNUMBER(gprD) + "0 0" + (symbolA.size() > 5 ? CONCAT_SYMBOLS(symbolA) : symbolA) + " ";
                    addInstruction(section, text);
                    offset += 4;
                }else{
                    symbolA = returnSymbolOffset(symbolA, section, offset, ADDRESS);
                    text = "91 " + REGNUMBER(gprD) + "0 0" + (symbolA.size() > 5 ? CONCAT_SYMBOLS(symbolA) : symbolA) + " ";
                    addInstruction(section, text);
                    offset += 4;
                    text = "92 " + REGNUMBER(gprD) + REGNUMBER(gprD) + " 00 00 ";
                    addInstruction(section, text);
                    offset += 4;
                    text = "92 " + REGNUMBER(gprD) + REGNUMBER(gprD) + " 00 00 ";
                    addInstruction(section, text);
                    offset += 4;
                }
            }else if(numA != 0){
                string displ = decimalToHex(to_string(numA));
                displ = addSpaces2(displ);
                displ = toUpperCase(displ);
                addLiteralToLiteralPool(displ, section, offset);
                text = "92 " + REGNUMBER(gprD) + "0 0? ?? ";
                addInstruction(section, text);
                offset += 4;
                text = "92 " + REGNUMBER(gprD) + REGNUMBER(gprD) + " 00 00 ";
                addInstruction(section, text);
                offset += 4;
            }else {
                string gprS1 = decimalToHex(to_string(gprs1));
                text = "91 " + REGNUMBER(gprD) + REGNUMBER(gprS1) + " 00 00 ";
                addInstruction(section, text);
                offset += 4;
            }
        }
        //cout << text << endl;
        return 0;
    }else if(instruction == "csrrd"){
        string csrReg;
        iss >> csrReg;
        csrReg = getNumberOfCSRRegister(csrReg);
        string gprReg;
        iss >> gprReg;
        gprReg = getNumberOfRegister(gprReg);
        string text = "90 " + gprReg + csrReg + " 00 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "csrwr"){
        string gprReg;
        iss >> gprReg;
        gprReg = getNumberOfRegister(gprReg);
        string csrReg;
        iss >> csrReg;
        csrReg = getNumberOfCSRRegister(csrReg);
        string text = "94 "+ csrReg + gprReg + " 00 00 ";
        offset += 4;
        addInstruction(section, text);
        return 0;
    }else if(instruction == "jmp"){
        string symbol;
        iss >> symbol;
        string text = "3";
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            text += "8 ";
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi  simbol                         !!!
                // Imamo simbol
                if(pom[pom.size() - 1] == ']'){
                    pom = pom.substr(0, pom.size()-1);
                    //cout << pom << " ovde je simbol u jmp" << endl;
                    pom = returnSymbolOffset(pom, section, offset, VALUE);
                }else{
                    pom = returnSymbolOffset(pom, section, offset, VALUE);
                }
                text += symbol + "0 0" + pom + " ";
            }else{//Imamo neki simbol
                symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                pom = pom.substr(0, pom.size() - 1);
                pom = getNumberOfRegister(pom);
                text += pom + "0 0" + symbol + " ";
            }
        }else{
            if (symbol[0] != '%'){ // smeštamo u neki simbol
                symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
                if(symbol == "? ?? "){
                    text+="8 ";
                    text += "00 0" + symbol + " ";
                }else{
                    text+="0 ";
                    text += "00 00 00 ";
                }
            }else{ // adresa ce se nalaziti u registru
                symbol = getNumberOfRegister(symbol);
                text +=symbol + "0 00 00";
            }
        }
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "beq"){
        string regB;
        iss >> regB;
        regB = getNumberOfRegister(regB);
        string regC;
        iss >> regC;
        regC = getNumberOfRegister(regC);
        //Racunanje adrese na koju skacemo 
        string symbol;
        iss >> symbol;
        string text = "31 ";
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi  simbol                         !!!
                // Imamo simbol
                if(pom[pom.size() - 1] == ']'){
                    pom = pom.substr(0, pom.size()-1);
                    //cout << pom << " ovde je simbol u jmp" << endl;
                    pom = returnSymbolOffset(pom, section, offset, VALUE);
                }else{
                    pom = returnSymbolOffset(pom, section, offset, VALUE);
                }
                text += symbol + regB + " " + regC + pom + " ";
            }else{//Imamo neki simbol
                symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                pom = pom.substr(0, pom.size() - 1);
                pom = getNumberOfRegister(pom);
                text += pom + regB + " "+ regC + symbol + " ";
            }
        }else if (symbol[0] != '%'){ // smeštamo u neki simbol
            symbol = returnSymbolOffset(symbol, section, offset, VALUE);
            text += "0"+ regB + " "+ regC + symbol + " ";
        }else{ // adresa ce se nalaziti u registru
            symbol = getNumberOfRegister(symbol);
            text +=symbol + regB + " " + regC + "0 00";
        }
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "bne"){
        string regB;
        iss >> regB;
        regB = getNumberOfRegister(regB);
        string regC;
        iss >> regC;
        regC = getNumberOfRegister(regC);
        //Racunanje adrese na koju skacemo 
        string symbol;
        iss >> symbol;
        string text = "32 ";
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi  simbol                         !!!
                // Imamo simbol
                if(pom[pom.size() - 1] == ']'){
                    pom = pom.substr(0, pom.size()-1);
                    //cout << pom << " ovde je simbol u jmp" << endl;
                    pom = returnSymbolOffset(pom, section, offset, ADDRESS);
                }else{
                    pom = returnSymbolOffset(pom, section, offset, ADDRESS);
                }
                text += symbol + regB + " " + regC + pom + " ";
            }else{//Imamo neki simbol
                symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                pom = pom.substr(0, pom.size() - 1);
                pom = getNumberOfRegister(pom);
                text += pom + regB + " "+ regC + symbol + " ";
            }
        }else if (symbol[0] != '%'){ // smeštamo u neki simbol
            symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
            text += "0"+ regB + " "+ regC + symbol + " ";
        }else{ // adresa ce se nalaziti u registru
            symbol = getNumberOfRegister(symbol);
            text +=symbol + regB + " " + regC + "0 00";
        }
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "bgt"){
        string regB;
        iss >> regB;
        regB = getNumberOfRegister(regB);
        string regC;
        iss >> regC;
        regC = getNumberOfRegister(regC);
        //Racunanje adrese na koju skacemo 
        string symbol;
        iss >> symbol;
        string text = "33 ";
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi  simbol                         !!!
                // Imamo simbol
                if(pom[pom.size() - 1] == ']'){
                    pom = pom.substr(0, pom.size()-1);
                    //cout << pom << " ovde je simbol u jmp" << endl;
                    pom = returnSymbolOffset(pom, section, offset, VALUE);
                }else{
                    pom = returnSymbolOffset(pom, section, offset, VALUE);
                }
                text += symbol + regB + " " + regC + pom + " ";
            }else{//Imamo neki simbol
                symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                pom = pom.substr(0, pom.size() - 1);
                pom = getNumberOfRegister(pom);
                text += pom + regB + " "+ regC + symbol + " ";
            }
        }else if (symbol[0] != '%'){ // smeštamo u neki simbol
            symbol = returnSymbolOffset(symbol, section, offset, VALUE);
            text += "0"+ regB + " "+ regC + symbol + " ";
        }else{ // adresa ce se nalaziti u registru
            symbol = getNumberOfRegister(symbol);
            text +=symbol + regB + " " + regC + "0 00";
        }
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "push"){
        string regC;
        iss >> regC;
        regC = getNumberOfRegister(regC);
        string text = "80 e0 " + regC + "0 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0; 
    }else if(instruction == "pop"){
        string regA;
        iss >> regA;
        regA = getNumberOfRegister(regA);
        string text = "93 " + regA + "e 00 00 "; 
        addInstruction(section, text);
        offset += 4;
        return 0;
    }else if(instruction == "iret"){
        string text = "93 fe 00 00 ";
        addInstruction(section, text);
        text = "93 0e 00 00 ";
        addInstruction(section, text);
        offset += 8;
        return 0;
    }else if(instruction == "ret"){
        string text = "93 fe 00 00 ";
        addInstruction(section, text);
        offset += 4;
        return 0;
    }
    

    cout << "Nije prepoznata naredba! " << instruction <<endl;
    return -1; 
}


#endif  // SECTION_HPP





/*Ovde stoji kod za instrukcije smeštanja ako budu tražili

else if(instruction == "csrrw"){
        string csrReg;
        iss >> csrReg;
        csrReg = getNumberOfCSRRegister(csrReg);
        cout << csrReg << " < - CSRREG" << endl;
        string symbol;
        iss >> symbol;
        string text = "9";
        if(symbol[0] == '['){ // Podatak se nalazi u memoriji
            if(symbol[1] == ' '){
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }// Odavde u nextSymbol sigurno imam ili registar ili simbol
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi naredni registar ili simbol         !!!
                if(pom[0] == '%'){ //Imamo još jedan registar
                    text += "6 "; // Pošto imam 2 registra sigurno nije drugi mod za store instrukciju!!!
                    if(pom[pom.size()-1] == ']'){ // Nemamo više elemenata
                        pom = getNumberOfRegister(pom.substr(0, pom.size() - 1));
                        text += symbol + pom + " " + csrReg + "0 00 ";
                    }else{
                        string pom2;
                        iss >> pom2; // Tu se sada nalazi ili + ili ]
                        if(pom2 != "]"){//Imamo jos i simbol
                            iss >> pom2;
                            if(pom2[pom2.size()-1] == ']'){
                                pom2 = pom2.substr(0, pom2.size() - 1);   
                            }
                            pom2 = returnSymbolOffset(pom2, section, offset);
                            
                            text += pom2;
                        }else{
                            text += symbol + pom + " " + csrReg + "0 00 ";
                        }
                    }                    
                }else{ // Imamo simbol
                    if(pom[pom.size() - 1] == ']'){ // Imamo jos ovaj simbol
                        pom = pom.substr(0, pom.size()-1);
                        pom = returnSymbolOffset(pom, section, offset);
                        text += "7 " + symbol + "0 " + csrReg + pom;
                    }else{
                        pom = returnSymbolOffset(pom, section, offset);
                        string pom2;
                        iss >> pom2;
                        if(pom2 != "]"){ // Imam jos i registar pored simbola       !!!
                            if(pom2[pom2.size() - 1] == ']'){
                                pom2 = pom2.substr(0, pom2.size() - 1);
                            }
                            pom2 = getNumberOfRegister(pom2);
                            text += "6 " + symbol + pom2 + " " + csrReg + pom;
                        }else{
                            text += "7 " + symbol + "0 " + csrReg + pom;
                        }
                    }    
                }
            }else{//Imamo neki simbol
                symbol = returnSymbolOffset(symbol, section, offset);
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                if(pom[pom.size() - 1] == ']'){ // Nemamo više ništa
                    pom = pom.substr(0, pom.size() - 1);
                    pom = getNumberOfRegister(pom);
                    text += "7 " + pom + "0 " + csrReg + symbol;
                }else{ //Potencijalno imamo jos elemenata
                    string pom2;
                    iss >> pom2;
                    if(pom2 == "]"){//Nema vise elemenata                           !!!
                        text += "7 " + pom + "0 " + csrReg + symbol;
                    }else{ // Imamo jos jedan registar
                        if(pom2[pom2.size() - 1] == ']'){
                            pom2 = pom2.substr(0, pom2.size() - 1);
                        }
                        pom2 = getNumberOfRegister(pom2);
                        text += "6 " + pom + pom2 + " " + csrReg + symbol;
                    }
                }
            }

        }else if(symbol[0] != '%'){ //Podatak se nalazi na adresi nekog simbola
            symbol = returnSymbolOffset(symbol, section, offset);
            text += "7 00 0" + symbol + " ";
        }else{ // adresa ce se nalaziti u registru
            symbol = getNumberOfRegister(symbol);
            text += "7 " + symbol + "0 00 00";
        }
        offset += 4;
        addInstruction(section, text);
        return 0;
    }
*/


/*string symbol;
        iss >> symbol;
        string text = "9";
        string text2;
        if(symbol[0] == '['){ // smeštamo u adresu koju cemo tek izracunati (npr. [%r1 + a])
            if(symbol[1] == ' '){//Napisano je kao [ %r1 + a ]
                iss >> symbol;
            }else{
                symbol = symbol.substr(1, symbol.size() - 1);
            }//Odavde u symbol sigurno imam ili registar ili simbol                 !!!
            if(symbol[0] == '%'){//Imamo registar
                symbol = getNumberOfRegister(symbol);
                string pom;
                iss >> pom; // Tu se sada nalazi znak + 
                iss >> pom; // Sada se u pom nalazi naredni registar ili simbol         !!!
                if(pom[0] == '%'){ //Imamo još jedan registar
                    text += "2 "; // Pošto imam 2 registra sigurno nije drugi mod za store instrukciju!!!
                    if(pom.back() == ']' || pom.back() == ','){ // Nemamo više elemenata
                        pom = getNumberOfRegister(pom.substr(0, pom.size() - 1));
                        text2 = symbol + " " + pom + "0 00 ";
                    }else{
                        pom = getNumberOfRegister(pom);
                        string pom2;
                        iss >> pom2; // Tu se sada nalazi ili + ili ]
                        if(pom2 != "]"){//Imamo jos i simbol
                            iss >> pom2;
                            
                            if(pom2.back() == ']' || pom2.back() == ','){
                                if(pom2.back() == ',')
                                    pom2 = pom2.substr(0, pom2.size() - 1);
                                pom2 = pom2.substr(0, pom2.size() - 1);  
                            }
                            if(isdigit(pom2[0])){
                                pom2 = stringToLittleEndianHex(pom2);
                            }else{
                                pom2 = returnSymbolOffset(pom2, section, offset, VALUE);
                            }
                            if(isHexValueLargerThan12Bits(pom2)){
                                cout << "Neposredna vrednost u ld naredbi je veća od 12 bita " << symbol << endl;
                                exit(-1); 
                            }
                            text2 = symbol+ " " + pom + (pom2.size() > 5 ? CONCAT_SYMBOLS(pom2) : pom2);
                        }else{
                            text2 = symbol + " " + pom + "0 00 ";
                        }
                    }                    
                }else{ // Imamo simbol
                    if(pom.back() == ']' || pom.back() == ','){ // Imamo jos ovaj simbol
                        if(pom.back() == ','){
                            pom = pom.substr(0, pom.size()-1);
                        }
                        pom = pom.substr(0, pom.size()-1);
                        if(isdigit(pom[0])){
                            pom = stringToLittleEndianHex(pom);
                        }else{
                            pom = returnSymbolOffset(pom, section, offset, VALUE);
                        }
                        if(isHexValueLargerThan12Bits(pom)){
                            cout << "Neposredna rednost u ld naredbi je veća od 12 bita " << pom << endl;
                            exit(-1);
                        }
                        text += "2 ";
                        text2 = symbol + " 0" + (pom.size() > 5 ? CONCAT_SYMBOLS(pom) : pom);
                    }else{
                        pom = pom.substr(1, pom.size()-1);
                        pom = returnSymbolOffset(pom, section, offset, VALUE);
                        string pom2;
                        iss >> pom2;//  Ovde treba da se nađe sada +
                        iss >> pom2;
                        if(pom2 != "]"){ // Imam jos i registar pored simbola       !!!
                            if(pom2.back() == ']' || pom2.back() == ','){
                                if(pom2.back() == ',')
                                    pom2 = pom2.substr(0, pom2.size()-1);
                                pom2 = pom2.substr(0, pom2.size() - 1);
                            }
                            pom2 = getNumberOfRegister(pom2);
                            text += "2 ";
                            text2 = symbol + " " + pom2 + (pom.size() > 5 ? CONCAT_SYMBOLS(pom) : pom) + " ";
                        }else{
                            text += "2 ";
                            text2 = symbol + " 0" + (pom.size() > 5 ? CONCAT_SYMBOLS(pom) : pom) + " ";
                        }
                    }    
                }
            }else{//Imamo neki simbol
                //symbol = symbol.substr(1, symbol.size());
                if(isdigit(symbol[0])){
                    symbol = stringToLittleEndianHex(symbol);
                }else{
                    symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                }
                if(isHexValueLargerThan12Bits(symbol)){
                    cout << "Neposredna rednost u ld naredbi je veća od 12 bita " << symbol << endl;
                    exit(-1); 
                }
                string pom;
                iss >> pom; // Tu se sada nalazi sigurno + jer gramatika nalaže da ako ide samo simbol onda ide bez []
                iss >> pom; // Tu se sada nalazi sigurno registar 
                if(pom.back() == ']' || pom.back() == ','){ // Nemamo više ništa
                    if(pom.back() == ',')
                        pom = pom.substr(0, pom.size()-1);
                    pom = pom.substr(0, pom.size() - 1);
                    pom = getNumberOfRegister(pom);
                    text += "2 ";
                    text2 = pom + " 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol);
                }else{ //Potencijalno imamo jos elemenata
                    pom = getNumberOfRegister(pom);
                    string pom2;
                    iss >> pom2;
                    if(pom2 == "]"){//Nema vise elemenata                           !!!
                        text += "2 ";
                        text2 = pom + " 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol);
                    }else{ // Imamo jos jedan registar
                        iss >> pom2;
                        if(pom2.back() == ']' || pom2.back() == ','){
                            if(pom2.back() == ','){
                                pom2 = pom2.substr(0, pom2.size()-1);
                            }
                            pom2 = pom2.substr(0, pom2.size() - 1);
                        }
                        pom2 = getNumberOfRegister(pom2);
                        text += "2 ";
                        text2 = pom + " " +  pom2 + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol);
                    }
                }
            }
        }else if (symbol[0] != '%'){ // smeštamo u neki simbol
            if(symbol.back() == ','){ 
                symbol = symbol.substr(0, symbol.size()-1); // Ovim uklanjam , ukoliko je odmah uz ime simbola
            }
                if(symbol[0] == '$'){ //Imam vrednost simbola
                    symbol = symbol.substr(1,symbol.size());
                    if(isdigit(symbol[0])){ //Proveravam da li je stavljena neposredna vrednost
                        if(isValueLargerThan12Bits2(symbol)){
                            //cout << "Neposredna rednost u ld naredbi je veća od 12 bita " << symbol << endl;
                            //exit(-1);
                            cout << symbol << " u ld naredbi veci od 12 bita" << endl;
                            if(symbol[1] == 'x'){
                                symbol = symbol.substr(2,symbol.size() - 2);
                            }else{
                                symbol = decimalToHex(symbol);
                                
                            }
                            symbol = addSpaces2(symbol) + "";
                            addLiteralToLiteralPool(symbol, section, offset);
                            text += "2 ";
                            symbol = "? ??"; 
                            text2 += "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                            string regA;
                            iss >> regA;
                            if(regA == ","){
                                iss >> regA;
                            }
                            regA = getNumberOfRegister(regA);
                            text += regA + text2 + " ";
                            addInstruction(section, text);
                            offset += 4;
                            return 0;
                        }else{
                            if(symbol[1] == 'x'){
                                symbol = symbol.substr(2,symbol.size() - 2);
                            }else{
                                symbol = decimalToHex(symbol);
                            }
                            text +="1 ";
                            cout << symbol << " ld debgu " << endl;
                            text2 += "0 0" + (symbol.size() > 4 ? (string(1, symbol[symbol.size()-3]) + " " +string(1, symbol[symbol.size()-2]) + string(1, symbol[symbol.size()-1])) : (symbol.size() < 2 ? "0 0"+ symbol : (symbol.size() < 3 ? "0 " + symbol : symbol))) + " ";
                        }
                    }else{
                        if(!searchSymbol(symbol)){
                            cout << "Nedefinisan simbol "<< symbol <<"!!! Greška u ld naredbi" << endl;
                            return -1;
                        }
                        if(isSymbolExtern(symbol)){ //Simbol je eksteran, linker ce ovo resiti, zbog toga će ići kombinacija 91 92 učitavanja podataka jer ne znam da li je eksteran simbola većo od 12 bita
                            text += "2 ";
                            addSymbolToRelocationTable(symbol, section, offset, VALUE);
                            symbol = "? ??";
                            text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";*/
                            /*text += "1 ";
                            addSymbolToRelocationTable(symbol, section, offset, VALUE);
                            symbol = "? ??";
                            text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                            string regA;
                            iss >> regA;
                            if(regA == ","){
                                iss >> regA;
                            }
                            regA = getNumberOfRegister(regA);
                            text += regA + text2 + " ";
                            //Prvo sam dodao naredbu za učitavanje podataka 91 jer ne znam da li je eksterni simbola ima vrednost veću od 12bita
                            addInstruction(section, text);
                            offset += 4;
                            text = "92 " + regA + regA + " 00 00 ";
                            offset += 4;
                            //Dodajem naredbu za učitavanje podataka 92 gde mi je registar za upis i registar iz kog čitam isti jer se sada u njemu nalazi adresa simbola
                            addInstruction(section, text);
                            return 0;*/
                       /* }else{ //                   Simbol je lokalan dohvati njegovu vrednost
                            text += "2 ";
                            symbol = returnSymbolOffset(symbol, section, offset, VALUE);
                            text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                        }
                    }
                }else{
                    if(isdigit(symbol[0])){
                        if(symbol[1] == 'x'){
                                symbol = symbol.substr(2,symbol.size() - 2);
                        }else{
                            symbol = decimalToHex(symbol);
                            
                        }
                        symbol = addSpaces2(symbol) + "";
                        addLiteralToLiteralPool(symbol, section, offset);
                        text += "2 ";
                        symbol = "? ??"; 
                        text2 += "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                        string regA;
                        iss >> regA;
                        if(regA == ","){
                            iss >> regA;
                        }
                        regA = getNumberOfRegister(regA);
                        text += regA + text2 + " ";
                        addInstruction(section, text);
                        offset += 4;
                        return 0;
                    }else{
                        if(!searchSymbol(symbol)){
                            cout << "Nedefinisan simbol "<< symbol <<"!!! Greška u ld naredbi" << endl;
                            return -1;
                        }
                        if(!findSymbolDataSection(symbol)){
                            text += "2 ";
                            symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
                            text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                        }else{
                            symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
                            text += "1 ";
                            text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                            string regA;
                            iss >> regA;
                            if(regA == ","){
                                iss >> regA;
                            }
                            regA = getNumberOfRegister(regA);
                            text += regA + text2 + " ";
                            addInstruction(section, text);
                            offset += 4;
                            text = "92 " + regA + regA + " 00 00 ";
                            offset += 4;
                            addInstruction(section, text);
                            text = "92 " + regA + regA + " 00 00 ";
                            offset += 4;
                            addInstruction(section, text);
                            return 0;
                            //text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " "; 
                        }
                    }
            }/*else{
                if(symbol[0] == '$'){ //Imam vrednost simbola
                        text += "1 ";
                        symbol = symbol.substr(1,symbol.size());
                        if(!searchSymbol(symbol)){
                            cout << "Nedefinisan simbol "<< symbol <<"!!! Greška u ld naredbi" << endl;
                            return -1;
                        }
                        if(isSymbolExtern(symbol)){ //Simbol je eksteran, linker ce ovo resiti
                            addSymbolToRelocationTable(symbol, section, offset, VALUE);
                            symbol = "? ??";
                        }else{ //                   Simbol je lokalan dohvati njegovu vrednost
                            symbol = getValueOfSymbolDataSection(symbol);
                        }
                }else{
                    text += "2 ";
                    if(!searchSymbol(symbol)){
                        cout << "Nedefinisan simbol "<< symbol <<"!!! Greška u ld naredbi" << endl;
                        return -1;
                    }
                    if(sameSection(symbol, section)){ // Ako su u istoj sekciji moze da se izracuna pomeraj do te adrese
                        symbol = returnSymbolOffset(symbol, section, offset, ADDRESS);
                    }else{ //Ako nisu u istoj sekciji ostavljamo linkeru da to razresi, a mi popunjavamo relokacionu tabelu
                        addSymbolToRelocationTable(symbol, section, offset, ADDRESS);
                        symbol = "?? ?";
                    }
                }
                
                string nextElem;
                iss >> nextElem;
                if(nextElem != ","){
                    string onlyReg;
                    iss >> onlyReg;
                    if(onlyReg[0] != '%'){
                        cout << "U ld naredbi ako se koristi vrednost simbola dozvoljeno je samo sabiranje sa vrednosti registra!" << endl;
                        return -1;
                    }
                    if(onlyReg[onlyReg.size()-1] == ','){
                        onlyReg = onlyReg.substr(0, onlyReg.size()-1);
                    }
                    onlyReg = getNumberOfRegister(onlyReg);
                    text2 = onlyReg + " 0" +  (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                }else{ // Nemamo više elemenata
                    text2 = "0 0" + (symbol.size() > 5 ? CONCAT_SYMBOLS(symbol) : symbol) + " ";
                }
            }*/
        /*}else{ // adresa ce se nalaziti u registru
            bool isGPR = true;
            symbol = getNumberOfRegisterSpecial(symbol, isGPR);
            if(isGPR){
                text += "1 ";            
            }else{
                text += "0 ";           
            }
            text2 = symbol + " 00 00";
        }
        string regA;
        iss >> regA;
        if(regA == ","){
            iss >> regA;
        }
        regA = getNumberOfRegister(regA);
        text += regA + text2 + " ";
        addInstruction(section, text);
        offset += 4;
        return 0;*/