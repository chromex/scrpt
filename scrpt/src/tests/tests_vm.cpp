#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_VM"

static bool TestVM(const char* testName, const char* source);

void scrpt::Tests::RunTestsVM(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed
    ACCUMTEST(TestVM("Simple counting", R"testCode(
func main() {
    sum = 0;
    do
    {
        ++sum;
    } while(sum < 100000);
    return sum;
}
)testCode"));
}

static bool TestVM(const char* testName, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "VM: " << testName << std::endl;

    bool err = false;
    scrpt::Lexer lexer(scrpt::Tests::DuplicateSource(source));
    scrpt::Parser parser;
    scrpt::BytecodeGen compiler;
    try
    {
        parser.Consume(&lexer);
        compiler.Consume(*parser.GetAst());
        
        // TODO: Get bytecode object
        // Give it to a VM
        // Run it
        // Get ret val
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
