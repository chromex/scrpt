#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_Parser"

static bool TestParse(const char* testName, scrpt::ParseErr expectedErr, bool dumpAst, const char* source);

void scrpt::Tests::RunTestsParser(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed

    ACCUMTEST(TestParse("Parse Constants", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    num = 0;
    num = 0.1;
    num = 1.2;
    num = -2.5;
    num = -0;
    str = "";
    str = "asldfkj";
    bool = true;
    bool = false;
    list = [];
    list = [1, 2, "whut"];
    list = [[1], 3];
    list = ["whoa", []];
    list = [val1, call()];
    dict = {};
    dict = {"test": 1};
    dict = {"test": 2, "test2": 3};
    dict = 
        {
            "test": {
                "second": "no"
            }, 
            "what": 1
        };
    dict = {genKey(): 1 + 2 * 3};
}
)testCode"));

    ACCUMTEST(TestParse("Parse Loops", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    while (true) { a = 1; }
    do {} while(true);
    for (a = 0; a < 10; ++a) {}
    while (true) { }
}
)testCode"));

    ACCUMTEST(TestParse("Indexing Test", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    char = "str"[]; // Technically illegal
    char = "str"[0];
    char = "str"[1:];
    char = "str"[:2];
    char = "str"[:];
    list[1]++;
    ++list[1];
    list[1] = 5.5;
    val = dict[0];
    val = dict["foo"];
    val = dict["bar"] = 2;
    val = dict[foo["no"]("hey")];
    1 + 2 = "foo"; // Technically illegal
}
)testCode"));

    ACCUMTEST(TestParse("Dot Expand Test", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    console.trace("whut");
    console.trace.warningLevel = 2;
    dict.foo = "bar";
    dict.foo();
}
)testCode"));
}

bool TestParse(const char* testName, scrpt::ParseErr expectedErr, bool dumpAst, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "Parser: " << testName << std::endl;

    scrpt::ParseErr err = scrpt::ParseErr::NoError;
    scrpt::Parser parser;
    scrpt::Lexer lexer(scrpt::Tests::DuplicateSource(source));
    try
    {
        parser.Consume(&lexer);
        if (dumpAst) parser.DumpAst();
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        err = ex.GetParseErr();
        parser.DumpAst();
    }

    bool passed = err == expectedErr;

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
