#include "File.h"
int main(int argc, const char* argv[]) {
    // insert code here...
    File_example file;
    (file->*&File::OpenFile)();
    (file->*&File::read)();
    (file->*&File::ReadNext)();
    (file->*&File::Close)();

   
    // this is won't compile 
    //(file->* & File::OpenFile)();
    //(file->* & File::read)();
    //(file->* & File::Close)();
    //(file->* & File::ReadNext)();
    return 0;
}
