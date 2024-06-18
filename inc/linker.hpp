#ifndef LINKER_HPP
#define LINKER_HPP

#include "symTable.hpp"
#include "section.hpp"
#include <regex>

#define HEX 3
#define RELOC 4


using namespace std;


string littleEndianToRegularHex(const string& littleEndianHex) {
    string regularHex = littleEndianHex.substr(2); // Remove the "0x" prefix

    // Reverse the string
    reverse(regularHex.begin(), regularHex.end());

    // Swap adjacent characters to restore the original order
    for (size_t i = 0; i < regularHex.length(); i += 2) {
        swap(regularHex[i], regularHex[i + 1]);
    }

    return "0x" + regularHex; // Add back the "0x" prefix
}


//Funkcije vezane za ARGUMENTE -----------------------

//Za proveru imena izlaznog fajla
bool hasNoExtension(const std::string& str) {
    std::regex pattern("\\.(?!.*\\.)");
    return !std::regex_search(str, pattern);
}

bool hasHexExtension(const std::string& str) {
    std::regex pattern("\\.hex$");
    return std::regex_search(str, pattern);
}


//Za proveru ulaznih fajlova
bool endsWithDotO(const std::string& str) {
    regex pattern("\\.o$");
    return std::regex_search(str, pattern);
}

//Za proveru -place=<ime_sekcije>@<adresa>
bool isValidFormatForPlaceArg(const std::string& str) {
    regex pattern("-place=\\w+@0x[0-9a-fA-F]+");

    return regex_match(str, pattern);
}

bool checkValidityOfArguments(string argumenti[], int numArg, bool &generateOutputFile, int &outFilesLocation, int &type, bool &definedPlaces){
    bool hex = false;
    bool reloc = false;
    bool options = true; //Po default cu staviti kao da ima opcije
    int iter = 0;
    while(iter < numArg){
        if(argumenti[iter][0] == '-'){
            if(!options){ //Nije trebalo da ima vise opcija validnost argumenata nije dobra
                cout << "Pogresan redosled argumenata, prvo se navode sve opcije pa onda imena ulaznih fajlova! "<< endl;
                return false;
            }
            if(argumenti[iter] == "-o"){
                iter++;
                if(argumenti[iter][0] != '-'){ //Sledeci argument samo treba da bude neko ime
                    if(!hasHexExtension(argumenti[iter])){
                        cout << "Ime izlaznog fajla treba da ima .hex ekstenziju! " << endl;
                        return false;
                    }
                }else{
                    cout << "Nakon opcije -o neophodno je napisati ime izlaznog fajla. Ime ne sme pocinjati sa - !!!" << endl;
                    return false;
                }
                iter++;
                continue;
            }else if(argumenti[iter] == "-hex"){
                if(hex || reloc){
                    cout << "Vec je naveden argument " << (hex ? "hex" : "reloc") << endl;
                    return false;
                }
                type = HEX;
                hex = true;
                iter++;
                continue;
            }else if(argumenti[iter] == "-relocatable" ){
                if(hex || reloc){
                    cout << "Vec je naveden argument " << (hex ? "hex" : "reloc") << endl;
                    return false;
                }
                type = RELOC;
                reloc = true;
                iter++;
                continue;
            }else if(isValidFormatForPlaceArg(argumenti[iter])){
                definedPlaces = true;
                iter++;
                continue;
            }
            cout << "Nije prepoznat argument: " << argumenti[iter] << endl;
            return false;
        }else{ //Ako je ovde znaci da je taj argument ulazan podatak
            if(options){
                options = false;
                outFilesLocation = iter;
            }
            if(!endsWithDotO(argumenti[iter])){ //Proveravam da li je validan .o fajl
                cout << "Ulazni fajlovi moraju biti .o ekstenzije! " << argumenti[iter] << endl;
                return false;
            }
            iter++;
            //Ovde nemam neke posebne provere jer ako ime fajla nije dobro
            //kasnije nece uspeti da ga otvori
        }
    }
    if(hex || reloc){
        generateOutputFile = true;
    }
    return true;
}

//END ARGUMENTE ---------------------------------------


string decimalToHex(int decimalNumber) {
    stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0') << decimalNumber;
    return ss.str();
}

void writeLinkerToOutputFile(ofstream& outputFile){
    int prevOffset = 0;
    bool first = true;
    bool nwln = false;
    for(auto& pos : sectionPosition){
        nwln = true;
        int fill = 0;
        bool toFill = false;
        int offset = pos.offset;
        bool nastavi = false;
        int count;
        string hexOffset;
        // U fajlu ce adrese biti zapisivane samo kao mod 8 tj 00 -> 08 -> 10 -> 18 itd
        if(offset % 8 != 0){
            if(offset - (offset%8) != prevOffset){ // Ne pocinje tamo gde se prethodni zavrsio
                hexOffset = decimalToHex(offset - (offset%8));
                fill = offset % 8;
                if(count % 8 == 0){
                    nwln = false;
                }
                count = 0;
                offset = offset - (offset%8);
                toFill = true;
            }else{
                offset = offset - (offset%8);
                nastavi = true;
            }
        }else{ // Treba nam pocetak nove adrese
            if(!first){
                outputFile << endl;
            }
            hexOffset = decimalToHex(offset);
            count = 0;
        }
        if(toFill){
            if(nwln){
                outputFile << endl;
            }
            outputFile << hexOffset << " : " ;
            for(int i=0 ; i < fill; i++){
                outputFile << "xx ";
                count++;
            }
            nastavi = true;
        }
        for (const auto& section : table) {
            if(section.name == pos.name){
                if(!nastavi){
                    outputFile << hexOffset << " : " ; //Odavdee u falju upisan nekiHexBroj : 
                }
                std::istringstream iss(section.instructions);
                std::string instruction;

                while (iss >> instruction) {
                    if (count == 8) {
                        offset += 8;
                        hexOffset = decimalToHex(offset);
                        outputFile << endl;
                        outputFile << hexOffset << " : " ;
                        count = 0;
                    }
                    outputFile << instruction << " ";
                    count++;
                }
            }
        }
        prevOffset = offset;
        first = false;
    }
}

#endif // LINKER_HPP