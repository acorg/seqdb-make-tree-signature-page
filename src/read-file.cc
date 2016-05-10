#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "read-file.hh"
#include "xz.hh"

// ----------------------------------------------------------------------

std::string read_file(std::string aFilename)
{
    std::string buffer;
    int f = open(aFilename.c_str(), O_RDONLY);
    if (f >= 0) {
        struct stat st;
        fstat(f, &st);
        buffer.resize(static_cast<std::string::size_type>(st.st_size), ' '); // reserve space
        if (read(f, &*buffer.begin(), static_cast<size_t>(st.st_size)) < 0)
            throw std::runtime_error(std::string("Cannot read ") + aFilename + ": " + strerror(errno));
        close(f);
    }
    else {
        throw std::runtime_error(std::string("Cannot open ") + aFilename + ": " + strerror(errno));
    }
    if (xz_compressed(buffer))
        buffer = xz_decompress(buffer);
    return buffer;
}

// ----------------------------------------------------------------------

std::string read_from_file_descriptor(int fd, size_t chunk_size)
{
    std::string buffer;
    std::string::size_type offset = 0;
    for (;;) {
        buffer.resize(buffer.size() + chunk_size, ' ');
        const auto bytes_read = read(fd, (&*buffer.begin()) + offset, chunk_size);
        if (bytes_read < 0)
            throw std::runtime_error(std::string("Cannot read from file descriptor: ") + strerror(errno));
        if (static_cast<size_t>(bytes_read) < chunk_size) {
            buffer.resize(buffer.size() - chunk_size + static_cast<size_t>(bytes_read));
            break;
        }
        offset += static_cast<size_t>(bytes_read);
    }
    return buffer;
}

// ----------------------------------------------------------------------

void write_file(std::string aFilename, std::string aData)
{
    int f = -1;
    if (aFilename == "-") {
        f = 1;
    }
    else if (aFilename == "=") {
        f = 2;
    }
    else {
        if (aFilename.size() > 3 && aFilename.substr(aFilename.size() - 3) == ".xz")
            aData = xz_compress(aData);
        f = open(aFilename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (f < 0)
            throw std::runtime_error(std::string("Cannot open ") + aFilename + ": " + strerror(errno));
    }
    if (write(f, aData.c_str(), aData.size()) < 0)
        throw std::runtime_error(std::string("Cannot write ") + aFilename + ": " + strerror(errno));
    if (f > 2)
        close(f);

} // write_file

// ----------------------------------------------------------------------
