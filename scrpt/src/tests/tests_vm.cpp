#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_VM"

static bool TestVM(const char* testName, int resultValue, const char* source);

void scrpt::Tests::RunTestsVM(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed
    ACCUMTEST(TestVM("Simple counting", 100000, R"testCode(
func main() {
    sum = 0;
    do
    {
        ++sum;
    } while(sum < 100000);
    return sum;
}
)testCode"));

    ACCUMTEST(TestVM("Fibonacci", 317811, R"testCode(
func main() {
    v0 = 0;
    v1 = 1;
    while (v1 < 300000)
    {
        t = v0 + v1;
        v0 = v1;
        v1 = t;
    }
    return v1;
}
)testCode"));
}

static bool TestVM(const char* testName, int resultValue, const char* source)
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
        scrpt::Bytecode bytecode = compiler.GetBytecode();
        Decompile(bytecode);
        scrpt::VM vm(&bytecode);
        scrpt::VM::StackVal* ret = vm.Execute("main");
        err = ret == nullptr || ret->integer != resultValue;
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        err = true;
        Decompile(compiler.GetBytecode());
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
