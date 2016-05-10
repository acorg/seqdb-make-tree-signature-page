#pragma once

#include <string>
#include <sys/stat.h>

// ----------------------------------------------------------------------

bool file_exists(std::string aFilename);
std::string read_file(std::string aFilename);
std::string read_from_file_descriptor(int fd, size_t chunk_size = 1024);
std::string read_stdin();
void write_file(std::string aFilename, std::string aData);

// ----------------------------------------------------------------------

inline bool file_exists(std::string aFilename)
{
    struct stat buffer;
    return stat(aFilename.c_str(), &buffer) == 0;
}

// ----------------------------------------------------------------------

inline std::string read_stdin()
{
    return read_from_file_descriptor(0);
}

// ----------------------------------------------------------------------
