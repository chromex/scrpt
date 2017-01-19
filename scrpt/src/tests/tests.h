#ifndef TESTS_H
#define TESTS_H

namespace scrpt
{
    void RunTests();

    namespace Tests 
    {
        std::shared_ptr<const char> DuplicateSource(const char* source);
        void RunTestsParser(unsigned int* passed, unsigned int* failed);
        void RunTestsLexer(unsigned int* passed, unsigned int* failed);
        void RunTestsBytecodeGen(unsigned int* passed, unsigned int* failed);
    }
}

#endif
