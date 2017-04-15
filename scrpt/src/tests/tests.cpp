#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests"

void scrpt::RunTests()
{
    unsigned int passed = 0;
    unsigned int failed = 0;

    Tests::RunTestsLexer(&passed, &failed);
    Tests::RunTestsParser(&passed, &failed);
    Tests::RunTestsBytecodeGen(&passed, &failed);
    Tests::RunTestsVM(&passed, &failed);

    std::cout << passed << " passed and " << failed << " failed" << std::endl;
}

std::shared_ptr<const char> scrpt::Tests::DuplicateSource(const char* source)
{
    AssertNotNull(source);
    size_t len = strlen(source);
    char* copy(new char[len + 1]);
    strcpy_s(copy, len + 1, source);
    return std::shared_ptr<const char>(copy, std::default_delete<const char[]>());
}
