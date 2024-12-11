#include "File.h"

int main(int argc, const char* argv[]) {
    File file;
    File& file2=file;
    file.OpenFile();
    file2.read();
    file.ReadNext();
    file.Close();

    // this is won't compile 
    // file.OpenFile();
    // file.Close();
    // file.read();
   
    return 0;
}
