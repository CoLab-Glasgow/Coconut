//
//  File.cpp
//  File_example
//
//  Created by Arwa Alsubhi on 23/11/2021.
//


#include <fstream>
#include "TypestateLibrary.h"
using namespace std;

// file class with all methods to handle file objects 
class File{
    string myfileName;
    ifstream file;
    string fileText;
    
public:
    File(){
        
    }
    void OpenFile(){
        file.open(myfileName.c_str());
        
    }
    void read(){
        
       
        if (getline(file, fileText)) cout << fileText;
        
    }
    void ReadNext(){
        
        if (getline(file, fileText)) cout << fileText;
        
    }
    void Close(){
        file.close();
    }
    
  
};

// define states of the protocol 
 enum class FileState{
    INIT,
    OPEN,
    READ,
    CLOSE,
};

// exctract the templates 
using TypestateTool::State;
using TypestateTool::typestate;



// defined the protocol

using File_protocol = typestate<
    State<FileState::INIT, FileState::OPEN, &File::OpenFile>,
    State<FileState::OPEN, FileState::READ, &File::read>,
    State<FileState::READ, FileState::READ, &File::ReadNext>,
    State<FileState::READ, FileState::CLOSE, &File::Close>
>;
// assign it to class


Assign_to_Class(File, File_protocol);


int main(int argc, const char * argv[]) {
    // insert code here...
    File file;
    file.OpenFile();
    file.read();
    file.ReadNext();
    file.Close();
    return 0;
}

