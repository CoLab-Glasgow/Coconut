#include <fstream>
#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;


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
enum class FileState{ 
    INIT,
    OPEN,
    READ,
    CLOSE
    };

using File_protocol = Typestate_Template<
    State<FileState::INIT,  &File::OpenFile, FileState::OPEN>,
    State<FileState::OPEN, &File::read, FileState::READ>,
    State<FileState::READ, &File::ReadNext, FileState::READ>,
    State<FileState::READ,  &File::Close, FileState::CLOSE>,
    State<FileState::CLOSE,  &File::OpenFile, FileState::OPEN>
>;

using File_flag = TypestateClassConnector<File, File_protocol>;



File_flag fileflag;
File_protocol fprot;
void init(){
fprot.display();
fileflag.display();
}