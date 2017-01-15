#ifndef FILEIO_H
#define FILEIO_H

namespace scrpt
{
    std::unique_ptr<char[]> ReadFile(const fs::path& path);

    void WriteFile(const fs::path& path, const char* content, size_t nBytes);
}

#endif
