#include "scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_BytecodeGen"

static bool TestBytecodeGen(const char* testName, bool dumpBytecode, const char* source);

void scrpt::Tests::RunTestsBytecodeGen(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed
    ACCUMTEST(TestBytecodeGen("Basic Do Loop", false, R"testCode(
func main() {
    sum = 0;
    do
    {
        ++sum;
    } while(sum < 100000);
    return sum;
}
)testCode"));

	ACCUMTEST(TestBytecodeGen("Basic For Loop", false, R"testCode(
func main() {
    for (sum = 0; sum < 10; sum += 1)
        i = 0;
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("Basic While Loop", false, R"testCode(
func main() {
    while (true) {}
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("Basic Bracket", false, R"testCode(
func main() {
    i = 0;
    {
        sum = i + 1;
    }
    return i;
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("For Loop Emtpy Children", false, R"testCode(
func main() {
    for (i = 0; ; ++i) {}

    a = 0;
    for (; a < 10; ++a) {}

    for (i = 0; i < 10; ) {}
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("Basic If", false, R"testCode(
func main() {
    if (true) {a = 0;}
    if (false) {a = 0;} else {b = 0;}
    if (false) {a = 0;} elif (true) {b = 0;} else {c = 0;}
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("Assignment", false, R"testCode(
func main() {
    a = 4;
    b = 2;
    b += ++a * 3;
    return b;
}
)testCode"));

    ACCUMTEST(TestBytecodeGen("Parameters", false, R"testCode(
func main(one, two) {
    a = one;
    two += a;
}
)testCode"));
}

bool TestBytecodeGen(const char* testName, bool dumpBytecode, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "B|" << testName << "> ";

    bool err = false;
    scrpt::Lexer lexer(scrpt::Tests::DuplicateSource(source));
    scrpt::Parser parser;
    scrpt::BytecodeGen compiler;
    try
    {
        parser.Consume(&lexer);
        compiler.Consume(*parser.GetAst());
        if (dumpBytecode) Decompile(compiler.GetBytecode());
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        err = true;
        Decompile(compiler.GetBytecode());
    }

    if (!err)
    {
        std::cout << "Passed" << std::endl;
    }
    else
    {
        std::cout << "<<< Failed >>>" << std::endl;
    }

    return !err;
}