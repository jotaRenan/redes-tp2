#include "input_reader.hpp"

InputReader::InputReader()
{
    this->commands = queue<string>();
    this->read_from_file = false;
}

string InputReader::read()
{
    string result;
    if (this->read_from_file)
    {
        result = this->commands.front();
        this->commands.pop();
    }
    else
    {
        cin >> result;
    }
    return result;
}

void InputReader::read_file_to_buffer(const char *file_path)
{
    ifstream infile;
    infile.open(file_path, ios::in);
    if (!infile.is_open())
    {
        logexit("fileopen");
    }
    else
    {
        this->read_from_file = true;
        string input;
        while (infile >> input)
        {
            this->commands.push(input);
        }
        infile.close();
    }
}

bool InputReader::buffer_is_empty()
{
    return !this->read_from_file || this->commands.empty();
}
