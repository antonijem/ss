#include "linker.hpp"


int main(int argc, char* argv[]){

    if(argc < 2){
        cout << "Nedovoljan broj argumenata! " << endl;
        return -1;
    }
    
    bool generateOutputFile = false;
    int numArg = argc - 1;
    string argumenti[numArg];    
    
    //Ucitavam argumente linkera 
    for(int i=0; i<numArg; i++){
        argumenti[i] = argv[i+1];
    }

    //Proveravam da li su pravilno upisani
    bool definedPlaces = false;
    int type;
    int outFilesLocation;
    if(!checkValidityOfArguments(argumenti, numArg, generateOutputFile, outFilesLocation, type, definedPlaces)){
        return -1;
    }

    int iter = outFilesLocation;
    //Citam sve ulazne fajlove
    while(iter < numArg){
        
        ifstream inputFile;
        inputFile.open(argumenti[iter]);
        if (!inputFile) {
            // Error opening input file
            return -1;
        }
        // Associate the input file with stdin
        if (freopen(argumenti[iter].c_str(), "r", stdin) == nullptr) {
            // Error associating file with stdin
            return -1;
        }

        //Odavde citam meta podatke za dalji rad linkera
        //Popunjavam sve tabele koje su mi neophodne
        /*
        TabelaSimbola
        --------------
        Name               
        .....
        data
        .....
        ------------
        DataTabela
        -----------
        Name        
        .....
        data
        .....                                                     
        ----------
        RelokacionaTabela
        ----------
        Name          
        .....
        data
        ....                                                    
        -------
        Sections
        -------
        ....
        data
        */
        string line;
        int cntRel = 0;
        while (getline(inputFile, line)) {
            istringstream iss(line);
            string word;
            iss >> word;
            if (word == "TabelaSimbola") {
                getline(inputFile, line); // Skip "----"
                getline(inputFile, line); // Skip information line
                getline(inputFile, line); // Get the first data line

                iss.str(line); // Update istringstream with the new line
                iss.clear();   // Clear the error flags

                while (line[0] != '-') {
                    if(!readSymbolTableFromFile(line)){
                        
                    }
                    getline(inputFile, line);
                    iss.str(line); // Update istringstream with the new line
                    iss.clear();   // Clear the error flags
                }
                
            } else if (word == "DataTabela") {
                getline(inputFile, line); // Skip "----"
                getline(inputFile, line); // Skip information line
                getline(inputFile, line); // Get the first data line

                iss.str(line); // Update istringstream with the new line
                iss.clear();   // Clear the error flags

                while (line[0] != '-') {
                    readDataTableFromFile(line);
                    getline(inputFile, line);
                    iss.str(line); // Update istringstream with the new line
                    iss.clear();   // Clear the error flags
                }
                  
            } else if (word == "RelokacionaTabela") {
                getline(inputFile, line); // Skip "----"
                getline(inputFile, line); // Skip information line
                getline(inputFile, line); // Get the first data line

                iss.str(line); // Update istringstream with the new line
                iss.clear();   // Clear the error flags

                while (line[0] != '-') {
                    cntRel++;
                    readRelocTableFromFile(line);
                    getline(inputFile, line);
                    iss.str(line); // Update istringstream with the new line
                    iss.clear();   // Clear the error flags
                }
                
            } else if(word == "LiteralPool"){
                getline(inputFile, line); // Skip "----"
                getline(inputFile, line); // Skip information line

                getline(inputFile, line); // Get the first data line

                iss.str(line); // Update istringstream with the new line
                iss.clear();   // Clear the error flags

                while (line[0] != '-') {
                    readLiteralPoolFromFile(line);
                    getline(inputFile, line);
                    iss.str(line); // Update istringstream with the new line
                    iss.clear();   // Clear the error flags
                }
                
            } else if (word == "Sections") {
                getline(inputFile, line); // Skip "----"
                while (getline(inputFile, line)) {
                    iss.str(line); // Update istringstream with the new line
                    iss.clear();   // Clear the error flags
                    string section;
                    iss >> section;
                    string textSize;
                    iss >> textSize;
                    int size = stoi(textSize);
                    if(section == ""){
                        break;
                    }
                    if(!searchSection(section)){
                        addSection(section);
                    }
                    
                    addNonStandardSizeInstruction(section, "", size);
                    if(getSectionSize(section) != size && cntRel != 0 && sectionIsInRelocationTable(section)){ // Azuriraj offsete relokacionih tabela
                        updateLasNElementsRelocationTable(cntRel,getSectionSize(section)-size, section);
                    }
                    getline(inputFile, line); // odavde pocinju vrednosti sekcije
                    iss.str(line);
                    iss.clear();
                    int i;
                    for(i=0; i<size; ){
                        iss.clear();
                        iss >> word;
                        i++;
                        addNonStandardSizeInstruction(section, word + " ", 0);
                        if(i % 8 == 0){
                            getline(inputFile, line);
                            iss.str(line);
                            iss.clear();
                        }
                    }
                    if(i % 8 != 0){ // Za slucaj da sekcija nije poravnata na 32bita onda moramo rucno jos jednu liniju da ucitamo
                        getline(inputFile, line);
                        iss.str(line);
                        iss.clear();
                    }
                }
            } else {
                cout << "Neispravan format ulaznog fajla!" << endl;
                return -1;
            }   
        }
        inputFile.close();
        iter++;
    }

/*    
    printAllElements();
    printDataTable();  
    printRelocationTable();
    printTable();
    printLiteralPool();
*/
    //U zavisnoti koji je argument zadat, takav ćemo fajl praviti
    if(type == HEX){
        if(definedPlaces){ // Zadati su argumenti za eksplicitno smestanje sekcija na neku adresu
            for(int i=0; i<outFilesLocation; i++){ //Idem do outFilesLocation jer odatle pocinju ulazni podaci i posle toga nema opcija
                regex pattern(R"(-place=([^@]+)@(0x[0-9a-fA-F]+))");
                smatch matches;
                if (regex_search(argumenti[i], matches, pattern)) {
                    string sectionName = matches[1];
                    string address = littleEndianToRegularHex(matches[2]);
                    //cout << address << " adresa za place " << endl;
                    long offset = stol(address, nullptr, 16);
                    if(!searchSymbol(sectionName)){
                        cout << "Sekcija " << sectionName << " koja je prosleđena kao arugment opcije \"-place\" ne postoji." << endl;
                        exit(-1);
                    }
                    addSectionToSectionPlacement(sectionName, offset);
                } 
            }
            addRestOfTheSectionToPositionOfSection();
        }else{
            defaultSectionPlacement(); 
        }
        updateSymbolTableOffsets();
        transferSymbolTableIntoSection();
        addSymbolTableSectionPositionOfSection();
        //printTable();
        //printAllElements();
        //printPositionOfSection();
        while(!relocTable.empty()){ // Ovde rešavam sve probleme oko relokacija
            RelocationTable elem = getFirstElementFromRelocationTable();
            string inst = getInstructionFromSectionBasedOnOffset(elem.section, elem.offset);
            long relocationOffset = 0;
            if(string(1, inst[7]) + string(1, inst[9]) + string(1, inst[10]) != "???"){
                //if(findSymbolDataSection(elem.name)){
                //    cout << elem.name << " prvi if "  << endl;
                //    relocationOffset += getSymbolFromSymbolTable(elem.name).offset;
                //}else{
                //    cout << elem.name << " prvi else "  << endl;
                //    //long offsetJmp = hexToDecimal(string(1, inst[7]) + string(1, inst[9]) + string(1, inst[10]));
                //    relocationOffset = getSymbolOffset(elem.name);
                //    // offsetJmp + getSectionOffsetPositionOfSection(elem.section) + elem.offset;
                //    cout << elem.name << ": " << relocationOffset << endl;
                //}
                if(doesLiteralExistLiteralPool(elem.name)){
                    long symbolOffset;
                    symbolOffset += getSymbolOffset("literalPool");
                    symbolOffset += getOffsetLiteralPool(elem.name);
                }else if(elem.type == VALUE){
                    relocationOffset = getSymbolFromSymbolTable(elem.name).offset;
                }else{
                    //relocationOffset = hexToDecimal(string(1, inst[7]) + string(1, inst[9]) + string(1, inst[10]));
                    //cout << string(1, inst[7]) + string(1, inst[9]) + string(1, inst[10]) << " " << " displ " << elem.name << endl;
                    relocationOffset += getSymbolFromSymbolTable(elem.name).offset;
                    //cout << relocationOffset << " " << " displ + symbol offset" << elem.name << endl;
                }
            }else{
                long addend = elem.offset + getSectionOffsetPositionOfSection(elem.section);
                long symbolOffset = 0;
                //if(doesLiteralExistLiteralPool(elem.name)){
                //    cout << elem.name << " drugi if "  << endl;
                //    symbolOffset += getSymbolOffset("literalPool");
                //    symbolOffset += getOffsetLiteralPool(elem.name);
                //}else if(findSymbolDataSection(elem.name)){
                //    cout << elem.name << " else if "  << endl;
                //    symbolOffset += getSymbolFromSymbolTable(elem.name).offset;
                //}else{
                //    cout << elem.name << " 2 else "  << endl;
                //    cout <<elem.name<< getSectionOffsetPositionOfSection("symbolTable") << " lokacia tabele simbola " << getSymbolFromSymbolTable(elem.name).num << endl;
                //    symbolOffset += getSymbolOffset(elem.name);
                //    //getSectionOffsetPositionOfSection("symbolTable") + getSymbolFromSymbolTable(elem.name).num*4;//                primer inst ab cd ef gh
                //}
                if(doesLiteralExistLiteralPool(elem.name)){
                    
                    symbolOffset += getSymbolOffset("literalPool");
                    symbolOffset += getOffsetLiteralPool(elem.name);
                    //printLiteralPool();
                }else if(elem.type == VALUE){
                    symbolOffset = getSectionOffsetPositionOfSection("symbolTable") + getSymbolFromSymbolTable(elem.name).num*4;
                    //getSymbolFromSymbolTable(elem.name).offset;
                }else{
                    symbolOffset = getSymbolFromSymbolTable(elem.name).offset;
                }
                //cout << symbolOffset << " of simbola " << endl;
                //long relocationOffset = symbolOffset - (elem.offset + getSymbolOffset(elem.section));//               poz elem 012345678910
                //cout << relocationOffset << " skok " << endl;
                //if(relocationOffset > 2047 || relocationOffset < -2048){
                //    cout << "Skok je prevelik za simbol " << elem.name << endl;
                //    exit(-1);
                //}
                relocationOffset = symbolOffset;
            }
            //cout << relocationOffset << " " << elem.name << endl;
            string relocOffsetText = stringToLittleEndianHex(to_string(relocationOffset));
            inst[7] = relocOffsetText[7];
            inst[9] = relocOffsetText[9];
            inst[10] = relocOffsetText[10];
            fixOffsetInInstruction(elem.section, elem.offset, inst); // Prepravljamo instrukciju koja je u sebi imala ? ?? 

            if(isSymbolExtern(elem.name)){
                cout << "Simbol " << elem.name << " nije i dalje razrešen! Linker obustavalja rad." << endl;
                exit(-1);
            } 

        }

        string outputFileName;
        for(int i=0; i<numArg; i++){
            if(argumenti[i] == "-o"){
                outputFileName = argumenti[i+1];
                break;
            }
        }
        ofstream outputFile(outputFileName);

        // Check if the file was opened successfully
        if (outputFile.is_open()) {
            writeLinkerToOutputFile(outputFile);
            outputFile.close();
        } else {
            cout << "Failed to create the file." << endl;
        }

    }else if(type == RELOC){
        defaultSectionPlacement();
        string outputFileName;
        for(int i=0; i<numArg; i++){
            if(argumenti[i] == "-o"){
                outputFileName = argumenti[i+1];
                break;
            }
        }
        ofstream outputFile(outputFileName);

        // Check if the file was opened successfully
        if (outputFile.is_open()) {

            writeSymbolTableToFile(outputFile);
            writeDataTableToFile(outputFile);
            writeRelocationTableToFile(outputFile);
            writeSectionTableToFile(outputFile);
            outputFile.close();
        } else {
            cout << "Failed to create the file." << endl;
        }
    }
    //printPositionOfSection();
    //printAllElements();
    //Odavde krece obrada informacija kojih sam dobio od ulaznih fajlova


    return 0;
}