#include "../File.h"
int main() {
    File_example file;
    (file->*&File::Close)(); // Invalid: attempting to close without opening the file first.
    return 0;
}
