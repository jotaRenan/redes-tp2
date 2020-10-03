#ifndef INPUT_READER
#define INPUT_READER
#include <queue>
#include <string>
#include <fstream>
#include <iostream>

#include "utils.h"

using namespace std;

class InputReader {
    public:
        InputReader();
        string read();
        void read_file_to_buffer(const char* file_path);
        bool buffer_is_empty();

    private:
        bool read_from_file;
        queue<string> commands;
};
#endif