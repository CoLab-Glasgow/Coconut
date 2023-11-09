#include <fstream>
#include "TypestateLibrary.h"
using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;
// file class with all methods to handle file objects 
class File {   
    std::string myfileName;
    std::ifstream file;
    std::string fileText;
public:
    File() { }
    void OpenFile() {
        file.open(myfileName.c_str());
    }
    void read() {
        if (getline(file, fileText)) std::cout << fileText;
    }
    void ReadNext() {
        if (getline(file, fileText)) std::cout << fileText;
    }
    void Close() {
        file.close();
    }
};

// define states of the protocol 
BETTER_ENUM(FileState, int,
    INIT,
    OPEN,
    READ,
    CLOSE)

using File_protocol = Typestate_Template<
    State<FileState::INIT,  &File::OpenFile, FileState::OPEN>,
    State<FileState::OPEN, &File::read, FileState::READ>,
    State<FileState::READ, &File::ReadNext, FileState::READ>,
    State<FileState::READ,  &File::Close, FileState::CLOSE>
>;

using File_example = TypestateClassConnector<File, File_protocol>;