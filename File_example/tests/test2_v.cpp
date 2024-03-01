#include "../File.h"
int main() {
    File_example file;
    (file->*&File::OpenFile)(); // INIT -> OPEN
    (file->*&File::read)(); // OPEN -> READ
    (file->*&File::ReadNext)(); // READ -> READ (demonstrating multiple reads)
    (file->*&File::Close)(); // READ -> CLOSE
    return 0;
}
