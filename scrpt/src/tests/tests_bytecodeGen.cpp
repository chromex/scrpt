#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_BytecodeGen"

static bool TestBytecodeGen(const char* testName, bool dumpBytecode, const char* source);

void scrpt::Tests::RunTestsBytecodeGen(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed
}

bool TestBytecodeGen(const char* testName, bool dumpBytecode, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "Bytecode Gen: " << testName << std::endl;

    //scrpt::ParseErr err = scrpt::ParseErr::NoError;
    //scrpt::Parser parser;
    //scrpt::Lexer lexer(scrpt::Tests::DuplicateSource(source));
    //try
    //{
    //    parser.Consume(&lexer);
    //    if (dumpAst) parser.DumpAst();
    //}
    //catch (scrpt::CompilerException& ex)
    //{
    //    std::cout << ex.what() << std::endl;
    //    err = ex.GetParseErr();
    //    parser.DumpAst();
    //}

    bool passed = true;

    if (passed)
    {
        std::cout << "<<<Test Passed>>>" << std::endl;
    }
    else
    {
        std::cout << "<<<Test Failed>>>" << std::endl;
    }

    std::cout << std::endl;
    return passed;
}
