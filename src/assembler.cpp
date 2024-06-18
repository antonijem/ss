#include "assembler.hpp"

extern FILE* yyin;

using namespace std;

int main(int argc, char* argv[]){
    string inputFileName;
    string outputFileName;
    if (argc > 2) {
        if(string(argv[1]) == "-o"){
            outputFileName = argv[2];
            inputFileName = argv[3];
        }else{
            cout << "Neispravna komanda, očekivano je -o. " << argv[1] <<  endl;
        }
    }else{
        inputFileName = argv[1];
        outputFileName = argv[1];
        outputFileName = outputFileName.substr(0, outputFileName.size() - 1);
        outputFileName += "o";
    }
   
    ifstream inputFile;
    inputFile.open(inputFileName);
    if (!inputFile) {
        // Error opening input file
        return 1;
    }
    // Associate the input file with stdin
    if (freopen(inputFileName.c_str(), "r", stdin) == nullptr) {
        // Error associating file with stdin
        return 1;
    }

    // Set the input file for the lexer
    yyin = stdin;

    // Call the parser
    if(yyparse() != 0){
        exit(-1);
    }

    //yyparse();

    // First pass
    //  Create symbol table, put all symbols as local(that will be changed in second pass)
    // Read the file line by line
    string line;
    int offset = 0;

    string curSection = "";
    while (getline(inputFile, line)) {
        istringstream iss(line);
        string wordInstr;
        bool skip = false;
        //Get first word
        iss >> wordInstr;

        if(wordInstr == ".section"){//Found a new section reset offset
            offset = 0;
            iss >> wordInstr;
            curSection = wordInstr;
            addSymbol(curSection, SECTION, curSection, 0, true);
            addSection(curSection); //Svaki put kada naiđem na sekciju dodajem je u tabelu sekcija
            //cout << "Found symbol section: " << word << " " << offset <<endl;
        }else if(wordInstr[wordInstr.size()-1] == ':'){//Found a symbol
            wordInstr = wordInstr.substr(0, wordInstr.size() - 1);
            if(searchSymbol(wordInstr)){
                cout << "Simbol " << wordInstr << " je već definisan." << endl;
                exit(-1);
            }
            if(curSection == ""){
                addSymbol("defaultSection", SECTION, "defaultSection", 0, true);
                addSection("defaultSection");
                curSection = "defaultSection";
            }
            addSymbol(wordInstr, SYMBOL, curSection, offset, true); //In the begining all of the symbols are local
            //cout << "Found symbol: " << word << " " << offset <<endl;
        }else if(wordInstr[0] == '.' || wordInstr == "#"){//Found assembler directive or comment don't increment
            //continue
        }
        else{//Every instruction is 4bytes long inc cnt +4
            offset += 4;
        }
        
    }

    //printAllElements();

    inputFile.close();
    
    //Second pass
    //Convert assembly code to machine code, complete symbol table, create relocation table 
    inputFile.open(inputFileName);
    if (!inputFile) {
        // Error opening input file
        return 1;
    }
    // Associate the input file with stdin
    if (freopen(inputFileName.c_str(), "r", stdin) == nullptr) {
        // Error associating file with stdin
        return 1;
    }

    curSection = "";
    offset = 0;
    string curSymbol = "";
    while (getline(inputFile, line)) {
        istringstream iss(line);
        string word;
        //Get first word
        iss >> word;

        if(word == ".section"){ //Naisao na sekciju, promeni curSection, resetuj offset na 0
            addSectionSize(curSection, offset);
            offset = 0;
            iss >> word;
            curSection = word;
        }else if(word == ".global"){
            iss >> word;
            string pom;
            iss >> pom;
            while((word.back() == ',' || pom == ",")){ //Imam vise symbola 
                if(word.back() == ',')
                    word = word.substr(0, word.length()-1);
                if(!searchSymbol(word)){ // Ukoliko ne nađe simbol znaci da postoji greška u kodu
                    addSymbol(word, SYMBOL, "?", 0 , false);
                    //cout << "Error symbol " << word << "nije definisan. " << endl;
                    //return -1;
                }else{
                    changeSymbolToGlobal(word);
                }
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
            //if(!searchSymbol(word)){ // Ukoliko ne nađe simbol znaci da postoji greška u kodu
            //    cout << "Error symbol " << word << " nije definisan. " << endl;
            //    return -1;
            //}
            changeSymbolToGlobal(word);
        }else if(word == ".extern"){ //Naišao na eksterni simbol dodaj ga u tabelu simbola, sekcija je ? i offset je 0 jer će linker to morati da razreši
            iss >> word;
            string pom;
            iss >> pom;
            while((word[word.size()-1] == ',' || pom == ",")){ //Imam vise symbola 
                if(word[word.size()-1] == ',')
                    word = word.substr(0, word.length()-1);
                if(searchSymbol(word)){ // Ukoliko nađe simbol znaci da postoji greška u kodu
                cout << "Error symbol " << word << " je već definisan. " << endl;
                return -1;
                }
                addSymbol(word, SYMBOL, "?", 0 , false);
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
            addSymbol(word, SYMBOL, "?", 0 , false);
        }else if(word[word.size() - 1] == ':'){ // Naišao na simbol 
            curSymbol = word.substr(0, word.size()-1);
            fixSymbolOffset(curSymbol, offset);
        }else if(word[0] == '#' || word == ""){ // Komentar samo prekosci

        }else if(word == ".end"){
            addSectionSize(curSection, offset);
            break;
        }else{ //Obradi instrukciju
            //if(curSection == ""){
            //    addSymbol("defaultSection", SECTION, "defaultSection", 0, true);
            //    addSection("defaultSection");
            //    curSection = "defaultSection";
            //}
            if(convertToMachine(curSection, line, offset, curSymbol) < 0){
                return -1;
            }
        }
        
    }
    calculateAllEquExpressions();
    if(!literalPool.empty()){
        addSymbol("literalPool", SECTION ,"literalPool", literalPool.back().offset + 4 , true);
    }
    //printTable();
    //cout << "------------------------"<<endl;
    //printAllElements();
    //cout << "------------------------"<<endl;
    //printRelocationTable();
    //cout << "------------------------"<<endl;
    //printDataTable();
    //cout << "------------------------"<<endl;
    //printLiteralPool();
    inputFile.close();


    ofstream outputFile(outputFileName);

    // Check if the file was opened successfully
    if (outputFile.is_open()) {
        
        writeSymbolTableToFile(outputFile);
        writeDataTableToFile(outputFile);
        writeRelocationTableToFile(outputFile);
        writeLiteralPoolToFile(outputFile);
        writeSectionTableToFile(outputFile);
        outputFile.close();
    } else {
        cout << "Failed to create the file." << endl;
    }

    return 0;
}