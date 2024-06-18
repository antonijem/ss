#include "emulator.hpp"

int main(int argc, char* argv[]){

    if(argc < 2){
        cout << "Emulator se mora pozvati sa argumentom, tj imenom fajla koji pokrećete. " << endl;
        exit(-1);
    }

    string inputFileName = argv[1];

    loadCodeToMemory(inputFileName);
    loadTimerIntoMemory();
    setTerminalMemoryLocation();
    setTerminalForNonBlocking();
    discardInput();

    emulateCode();
    printRegisterValues();
    
    return 0;
}