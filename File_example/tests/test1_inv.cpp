#include "../File.h"

int invalidTest1() {
    File_example file;
    (file->*&File::read)(); // Invalid: attempting to read without opening the file first.
    return 0;
}
