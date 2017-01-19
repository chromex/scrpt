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

    bool err = false;
    scrpt::Lexer lexer(scrpt::Tests::DuplicateSource(source));
    scrpt::Parser parser;
    scrpt::BytecodeGen compiler;
    try
    {
        parser.Consume(&lexer);
        compiler.Consume(parser.GetAst());
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        err = true;
        compiler.DumpBytecode();
    }

    if (!err)
    {
        std::cout << "<<<Test Passed>>>" << std::endl;
    }
    else
    {
        std::cout << "<<<Test Failed>>>" << std::endl;
    }

    std::cout << std::endl;
    return !err;
}
