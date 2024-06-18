#ifndef EMULAOTR_HPP
#define EMULATOR_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <sys/poll.h>



using namespace std;

struct MemoryAddress{
    string address;
    string value;
};

list<MemoryAddress> memory;

 auto startTime = std::chrono::steady_clock::now();// za tajmer 

//Registri----------------------

const string r0 ="00000000";
string r1 ="00000000";
string r2 ="00000000";
string r3 ="00000000";
string r4 ="00000000";
string r5 ="00000000";
string r6 ="00000000";
string r7 ="00000000";
string r8 ="00000000";
string r9 ="00000000";
string r10 ="00000000";
string r11 ="00000000";
string r12 ="00000000";
string r13 ="00000000";
string sp ="00000000";
string pc ="00000000";

string status = "00000000";
string handler = "00000000";
string cause = "00000000";

string value;
//end Registri--------------------

bool isHALT = false;
bool terminalOutput = false;

void setStatusMask(int mask){
    int num = stoi(string(1,status[1]));
    num = num & mask;
    status[1] = '0' + mask;
    //cout << "status u set mask: " << status << endl;
}

int checkStatusMask(){
    return stoi(string(1, status[1]));
}

string toUpperCase(const string& str) {
    string result;
    result.reserve(str.length());

    for (char c : str) {
        result.push_back(toupper(c));
    }

    return result;
}

string regularToLittleEndianHex(const string& regularHex) {
    string littleEndianHex;

    for (int i = regularHex.length() - 2; i >= 0; i -= 2) {
        string byte = regularHex.substr(i, 2);
        littleEndianHex += byte;
    }

    return littleEndianHex;
}


int hexCharToInt(char hexChar) {
    hexChar = tolower(hexChar);
    int intValue = 0;

    if (isdigit(hexChar)) {
        intValue = hexChar - '0';
    } else if (hexChar >= 'a' && hexChar <= 'f') {
        intValue = hexChar - 'a' + 10;
    }

    return intValue;
}

void setStatusFirstBitToZero(){
    status[1] = '0';
}

string performHexNot(const string& hexValue) {
    // Convert the input hex string to an unsigned 32-bit integer
    stringstream ss;
    ss << hex << hexValue;
    unsigned int intValue;
    ss >> intValue;

    // Perform bitwise NOT operation
    unsigned int result = ~intValue;

    // Convert the result back to a hex string
    stringstream ssResult;
    ssResult << hex << setw(8) << setfill('0') << result;
    string hexResult = ssResult.str();

    transform(hexResult.begin(), hexResult.end(), hexResult.begin(), ::toupper);


    return hexResult;
}


string applyOperation(const std::string& status) {
    // Convert hexadecimal string to integer
    unsigned int value = stoul(status, nullptr, 16);
    
    // Perform bitwise AND with complement of 0x1
    unsigned int result = value & (~0x1);
    
    // Convert the result back to hexadecimal string
    std::stringstream stream;
    stream << hex << uppercase << setw(8) << setfill('0') << result;
    string hexResult = stream.str();
    
    return hexResult;
}

string addSpaces(const string& input) {
    string result;
    for (size_t i = 0; i < input.length(); i += 2) {
        result += input.substr(i, 2) + " ";
    }
    return result;
}

string removeSpaces(const string& input) {
    string result = input;
    result.erase(remove_if(result.begin(), result.end(), [](char c) {
        return isspace(c);
    }), result.end());
    return result;
}

bool compareAddresses(const MemoryAddress& a, const MemoryAddress& b) {
    // Convert address strings to integers for comparison
    long intA = stol(a.address, nullptr, 16);
    long intB = stol(b.address, nullptr, 16);

    // Sort in ascending order
    return intA < intB;
}
//Vraca hex u little endian formatu npr "256" -> "00010000"
string decimaltoLittleEndianHex(const string& input) {
    // Convert input to an integer
    int value;
    
    istringstream(input) >> value;
    

    // Convert integer to little-endian hex string
    ostringstream output;
    for (int i = 0; i < sizeof(int); i++) {
        output << setw(2) << setfill('0') << hex << uppercase << (value & 0xFF);
        value >>= 8;
    }

    return output.str();
}

//Mora da primi obican hex broj, ne little endian
int hexToDecimal(const string& hex) {
    int decimal = 0;

    for (size_t i = 0; i < hex.length(); i++) {
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

//Vraca oznacenu vrednost
int32_t hexToDecimalSigned(const string& hex) {
    int32_t decimal = 0;
    bool isNegative = false;

    for (size_t i = 0; i < hex.length(); i++) {
        char c = hex[i];
        int32_t digitValue;

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

    // Check if the most significant bit is 1
    if (hex.length() > 0 && (hex[0] >= '8' || hex[0] >= '8' && isdigit(hex[0]))) {
        isNegative = true;
    }

    if (isNegative) {
        // Calculate two's complement for negative values
        decimal = (~decimal) + 1;
        decimal = -decimal;
    }

    return decimal;
}


string littleEndianToRegularHex(const string& littleEndianHex) {
    string regularHex = littleEndianHex;

    // Reverse the string
    reverse(regularHex.begin(), regularHex.end());

    // Swap adjacent characters to restore the original order
    for (size_t i = 0; i < regularHex.length(); i += 2) {
        swap(regularHex[i], regularHex[i + 1]);
    }

    return regularHex;
}

int getStatusBitValue(int position) {
    // Convert hexValue to uppercase
    char hexValue = toupper(status[1]);

    int decimal;

    // Convert the hex value to decimal
    if (hexValue >= '0' && hexValue <= '9') {
        decimal = hexValue - '0';
    } else if (hexValue >= 'A' && hexValue <= 'F') {
        decimal = hexValue - 'A' + 10;
    } else {
        std::cout << "Invalid hex value: " << hexValue << std::endl;
        return -1;
    }

    // Get the hexadecimal mask based on the position
    int mask = 1 << position;

    // Extract the bit value at the given position
    int bitValue = (decimal & mask) ? 1 : 0;

    return bitValue;
}

//Podrazumevano ga umanjuje za 4, ovo je pomocna f-ja za push
void decrementSP(){
    int decimalSP = hexToDecimal(littleEndianToRegularHex(sp));
    decimalSP -= 4;
    sp =  decimaltoLittleEndianHex(to_string(decimalSP)); 
}

void incremetSP(){
    int decimalSP = hexToDecimal(littleEndianToRegularHex(sp));
    decimalSP += 4;
    sp =  decimaltoLittleEndianHex(to_string(decimalSP)); 
}


void printMemory() {
    cout << "Memory" << endl;
    cout << string(70, '-') << endl;
    for (const auto& memoryAddress : memory) {
        std::cout << memoryAddress.address << " : " << memoryAddress.value << std::endl;
    }
    cout << string(70, '-') << endl;
}


void setCauseValue(char val){
    cause[1] = val;
}


string incrementHexBy8(const string& hexNumber) {
    stringstream ss;
    unsigned long long decimalValue;
    ss << hex << hexNumber;
    ss >> decimalValue;

    decimalValue += 8;
    stringstream result;
    result << hex << decimalValue;

    string resultHex = result.str();
    if (resultHex.length() < hexNumber.length()) {
        resultHex = string(hexNumber.length() - resultHex.length(), '0') + resultHex;
    }

    return toUpperCase(resultHex);
}


//Svi upisi u memoriju se rade sa 4 ?
void writeToMemory(string address, string value){
    value = addSpaces(value);
    address = toUpperCase(address);

    //cout << " adresa za upis " << address << endl;
    //printMemory();
    if(address == "FFFFFF00"){
        //cout << address << " : " << value << " " << r1 <<endl;
        terminalOutput = true;
        setCauseValue('3');
    }
    if(address.back() == '0' || address.back() == '8'){ //Ako je deljiva sa 8
    //cout << "Upisujem u: " << address << " sa offsetom: " << 0 << " trenutni code " << value <<endl;
        for(auto& adr : memory){
            if(adr.address == address){
                adr.value.replace(0, 12, value);
                return; // adresa je već alocirana upisali smo u nju
            }
        }
        //Ako smo ovde izasli znaci da nije alocirana jos uvek, treba da alociramo i upišemo u nju
        MemoryAddress adr;
        adr.address = address;
        adr.value = value;
        memory.push_back(adr);
        memory.sort(compareAddresses);
        return;
    }else{ // Ako nije deljiva sa 8
        //cout << "Adresa " << address << " " ;
        int adrOff = hexToDecimal(address.substr(address.size()-1, address.size()));
        int pom = 8-adrOff;
        address.replace(address.size()-1, address.size(), (pom < 0 ? "8" : "0")); // Sada je adresa deljvia sa 8 pokušavam da je pronađem
        if(pom > 0){
            pom = adrOff;
        }else{
            pom = abs(pom);
        }
        int cnt = 0;
        //cout << "Upisujem u: " << address << " sa offsetom: " << adrOff << " trenutni code " << value <<endl;
        //cout << " pc :" << pc << endl;
        for(auto& adr : memory){
            if(adr.address == address){
                while(cnt < 4){
                    adr.value.replace(pom*3, 3, string(1,value[cnt*3]) + string(1, value[cnt*3+1]) + " ");
                    cnt++;
                    pom++;
                    if(pom == 8){ // Popunili smo ovu adresu idemo na sledecu
                        pom = 0;
                        address = incrementHexBy8(address);
                        for(auto& newadr : memory){
                            if(newadr.address == address){
                                while(cnt < 4){
                                    newadr.value.replace(pom*3, 3, string(1,value[cnt*3]) + string(1, value[cnt*3+1]) + " ");
                                    cnt++; //ovde ne moram i pom da proveravam jer sigurno nece trebati jos jedna nova adresa
                                    pom++;
                                }
                                return;
                            }
                        }
                        //Ako smo ovde izasli znaci da nije alocirana jos uvek, treba da alociramo i upišemo u nju
                        MemoryAddress newadr;
                        newadr.address = address;
                        pom = 0;
                        while(cnt < 4){
                            newadr.value +=  string(1,value[cnt*3]) + string(1, value[cnt*3+1]) + " ";
                            cnt++;
                            pom++; 
                        }
                        pom = 8 - pom;
                        for(int i=0; i< pom; i++){
                            newadr.value += "xx ";
                        }
                        memory.push_back(newadr);
                        memory.sort(compareAddresses);
                    } 
                }
                
                return;
            }
        }
        //Ako smo ovde izasli znaci da nije alocirana jos uvek, treba da alociramo i upišemo u nju
        MemoryAddress adr;
        adr.address = address;
        for(int i=0; i < pom; i++){
            adr.value += "xx ";
        }
        cnt = 0;
        while(cnt < 4){
            adr.value +=  string(1,value[cnt*3]) + string(1, value[cnt*3+1]) + " ";
            cnt++;
            pom++;
            if(pom == 8){ // Popunili smo ovu adresu idemo na sledecu
                pom = 0;
                memory.push_back(adr);
                memory.sort(compareAddresses);
                address = incrementHexBy8(address);
                for(auto& newadr : memory){
                    if(newadr.address == address){
                        while(cnt < 4){
                            newadr.value.replace(pom*3, 3, string(1,value[cnt*3]) + string(1, value[cnt*3+1]) + " ");
                            cnt++; //ovde ne moram i pom da proveravam jer sigurno nece trebati jos jedna nova adresa
                            pom++;
                        }
                        return;
                    }
                }
                //Ako smo ovde izasli znaci da nije alocirana jos uvek, treba da alociramo i upišemo u nju
                MemoryAddress newadr;
                newadr.address = address;
                pom = 0;
                while(cnt < 4){
                    newadr.value +=  string(1,value[cnt*3]) + string(1, value[cnt*3+1]) + " ";
                    cnt++;
                    pom++; 
                }
                pom = 8 - pom;
                for(int i=0; i< pom; i++){
                    newadr.value += "xx ";
                }
                memory.push_back(newadr);
                memory.sort(compareAddresses);
                return;
            } 
        }
        pom = 8 - pom;
        for(int i=0; i< pom; i++){
            adr.value += "xx ";
        }
        memory.push_back(adr);
        memory.sort(compareAddresses);
        return;
    }
}


void checkCause(){
    if(cause[1] == '2' && (checkStatusMask() & 1) != 0){ // maskiran prekid od tajmera
        return;
    }else if(cause[1] == '3' && (checkStatusMask() & 2) != 0){ // maskiran prekid od terminala
        return;
    }else if(cause[1] == '4' && (checkStatusMask() & 4) != 0){ // maskiran softversi prekid
        return;
    }else if(cause[1] == '4'){
        setStatusMask(7);
    } else  if(cause[1] != '0'){
        //cout << " usao u check cause " << endl;
        //push status
        decrementSP(); // sp <= sp - 4
        writeToMemory(littleEndianToRegularHex(sp), status); //  mem32[sp] <= status 
        setStatusMask(7); //maskiraj sve prekide
        //cout << "status posle set mask : "<< status << endl;
        // push pc
        decrementSP(); // sp <= sp - 4
        writeToMemory(littleEndianToRegularHex(sp), pc); //  mem32[sp] <= pc 
        pc = toUpperCase(handler);
        //cout << pc << "prekid cause:" << cause[1] << " status: "<< status <<endl;
        //printMemory();
    }
    
}

void loadCodeToMemory(string inputFileName) {
    ifstream inputFile(inputFileName);
    if (!inputFile) {
        cout << "ERROR: Fajl neuspešno otvoren!" << endl;
        exit(-1);
    }

    string line;
    while (getline(inputFile, line)) {
        istringstream iss(line);
        string address;
        iss >> address;
        int decimalValue = hexToDecimal(address);
        if (decimalValue < 0) {
            cout << "Adresa " << address << " prelazi izvan opsega memorije 0xFFFFFFFF." << endl;
            exit(-1);
        }
        string value;
        iss >> value; // ovde se sada nalazi ":"
        iss >> value; // sada se nalazi prava vrednost
        for (int i = 0; i < 7; i++) {
            string pom;
            iss >> pom;
            value += " " + pom;
        }
        MemoryAddress memoryAddress;
        memoryAddress.address = address;
        memoryAddress.value = value;
        memory.push_back(memoryAddress);
    }
    inputFile.close();
}

string getRegisterValue(int n){
    switch (n)
    {
    case 0:
        return r0;
      break;    
    case 1:
        return r1;
      break;
    case 2:
        return r2;
      break;    
    case 3:
        return r3;
      break;
    case 4:
        return r4;
      break;    
    case 5:
        return r5;
      break;
    case 6:
        return r6;
      break;    
    case 7:
        return r7;
      break;
    case 8:
        return r8;
      break;    
    case 9:
        return r9;
      break;
    case 10:
        return r10;
      break;    
    case 11:
        return r11;
      break;
    case 12:
        return r12;
        break;
    case 13:
        return r13;
        break;
    case 14: 
        return sp;
        break;
    case 15:
        return pc;
        break;     
    default:
        break;
    }
    return "ERROR: došao do kraja switch u getRegisterValue";
}

string getRegisterValue(char n){
    if(n == '0'){
        return r0;
    }else if(n == '1'){
        return r1;
    }else if(n == '2'){
        return r2;
    }else if(n == '3'){
        return r3;
    }else if(n == '4'){
        return r4;
    }else if(n == '5'){
        return r5;
    }else if(n == '6'){
        return r6;
    }else if(n == '7'){
        return r7;
    }else if(n == '8'){
        return r8;
    }else if(n == '9'){
        return r9;
    }else if(n == 'a' || n == 'A'){
        return r10;
    }else if(n == 'b' || n == 'B'){
        return r11;
    }else if(n == 'c' || n == 'C'){
        return r12;
    }else if(n == 'd' || n == 'D'){
        return r13;
    }else if(n == 'e' || n == 'E'){
        return sp;
    }else if(n == 'f' || n == 'F'){
        return pc;
    }
    return "ERROR: došao do kraja switch u getRegisterValue";
}

string getCSRRegisterValue(char n){
    if(n == '0'){
        return status;
    }else if(n == '1'){
        return handler;
    }else if(n == '2'){
        return cause;
    }
    setCauseValue('1');
    return "";
}

void writeToRegister(char n, string value){
    if(n == '0'){
        setCauseValue('1');
    }else if(n == '1'){
        r1 = value;
    }else if(n == '2'){
        r2 = value;
    }else if(n == '3'){
        r3 = value;
    }else if(n == '4'){
        r4 = value;
    }else if(n == '5'){
        r5 = value;
    }else if(n == '6'){
        r6 = value;
    }else if(n == '7'){
        r7 = value;
    }else if(n == '8'){
        r8 = value;
    }else if(n == '9'){
        r9 = value;
    }else if(n == 'a' || n == 'A'){
        r10 = value;
    }else if(n == 'b' || n == 'B'){
        r11 = value;
    }else if(n == 'c' || n == 'C'){
        r12 = value;
    }else if(n == 'd' || n == 'D'){
        r13 = value;
    }else if(n == 'e' || n == 'E'){
        sp = value;
    }else if(n == 'f' || n == 'F'){
        pc = value;
    }else{
        setCauseValue('1');
    }
    return;
}

void writeToCSRRegister(char n, string value){
    if(n == '0'){
        status = value;
    }else if(n == '1'){
        handler = value;
    }else if(n == '2'){
        cause = value;
    }else{
        setCauseValue('1');
    }
    return;
}

void printRegisterValues() {
    cout << endl;
    cout << "Emulated processor executed halt instruction" <<endl;
    cout << "Emulated processor state:" << endl;;
    for (int i = 0; i <= 15; ++i) {
        string regName = "r" + to_string(i);
        cout << regName << "=" << hex << "0x" << setw(8) << setfill('0') << getRegisterValue(i) << " ";
        if ((i + 1) % 4 == 0)
            cout << endl;
    }
}

//address je adresa odakle dohvatam mora biti u little endian formatu, offset koliko elemenata
string getValueOfAddressMemory(string address, int offset){
    //cout << address;
    if(address[1] == '0' || address[1] == '4' || address[1] == '8' || address[1] == 'C'){ // imam pristup "ravnim" adresama
        int adrOff = hexToDecimal(string(1, address[1]));
        address[1] = (adrOff >= 8 ? '8' : '0');
        if(adrOff == 8){
            adrOff = 0;
        }else if(adrOff == 12){
            adrOff = 4;
        }
        int cnt = 0;
        string val = "";
        for(auto& adr : memory){
            if(adr.address == littleEndianToRegularHex(address)){
                
                while(cnt < 4){
                    val += string(1,adr.value[adrOff*3]) + string(1, adr.value[adrOff*3+1]) + " ";
                    cnt++;
                    adrOff++;
                }
                return val;
            }
        }
    }else{
        int adrOff = hexToDecimal(string(1, address[1]));
        //cout << adrOff ;
        if((adrOff > 4 && adrOff < 8) || (adrOff > 12)){ // Ide citanje iz dve adrese
            address[1] = (adrOff >= 8 ? '8' : '0');
            int cnt = 0;
            string val = "";
            if(adrOff >= 8){
                adrOff = adrOff - 8;
            }
            for(auto& adr : memory){
                if(adr.address == littleEndianToRegularHex(address)){
                    while(cnt < 4){
                        val += string(1,adr.value[adrOff*3]) + string(1, adr.value[adrOff*3+1]) + " ";
                        cnt++;
                        adrOff++;
                        if(adrOff == 8){
                            adrOff = 0;
                            address = incrementHexBy8(littleEndianToRegularHex(address));
                            for(auto& secAdr : memory){
                                if(secAdr.address == address){
                                    while(cnt < 4){
                                        val += string(1,secAdr.value[adrOff*3]) + string(1, secAdr.value[adrOff*3+1]) + " ";
                                        adrOff++;
                                        cnt++;
                                    }
                                }
                            }
                        }
                    }
                    return val;
                }
            }
        }else{ // Ovde je citanje iz jedne adrese
            address[1] = (adrOff >= 8 ? '8' : '0');
            //cout << " i adresa " << address << " ";
            if(adrOff >= 8){
                adrOff = adrOff - 8;
            }
            //cout << adrOff << " adr off u getValue " << endl;
            for(auto& adr : memory){
                if(adr.address == littleEndianToRegularHex(address)){
                    return adr.value.substr(adrOff*3, 3*offset-1);
                }
            }  
        }
    }
    cout << "Pokušan pristup memoriji koja nije alocirana " << address << endl;
    cout << " code " << value << endl;
    printMemory();
    printRegisterValues();
    cout << cause << endl;
    cout << status << endl;
    exit(-1);
}



//value[0] -> OC, value[1] -> MOD, value[3] -> regA, value[4]-> regB, value[6]->regC, value[7,9,10]-> displ
void executeInstruction(){
    //cout << "value0: " << value[0] << " value1: " << value[1] <<endl;
    switch (value[0])
    {
    case '0':   // Pročitana je HALT instrukcija
        if(value == "00 00 00 00 " || value == "00 00 00 00"){ //Instrukcija je ispravna
            isHALT = true;
        }else{ // Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '1':   //Pročitana instrukcija softverskog prekida
        if(value == "10 00 00 00" || value == "10 00 00 00 "){ //Instrukcija je ispravna
            // push status
            decrementSP(); // sp <= sp - 4
            writeToMemory(littleEndianToRegularHex(sp), status); //  mem32[sp] <= status 
            // push pc
            decrementSP(); // sp <= sp - 4
            writeToMemory(littleEndianToRegularHex(sp), pc); //  mem32[sp] <= pc 
            setCauseValue('4'); // cause <= 4
            setStatusFirstBitToZero(); // status<=status&(~0x1)
            //cout << " int inst " << endl; 
            pc = handler;
        }else{ // Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '2':   //Instrukcija poziva potprograma
        if(value[1] == '0' && value[6] == '0'){
            // push pc
            decrementSP(); // sp <= sp - 4
            writeToMemory(littleEndianToRegularHex(sp), pc); //  mem32[sp] <= pc
            long gprA = 0;
            long gprB = 0;
            long dis = 0;
            if(value[3] != '0'){ // Korišćen je gprA
                gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            }
            if(value[4] != '0'){ // Korišćen je gprB
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            long result = gprA + gprB + dis;
            pc = decimaltoLittleEndianHex(to_string(result)); // pc<=gpr[A]+gpr[B]+D;

        }else if(value[1] == '1' && value[6] == '0'){
            // push pc
            decrementSP(); // sp <= sp - 4
            writeToMemory(littleEndianToRegularHex(sp), pc); //  mem32[sp] <= pc
            long gprA = 0;
            long gprB = 0;
            long dis = 0;
            if(value[3] != '0'){ // Korišćen je gprA
                gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            }
            if(value[4] != '0'){ // Korišćen je gprB
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            long result = gprA + gprB + dis;
            pc = getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(result)), 4); // pc<=mem32[gpr[A]+gpr[B]+D];
            pc = removeSpaces(pc);
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '3':   //Instrukcija skoka 
        if(value[1] == '0' && value[4] == '0' && value[6] == '0'){  // pc<=gpr[A]+D;
            long gprA = 0;
            long dis = 0;
            long result = 0;
            if(value[3] != '0'){ // Korišćen je gprA
                gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            }
            dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            result = gprA  + dis;
            pc = decimaltoLittleEndianHex(to_string(result));
        }else if(value[1] == '1'){                                  //if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
            long gprB = 0;
            long gprC = 0;
            if(value[4] != '0'){ // Korišćen je gprA
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            if(value[6] != '0'){ // Korišćen je gprA
                gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
            }
            //cout << " vrednost gprB " << gprB << " vrednost gprC " << gprC <<" vrednost cause"<< cause <<endl;
            if(gprC == gprB){
                long gprA = 0;
                long dis = 0;
                long result = 0;
                if(value[3] != '0'){ // Korišćen je gprA
                    gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
                }
                dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
                result = gprA  + dis;
                pc = decimaltoLittleEndianHex(to_string(result));

            }

        }else if(value[1] == '2'){                                  //if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
            long gprB = 0;
            long gprC = 0;
            if(value[4] != '0'){ // Korišćen je gprA
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            if(value[6] != '0'){ // Korišćen je gprA
                gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
            }
            if(value == "32 01 20 d8 " || value == "32 01 20 d8")
                cout << "vrednost gprb: " << gprB<< " vrednost gprc: " << gprC << endl;
            if(gprC != gprB){
                long gprA = 0;
                long dis = 0;
                long result = 0;
                if(value[3] != '0'){ // Korišćen je gprA
                    gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
                }
                dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
                result = gprA  + dis;
                pc = decimaltoLittleEndianHex(to_string(result));

            }

        }else if(value[1] == '3'){                                  //if (gpr[B] signed > gpr[C]) pc<=gpr[A]+D;
            long gprB = 0;
            long gprC = 0;
            if(value[4] != '0'){ // Korišćen je gprB
                gprB = hexToDecimalSigned(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            if(value[6] != '0'){ // Korišćen je gprC
                gprC = hexToDecimalSigned(littleEndianToRegularHex(getRegisterValue(value[6])));
            }
            if(gprB > gprC){
                long gprA = 0;
                long dis = 0;
                long result = 0;
                if(value[3] != '0'){ // Korišćen je gprA
                    gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
                }
                dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
                result = gprA  + dis;
                pc = decimaltoLittleEndianHex(to_string(result));

            }

        }else if(value[1] == '8' && value[4] == '0' && value[6] == '0'){ //pc<=mem32[gpr[A]+D];
            long gprA = 0;
            long dis = 0;
            long result = 0;
            if(value[3] != '0'){ // Korišćen je gprA
                gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            }
            dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            result = gprA  + dis;
            pc = getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(result)), 4); // pc<=mem32[gpr[A]+gpr[B]+D];
            pc = removeSpaces(pc);   
            pc = regularToLittleEndianHex(pc);
        }else if(value[1] == '9'){                                  //if (gpr[B] == gpr[C]) pc<=mem32[gpr[A]+D];
            long gprB = 0;
            long gprC = 0;
            if(value[4] != '0'){ // Korišćen je gprA
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            if(value[6] != '0'){ // Korišćen je gprA
                gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
            }
            if(gprC == gprB){
                long gprA = 0;
                long dis = 0;
                long result = 0;
                if(value[3] != '0'){ // Korišćen je gprA
                    gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
                }
                dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
                result = gprA  + dis;
                pc = getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(result)), 4); // pc<=mem32[gpr[A]+gpr[B]+D];
                pc = removeSpaces(pc);

            }

        }else if(value[1] == 'a' || value[1] == 'A'){                                  // if (gpr[B] != gpr[C]) pc<=mem32[gpr[A]+D];
            long gprB = 0;
            long gprC = 0;
            if(value[4] != '0'){ // Korišćen je gprA
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            if(value[6] != '0'){ // Korišćen je gprA
                gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
            }
            if(gprC != gprB){
                long gprA = 0;
                long dis = 0;
                long result = 0;
                if(value[3] != '0'){ // Korišćen je gprA
                    gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
                }
                dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
                result = gprA  + dis;
                pc = getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(result)), 4); // pc<=mem32[gpr[A]+gpr[B]+D];
                pc = removeSpaces(pc);

            }

        }else if(value[1] == 'b' || value[1] == 'B'){                                  //if (gpr[B] signed> gpr[C]) pc<=mem32[gpr[A]+D];
            long gprB = 0;
            long gprC = 0;
            if(value[4] != '0'){ // Korišćen je gprB
                gprB = hexToDecimalSigned(littleEndianToRegularHex(getRegisterValue(value[4])));
            }
            if(value[6] != '0'){ // Korišćen je gprC
                gprC = hexToDecimalSigned(littleEndianToRegularHex(getRegisterValue(value[6])));
            }
            if(gprB > gprC){
                long gprA = 0;
                long dis = 0;
                long result = 0;
                if(value[3] != '0'){ // Korišćen je gprA
                    gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
                }
                dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
                result = gprA  + dis;
                pc = getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(result)), 4); // pc<=mem32[gpr[A]+gpr[B]+D];
                pc = removeSpaces(pc);

            }

        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '4':   //Instrukcija atomične zamene vrednosti
        if(value[1] == '0' && value[3] == '0' && value[7] == '0' && value[9] == '0' && value[10] == '0'){
            string temp;
            string gprB = getRegisterValue(value[4]);
            string gprC = getRegisterValue(value[6]);
            temp = gprB;
            writeToRegister(value[4], gprC); //  gpr[B]<=gpr[C]
            writeToRegister(value[6], temp); //gpr[C]<=temp;
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '5':   //Instrukcija aritmetičkih operacija
        if(value[3] != '0' && value[7] == '0' && value[9] == '0' && value[10] == '0'){
            if(value[1] == '0'){ // gpr[A]<=gpr[B] + gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                //cout << "sabirak 1 : " << gprB << "sabirak 2 : " << gprC << endl;
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB + gprC)));
                //cout << "Rezultat sabiranja : " << getRegisterValue(value[3]) << endl;
                //printMemory();
            }else if(value[1] == '1'){ // gpr[A]<=gpr[B] - gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                //cout << "sabirak 1 : " << gprB << "sabirak 2 : " << gprC << endl;
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB - gprC)));
                //cout << "Rezultat oduzimanja : " << getRegisterValue(value[3]) << endl;
                
            }else if(value[1] == '2'){ // gpr[A]<=gpr[B] * gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                //cout << "sabirak 1 : " << gprB << "sabirak 2 : " << gprC << endl;
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB * gprC)));
                //cout << "Rezultat mnozenja : " << getRegisterValue(value[3]) << endl;
                
            }else if(value[1] == '3'){ // gpr[A]<=gpr[B] / gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                //cout << "sabirak 1 : " << gprB << "sabirak 2 : " << gprC << endl;
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB / gprC)));
                //cout << "Rezultat deljenja : " << getRegisterValue(value[3]) << endl;
                
            }
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '6':   //Instrukcija logičkih operacija
        if(value[3] != '0' && value[7] == '0' && value[9] == '0' && value[10] == '0'){
            if(value[1] == '0' && value[6] == '0'){ // gpr[A]<=~gpr[B];
                string gprB = getRegisterValue(value[4]);
                writeToRegister(value[3], performHexNot(gprB));
            }else if(value[1] == '1'){ // gpr[A]<=gpr[B] & gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB & gprC)));
            }else if(value[1] == '2'){ // gpr[A]<=gpr[B] | gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB | gprC)));
            }else if(value[1] == '3'){ // gpr[A]<=gpr[B] ^ gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB ^ gprC)));
            }
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '7':   //Instrukcija pomeračkih operacija
        if(value[3] != '0' && value[7] == '0' && value[9] == '0' && value[10] == '0'){
            if(value[1] == '0'){ // gpr[A]<=gpr[B] << gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB << gprC)));
            }else if(value[1] == '1'){ // gpr[A]<=gpr[B] & gpr[C];
                long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
                writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprC >> gprB)));
            }
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '8':   //Instrukcija smeštanja podatka
        if(value[1] == '0'){ // mem32[gpr[A]+gpr[B]+D]<=gpr[C];
            if(value[3] == 'e' && value[4] == '0'){ //Ovo je push onda
                //cout << " push " << value <<endl;
                decrementSP();
            }
            long gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            long result = gprA + gprB + dis;
            writeToMemory(littleEndianToRegularHex(decimaltoLittleEndianHex(to_string(result))), getRegisterValue(value[6]));
        }else if(value[1] == '2'){ // mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
            //cout << "regA : " <<value[3]<< " regB : " << value[4] << " displ: " << string(1,value[7])+string(1,value[9])+string(1,value[10]) << endl;
            long gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            long result = gprA + gprB + dis;
            string address =  getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(result)),4);
            
            address = removeSpaces(address);
            address = regularToLittleEndianHex(address);
            //cout << " pc : " << pc << endl;
            //cout << " adresa za st : " << address << endl;
            //cout << " mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C] " << " adresa: "<< address << endl;
            //cout << " upisuje : " << getRegisterValue(value[6]) << endl;;
            writeToMemory(address, getRegisterValue(value[6]));
            
        }else if(value[1] == '1' && value[4] == '0'){ // gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
            long gprA = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[3])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            long result = gprA + dis;
            writeToMemory(littleEndianToRegularHex(decimaltoLittleEndianHex(to_string(result))), getRegisterValue(value[6]));
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
        break;
    case '9':   //Instrukcija učitavanja podatka
        if(value[1] == '0' && value[6] == '0' && value[7] == '0' && value[9] == '0' && value[10] == '0'){ //gpr[A]<=csr[B];
            writeToRegister(value[3], getCSRRegisterValue(value[4]));
        }else if(value[1] == '1' && value[6] == '0'){ //gpr[A]<=gpr[B]+D;
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            writeToRegister(value[3], decimaltoLittleEndianHex(to_string(gprB + dis)));
          
        }else if(value[1] == '2' ){ // gpr[A]<=mem32[gpr[B]+gpr[C]+D];
            long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            //if(value == "92 11 00 00 " || value == "92 11 00 00"){
            //    cout << "adresa iz koje citamo " << decimaltoLittleEndianHex(to_string(gprC+gprB+dis)) << endl;
            //}
            writeToRegister(value[3], removeSpaces(getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(gprC+gprB+dis)), 4)));
            //cout << getRegisterValue(value[3]) << " vrednost registra posle 92 " << endl;
        }else if(value[1] == '3' && value[6] == '0' && value[3] == '0' && value[4] == 'e'){ //gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            writeToRegister(value[4], decimaltoLittleEndianHex(to_string(gprB + dis)));
            writeToCSRRegister(value[3], removeSpaces(getValueOfAddressMemory(getRegisterValue(value[4]), 4)));
            if(value[4] == 'e'){ // Ovo je pop
                //cout << " pop " << value <<endl;
                incremetSP();
            }
        }else if(value[1] == '3' && value[6] == '0'){ //gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            writeToRegister(value[4], decimaltoLittleEndianHex(to_string(gprB + dis)));
            //cout << removeSpaces(getValueOfAddressMemory(getRegisterValue(value[4]), 4)) << " vrednost adrese  u st " << endl;
            int decimalPC = hexToDecimal(littleEndianToRegularHex(pc));
            writeToRegister(value[3], removeSpaces(getValueOfAddressMemory(getRegisterValue(value[4]), 4)));
            //decimalPC += 4;
            string pomPC = decimaltoLittleEndianHex(to_string(decimalPC));
            string pomVALUE = getValueOfAddressMemory(pomPC, 4);
           // cout << "Pomvalue " << pomVALUE << " " << pc << endl;
            if(value[4] == 'e' && value[7] == '0' && value[9] == '0' && value[10] == '0'){ // Ovo je pop
             //   cout << " pop " << value <<endl;
                incremetSP();
            }if(value[3] == 'f' && (pomVALUE == "93 0e 00 00 " || pomVALUE == "93 0e 00 00" || pomVALUE == " 93 0e 00 00" || pomVALUE == " 93 0e 00 00 ")){ //Ovo je iret !
                //cout << "iret " << endl;
                gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
                writeToRegister(value[4], decimaltoLittleEndianHex(to_string(gprB + dis)));
                writeToCSRRegister( '0' , removeSpaces(getValueOfAddressMemory(getRegisterValue(value[4]), 4)));
                incremetSP();
                setCauseValue('0');
            }
        }else if(value[1] == '4' && value[6] == '0' && value[7] == '0' && value[9] == '0' && value[10] == '0'){ // csr[A]<=gpr[B];
            writeToCSRRegister(value[3], getRegisterValue(value[4]));
        }else if(value[1] == '5' && value[6] == '0'){ // csr[A]<=csr[B] | D;
            long csrB = hexToDecimal(littleEndianToRegularHex(getCSRRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            writeToCSRRegister(value[3], removeSpaces(decimaltoLittleEndianHex(to_string(csrB | dis))));
        }else if(value[1] == '6'){ //  csr[A]<=mem32[gpr[B]+gpr[C]+D];
            long gprC = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[6])));
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            writeToCSRRegister(value[3], removeSpaces(getValueOfAddressMemory(decimaltoLittleEndianHex(to_string(gprC+gprB+dis)), 4)));
        }else if(value[1] == '7' && value[6] == '0'){ // csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
            long gprB = hexToDecimal(littleEndianToRegularHex(getRegisterValue(value[4])));
            long dis = hexToDecimal(string(1,value[7])+string(1,value[9])+string(1,value[10]));
            writeToRegister(value[4], decimaltoLittleEndianHex(to_string(gprB + dis)));
            writeToCSRRegister(value[3], removeSpaces(getValueOfAddressMemory(getRegisterValue(value[4]), 4)));
        }else{//Instrukcija nije ispravna u PC upiši memorijsku adresu prekidne rutine i azuriraj cause registar
            setCauseValue('1');
        }
    default:
        break;
    }
}

//Po defaultu cu staviti da je vrednost u tajmeru 00 00 00 00 sto znaci da generise prekid na 500ms
void loadTimerIntoMemory(){
    writeToMemory("FFFFFF10", "00000000");
}

void setTerminalMemoryLocation(){
    writeToMemory("FFFFFF00", "00000000");
    writeToMemory("FFFFFF04", "00000000");
}

int convertTimerPeriod(int period){
    switch (period)
    {
    case 0:
        return 500;
        break;
    case 1:
        return 1000;
        break;
    case 2:
        return 1500;
        break;
    case 3:
        return 2000;
        break;    
    case 4:
        return 5000;
        break;
    case 5:
        return 10000;
        break;
    case 6:
        return 30000;
        break;
    case 7:
        return 60000;
        break;    
    default:
        return 500;
        break;
    }
}


void oneTimePeriod(){
    int period;
    string valuePeriod = getValueOfAddressMemory("10FFFFFF", 4);
    //cout << string(1,valuePeriod[1]) << endl;
    period = stoi(string(1,valuePeriod[1]));
    period = convertTimerPeriod(period);
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    if( elapsedTime >= period){
        //cout << " timer trigerovan" << endl;
        setCauseValue('2');
        startTime = std::chrono::steady_clock::now();
        //printMemory();
    }
}

void setTerminalForNonBlocking(){
    // Set the terminal to non-canonical mode
    termios originalTermios, modifiedTermios;
    tcgetattr(STDIN_FILENO, &originalTermios);
    modifiedTermios = originalTermios;
    modifiedTermios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &modifiedTermios);
}

void isOutputAvailabe(){
    if (terminalOutput) {
        string out;
        for (auto& adr : memory) {
            if (adr.address == "FFFFFF00") {
                out = string(1, adr.value[0]) + string(1, adr.value[1]);
                //cout << out << " isoa " << endl;
            }
        }
        int number = hexToDecimal(out);
        //cout << number << endl;
        cout << char(number);
        cout.flush();
        setCauseValue('0');
        terminalOutput = false;
        //printMemory();
    }
    
}

void isInputAvailable2() {
    struct pollfd fd[1];
    fd[0].fd = STDIN_FILENO;
    fd[0].events = POLLIN;

    // Check if any keys are pressed
    int poll_result = poll(fd, 1, 0);
    if (poll_result > 0) {
        // Read and discard the pressed character
        char ch;
        cin.get(ch);
        int asciiCode = static_cast<int>(ch);
        string ascii = decimaltoLittleEndianHex(to_string(asciiCode));
        writeToMemory("FFFFFF04", ascii);
        setCauseValue('3');
    }
}



int ascii_code;
bool keyPressed;

void isInputAvailable() {
   
    struct termios old_termios, new_termios;

    // Sačuvaj trenutna podešavanja terminala
    tcgetattr(STDIN_FILENO, &old_termios);

    // Kopiraj trenutna podešavanja u novu strukturu
    new_termios = old_termios;

    // Isključi echo (prikazivanje unetih karaktera na ekranu)
    new_termios.c_lflag &= ~ECHO;

    // Postavi no buffering (ne čekaj na terminator unosa)
    new_termios.c_lflag &= ~ICANON;

    // Postavi novo podešavanje terminala
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    // Čitaj tastera sve dok se ne pritisne prekidni taster (npr. CTRL+C)
    char input;
    if (std::cin.get(input)) {
        // Upisuje ASCII kod pritisnutog tastera u term_in registar
        ascii_code = static_cast<int>(input);
        // Ovde možete raditi sa vrednošću ASCII koda kako god želite
        keyPressed = true;
        cout << " timer " << endl;
        setCauseValue('3');
        // Generiše zahtev za prekid
        // ...


    }

    // Vrati originalno podešavanje terminala
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void emulateCode(){
    int decimalPC;
    pc = "40000000"; // Ovo je u little endian formatu tako da je to ustvari 00000040
    int cnt = 0;
    while(!isHALT){
        setTerminalForNonBlocking();
        value = getValueOfAddressMemory(pc, 4);
        //cout << " vrednost pc-a " << pc << " instrukcija " << value << " vrednost sp " << sp <<endl;
        decimalPC = hexToDecimal(littleEndianToRegularHex(pc));
        decimalPC += 4;
        pc = decimaltoLittleEndianHex(to_string(decimalPC));
        executeInstruction();
        isInputAvailable2();
        isOutputAvailabe();
        oneTimePeriod();

        //printMemory();
        checkCause();
        if(cause[1] == '1'){
            cout << "ERROR: pročitana je pogrešna instrukcija: " << value <<endl;
            exit(-1);
        }
    }


/*
    while(status != "07000000"){
        cout << pc << " vrednost pc-a "<< endl;
        value = getValueOfAddressMemory(pc, 4);
        decimalPC = hexToDecimal(littleEndianToRegularHex(pc));
        decimalPC += 4;
        pc = decimaltoLittleEndianHex(to_string(decimalPC));
        executeInstruction();
        oneTimePeriod();
        isInputAvailable2();
        
        checkCause();

        if(keyPressed){
            cout << ascii_code << " taster " << endl;
        }

        if(cause[1] == '1'){
            cout << "ERROR: pročitana je pogrešna instrukcija: " << value <<endl;
            exit(-1);
        }
    }
    for(int i= 0; i< 50; i++){
       
        cout << pc << " vrednost pc-a "<< endl;
        value = getValueOfAddressMemory(pc, 4);
        decimalPC = hexToDecimal(littleEndianToRegularHex(pc));
        decimalPC += 4;
        pc = decimaltoLittleEndianHex(to_string(decimalPC));
        executeInstruction();
        oneTimePeriod();
        isInputAvailable2();
        
        checkCause();

        if(keyPressed){
            cout << ascii_code << " taster " << endl;
        }

        if(cause[1] == '1'){
            cout << "ERROR: pročitana je pogrešna instrukcija: " << value <<endl;
            exit(-1);
        }
    
    }
*/
    
}


void reserveMemoryForTerminal(){
    MemoryAddress memTerminal;
    memTerminal.address = "FFFFFF00";
    memTerminal.value = "00 00 00 00 00 00 00 00";
    memory.push_back(memTerminal);
}


void discardInput() {
    struct termios oldSettings, newSettings;
    tcgetattr(STDIN_FILENO, &oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms timeout

    // Clear any existing input
    while (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
        char ch;
        read(STDIN_FILENO, &ch, sizeof(ch));
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
}

#endif  // EMULATOR_HPP