#include "..\scrpt.h"

#define COMPONENTNAME "FileIO"

std::unique_ptr<char[]> scrpt::ReadFile(const fs::path& path)
{
    // open
    FILE *fin;
    errno_t err = fopen_s(&fin, path.string().c_str(), "rb");
    if (err != 0)
    {
        AssertFail("Failed to open file for reading: " << path);
        return nullptr;
    }

    // compute size
    fseek(fin, 0, SEEK_END);
    size_t fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char* buffer = new char[fsize + 1];

    if (fsize != fread(buffer, sizeof(char), fsize, fin))
    {
        delete[] buffer;
        buffer = 0;
    }
    else
    {
        buffer[fsize] = 0;
    }

    fclose(fin);

    return std::unique_ptr<char[]>(buffer);
}

void scrpt::WriteFile(const fs::path& path, const char* content, size_t nBytes)
{
    FILE *fout;
    errno_t err = fopen_s(&fout, path.string().c_str(), "wb");
    if (!fout)
    {
        AssertFail("Failed to open file for writing: " << path);
    }

    size_t n = fwrite(content, 1, nBytes, fout);
    fclose(fout);

    if (n != nBytes)
    {
        AssertFail("Failed to write total contents to file: " << path);
    }
}
