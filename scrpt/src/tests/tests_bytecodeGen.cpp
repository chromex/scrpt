#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_BytecodeGen"

static bool TestBytecodeGen(const char* testName, bool dumpBytecode, const char* source);

void scrpt::Tests::RunTestsBytecodeGen(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed
    ACCUMTEST(TestBytecodeGen("Hello World", true, R"testCode(
func main() {
    sum = 0;
    for (i = 0; i < 1000; ++i)
        if (test(i, 5, 3))
            sum += i;
}

func test(val, m, v) { return val % m == 0 || val % v == 0; }
)testCode"));

    ACCUMTEST(TestBytecodeGen("Hello World2", true, R"testCode(
func main() {
    sum = 0;
    val1 = 1;
    val2 = 2;

    while (val2 < 4000000)
    {
         if (val2 % 2 == 0) sum += val2;
         t = val2;
         val2 += val1;
         val1 = t;
    }

    return sum;
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("Basic Do", true, R"testCode(
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
        compiler.Consume(*parser.GetAst());
        if (dumpBytecode) compiler.DumpBytecode();
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
