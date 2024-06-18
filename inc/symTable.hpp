#ifndef SYMTABLE_HPP
#define SYMTABLE_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <iomanip>
#include <algorithm>

#define SYMBOL 1
#define SECTION 2

using namespace std;

static int number = 0;

struct SymbolTable {
    int num;
    string name;
    int type;
    string section;
    long offset; // Za simbole je zaista offset, za sekcije je njihova velicina, a kasnije u koliko je korišćena -hex opcija bice pozicija sekcije u fajlu
    bool local;
};

struct RelocationTable{
    string name;
    string section;
    long offset;
    int type;
};


list<SymbolTable> symTable;

list<RelocationTable> relocTable;

void addSectionSize(string name, int size){
    for(auto& symbol : symTable){
        if(symbol.name == name){
            symbol.offset = size;
            return;
        }
    }
}

void fixSymbolOffset(string name, int offset){
    for(auto& sym : symTable){
        if(sym.name == name){
            //cout << sym.offset << "trenutni ofset" << endl;
            sym.offset = offset;
            //cout << sym.offset << "novi ofset" << endl;
        }
    }
}


struct PositionOfSection{
    string name;
    long offset;
    int size;
};

list<PositionOfSection> sectionPosition;

bool searchSymbol(string& name){
    for (const auto& symbol : symTable) {
            if (symbol.name == name) {
                return true;  // Found the symbol
            }
        }
    return false;
}

bool fixUndefinedSymbol(string& name, string& section, int offset){
    for (auto& symbol : symTable) {
            if (symbol.name == name && symbol.section == "?") {
                symbol.section = section;
                symbol.offset = offset;
                return true;  // Found the symbol
            }
        }
    return false;
}

bool sameSection(string name, string section){
    for(const auto& symbol : symTable){
        if(symbol.name == name)
            if(symbol.section == section){
                return true;
            }
    }
    return false;
}


SymbolTable getSymbolFromSymbolTable(string name){
    for(auto& symbol : symTable){
        if(symbol.name == name){
            return symbol;
        }
    }
    cout << "Nepostojeći simbol u tabeli simbola " << name << endl;
    exit(-1);
}

long getSymbolOffset(string name){
    for(const auto& symbol : symTable){
        if(symbol.name == name){
            return symbol.offset;
        }
    }
    cout << "Greška prilikom dohvatnja offset pozicije simbola " << name << endl;
    exit(-1);
}

bool isSymbolLocal(string name){
    for(auto& symbol : symTable){
        if(symbol.name == name){
            return symbol.local;
        }
    }
    return false;
}

bool isSymbolExtern(string name){
    for(auto& symbol : symTable){
        if(symbol.name == name){
            if(symbol.section == "?")
                return true;
        }
    }
    return false;
}

void changeSymbolToGlobal(string name){
    for(auto& symbol : symTable){
        if(symbol.name == name){
            symbol.local = false;
            return;
        }
    }
}

void addSymbol(string name, int type, string section, int offset, bool local){
    SymbolTable newSymbol;
    newSymbol.name = name;
    newSymbol.type = type;
    newSymbol.section = section;
    newSymbol.offset = offset;
    newSymbol.local = local;
    newSymbol.num = number++;
    symTable.push_back(newSymbol);
}


void writeSymbolTableToFile(ofstream& outputFile) {
    outputFile << "TabelaSimbola" << endl;
    outputFile << string(70, '-') << endl;
     // Write header line
    outputFile << std::left << std::setw(20) << "Name"
               << std::setw(12) << "Offset"
               << std::setw(25) << "Section"
               << std::setw(10) << "Type"
               << std::setw(10) << "Scope" 
               << setw(10) << "r.br" << endl;

    // Write symbol table data
    for (const auto& symbol : symTable) {
        string typeString = (symbol.type == 1) ? "SYMBOL" : (symbol.type == 2) ? "SECTION" : "UNKNOWN";
        outputFile << left << setw(20) << symbol.name
                   << setw(12) << symbol.offset
                   << setw(25) << symbol.section
                   << setw(10) << typeString
                   << setw(10) << (symbol.local ? "Local" : "Global") 
                   << setw(10) << symbol.num <<endl;
    }
    outputFile << string(70, '-') << endl;
}

long getSectionOffsetPositionOfSection(string name){
    for(auto& sec : sectionPosition){
        if(sec.name == name){
            return sec.offset;
        }
    }
    cout << "Greška prilikom pretrage sekcije u poziciji sekcija! " << name <<endl;
    exit(-1);
}


//ZA LINKER
//----------------
struct DefinedSections{
    string name;
    int offset;
};

list<DefinedSections> defSections;

void addDefinedSection(string name, int offset){
    DefinedSections section;
    section.name = name;
    section.offset = offset;
    defSections.push_back(section);
}

//Vraca true ako nađe sekciju
bool searchDefinedSections(string name){
    for(auto& section : defSections){
        if(section.name == name){
            return true;
        }
    }
    return false;
}

int getDefSectionOffset(string name){
    for(auto& section : defSections){
        if(section.name == name){
            return section.offset;
        }
    }
    return 0;
}

struct AddendSection{
    string name;
    int addend;
};

list<AddendSection> addendSection;

void addAddendSection(string name, int size){
    AddendSection ads;
    ads.name = name;
    ads.addend = size;
    addendSection.push_back(ads);
}

void replaceAddendSection(string name, int size){
    for(auto& sec : addendSection){
        if(sec.name == name){
            sec.addend = size;
            return;
        }
    }
}

bool findAddendSection(string name){
    for(auto& sec : addendSection){
        if(sec.name == name){
            return true;
        }
    }

    return false;

}

int getAddendSection(string name){
    for(auto& ads : addendSection){
        if(ads.name == name){
            return ads.addend;
        }
    }
    return 0;
}

bool readSymbolTableFromFile(string line){
    istringstream iss(line);
    string name;
    string textOffset;
    int offset;
    string section;
    string textType;
    int type;
    string textLocal;
    bool local;
    iss >> name; // Ovde se nalazi ime simbola
    iss >> textOffset;

    offset = stoi(textOffset);
    iss >> section;
    iss >> textType;
    //cout <<"ime simbola "<< name << " offset " << textOffset << " seckija i njen addend " << section << " " <<getAddendSection(section) <<endl;
    if(textType == "SECTION"){
        type = SECTION;
    }else{
        type = SYMBOL;
    }
    iss >> textLocal;
    if(textLocal == "Local"){
        local = true;
    }else{
        local = false;
    }
    if(section == "?"){
        if(!searchSymbol(name)){
            addSymbol(name, type, section, offset + getAddendSection(section), local);
        }
        return true;
    }
    if(!searchSymbol(name)){
        addSymbol(name, type, section, offset + getAddendSection(section), local);
        return true;
    }else if(searchSymbol(name) && type == SECTION){ //Ako sekcija vec postoji, treba da je dodam u listu vec definisanih sekcija i povecam velicinu te sekcije u sekciji simbola
        if(!findAddendSection(name)){
            //cout <<"ime "<< name << " dodajem u addend i offset je " << getSymbolOffset(name) << endl;
            addAddendSection(name, getSymbolOffset(name));
            addDefinedSection(name, getSymbolOffset(name));
            addSectionSize(name, offset+getSymbolOffset(name));
        }else{
            replaceAddendSection(name, getSymbolOffset(name));
            addSectionSize(name, offset+getSymbolOffset(name));
        }
        return true;
    }else if(fixUndefinedSymbol(name, section, offset + getAddendSection(section))){
        return true;
    }else if(searchSymbol(name) && type == SYMBOL){
        cout << "Simbol " << name << " je vec definisan!" << endl;
        return false;
    }
    addSymbol(name, type, section, offset + getDefSectionOffset(section) + getAddendSection(section), local);
    
    return true;

}


void updateSymbolTableOffsets(){
    long offset;
    for(auto& sym : symTable){
        offset = getSectionOffsetPositionOfSection(sym.section);
        if(sym.type == SECTION){
            sym.offset = offset;
        }else{
            sym.offset += offset;
        }
    }
}

//END ZA LINKER
//----------------


//Funkcije vezane za RELOCATION TABLE -----------------------

//Dodaje simbol u relokacionu tabelu
void addSymbolToRelocationTable(string name, string section, int offset, int type){
    RelocationTable symbol;
    symbol.name = name;
    symbol.section = section;
    symbol.offset = offset;
    symbol.type = type;
    relocTable.push_back(symbol);
}

RelocationTable getFirstElementFromRelocationTable(){
    RelocationTable elem = relocTable.front();
    relocTable.pop_front();
    return elem;
}

void writeRelocationTableToFile(ofstream& outputFile) {
    outputFile << "RelokacionaTabela" << endl;
    outputFile << string(70, '-') << endl;
     // Write header line
    outputFile << left <<setw(20) << "Name"
               << setw(25) << "Section"
               << setw(15) << "Offset" 
               << setw(15) << "Type" << endl;

    // Write symbol table data
    for (const auto& symbol : relocTable) {
        outputFile << left << setw(20) << symbol.name
                   << setw(25) << symbol.section
                   << setw(15) << symbol.offset 
                   << setw(15) << symbol.type << endl;
    }
    outputFile << string(70, '-') << endl;
}

void readRelocTableFromFile(string line){
    istringstream iss(line);
    string name;
    string section;
    string textOffset;
    string typeText;
    long offset;
    int type;
    iss >> name;
    iss >> section;
    iss >> textOffset;
    iss >> typeText;
    type = stoi(typeText);
    offset = stol(textOffset);
    addSymbolToRelocationTable(name, section, offset, type);
}

void updateLasNElementsRelocationTable(int n, int offset, string section){
    auto it = relocTable.rbegin();  // Reverse iterator to start from the end
    
    // Iterate 'n' times or until reaching the beginning of the list
    for (int i = 0; i < n && it != relocTable.rend(); ++i, ++it) {
        if(it->section == section){ 
            it->offset += offset;
            //cout << it->name <<  " " << it->offset << " updated debug" <<endl;
        }
          // Update the value of the structure member
        // Update other members as needed
    }
}

bool sectionIsInRelocationTable(string section){
    for(auto& rel : relocTable){
        if(rel.section == section){
            return true;
        }
    }
    return false;
}

void updateRelocationTable(){
    
}

//END RELOCATION TABLE ---------------------------------------


//Stvari za poziciju sekcija-----------------------------


//Vraca true ako sekcija moze da stane, inace vraca false
bool checkIfSectionFitsIntoPositionOfSection(string section, int offset, int size){
    //Proveri prvo za adrese nize od offseta
    for(auto& symbol : sectionPosition){
        if(symbol.offset == offset){ // Ne mogu da budu na istoj poziciji
            return false;
        }else if(symbol.offset < offset && symbol.offset+symbol.size >= offset){
            return false;
        }else if(symbol.offset > offset && offset+size >= symbol.offset){
            return false;
        }
    }
    return true;
}

bool compareByOffset(const PositionOfSection& a, const PositionOfSection& b) {
    return a.offset < b.offset;
}

void addSectionToSectionPlacement(string section, long offset){
    if(!checkIfSectionFitsIntoPositionOfSection(section, offset, getSymbolOffset(section))){
        cout << "Sekcija " << section << " koja je argument opcije \"-place\" se preklapa sa nekom drugom sekcijom" << endl;
        exit(-1);
    }
    PositionOfSection positionOfSection;
    positionOfSection.name = section;
    positionOfSection.offset = offset;
    positionOfSection.size = getSymbolOffset(section);
    sectionPosition.push_back(positionOfSection);
    sectionPosition.sort(compareByOffset);
}


void defaultSectionPlacement(){
    int prevOffset = 0;
    for(auto& symbol : symTable){
        if(symbol.type == SECTION){
            PositionOfSection section;
            section.name = symbol.name;
            section.size = symbol.offset;
            section.offset = prevOffset;
            prevOffset += symbol.offset;
            sectionPosition.push_back(section);
        }
    }
}

//Vraca true ako je nasao simbol, false ako nije
bool checkIfSectionIsInPositionOfSection(string name){
    for(auto& symbol : sectionPosition){
        if(symbol.name == name){
            return true;
        }
    }
    return false;
}



void addRestOfTheSectionToPositionOfSection(){
    long prevOffset = sectionPosition.back().offset + sectionPosition.back().size;
    for(auto& symbol : symTable){
        if(symbol.type == SECTION){
            if(!checkIfSectionIsInPositionOfSection(symbol.name)){
                PositionOfSection section;
                section.name = symbol.name;
                section.size = symbol.offset;
                section.offset = prevOffset;
                prevOffset += symbol.offset;
                sectionPosition.push_back(section);
            }
        }
    }
    
}

void printPositionOfSection() {
    cout << "PositionOfSections" << endl;
    cout << string(70, '-') << endl;
     // Write header line
    cout << left << setw(20) << "Name"
               << setw(12) << "Size"
               << setw(40) << "offset" << endl;

    // Write symbol table data
    for (const auto& symbol : sectionPosition) {
        cout << left << setw(20) << symbol.name
                   << setw(12) << symbol.size
                   << setw(40) << symbol.offset << endl;
    }
    cout << string(70, '-') << endl;
}

//END stvari za poziciju sekcija-------------------------

//Pomoćne funkcije 

void printAllElements(){
    cout << "TabelaSimbola" << endl;
    cout << string(70, '-') << endl;
     // Write header line
    cout << left << setw(20) << "Name"
               << setw(12) << "Offset"
               << setw(25) << "Section"
               << setw(10) << "Type"
               << setw(10) << "Scope" 
               << setw(10) << "r.br" << endl;

    // Write symbol table data
    for (const auto& symbol : symTable) {
        string typeString = (symbol.type == 1) ? "SYMBOL" : (symbol.type == 2) ? "SECTION" : "UNKNOWN";
        cout << left << setw(20) << symbol.name
                   << setw(12) << symbol.offset
                   << setw(25) << symbol.section
                   << setw(10) << typeString
                   << setw(10) << (symbol.local ? "Local" : "Global") 
                   << setw(10) << symbol.num << endl;
    }
    cout << string(70, '-') << endl;
}

void printRelocationTable() {
    cout << "RelokacionaTabela" << endl;
    cout << string(70, '-') << endl;
     // Write header line
    cout << left <<setw(20) << "Name"
               << setw(25) << "Section"
               << setw(15) << "Offset" << endl;

    // Write symbol table data
    for (const auto& symbol : relocTable) {
        cout << left << setw(20) << symbol.name
                   << setw(25) << symbol.section
                   << setw(15) << symbol.offset << endl;
    }
    cout << string(70, '-') << endl;
}

#endif  // SYMTABLE_HPP
