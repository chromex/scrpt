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
    num = 1234;
    num = -4321;
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

    ACCUMTEST(TestParse("Control Flow", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    while (true) {}
    while (true) 1 + 2;
    do {} while(true);
    do 1 + 2; while(true);
    for (a = 0; a < 10; ++a) {}
    for (a = 0; a < 10; ++a) 1 + 2;
    for (;;) {}
    if (true) {}
    if (false) {} elif (true) {};
    if (false) {} elif (true) {} else {};
    if (false) {} else {};
    if (false) 1 + 2;
    return;
    return foo;
    continue;
    break;
    switch (foo) {}
    switch (foo) 
    {
    default: break;
    }
    switch (foo) 
    {
    case 0: break;
    }
    switch (foo) 
    {
    case "what":
    case 0: break;
    case 1: break;
    }
    switch (foo)
    {  
    case 0: { 1+2; ++bar; } break;
    }
}
)testCode"));

    ACCUMTEST(TestParse("Indexing", scrpt::ParseErr::NoError, false, R"testCode(
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

    ACCUMTEST(TestParse("Dot Expand", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    console.trace("whut");
    console.trace.warningLevel = 2;
    dict.foo = "bar";
    dict.foo();
}
)testCode"));

    ACCUMTEST(TestParse("Call Invocation", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    foo();
    foo("test");
    foo("test", 2);
    foo(3, {"dict": true});
    foo(3)(2);
    foo[1](123)[1];
    foo((bar()));
    1+2("whut"); // Technically illegal
}
)testCode"));

    ACCUMTEST(TestParse("Func Decl", scrpt::ParseErr::NoError, false, R"testCode(
func main() {}
func foo(var) {}
func foo(var, var2) {}
func foo(var, var2, var3) {}
func foo() { return; }
func foo() { return 1 + 2; }
)testCode"));

    ACCUMTEST(TestParse("Assignment", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    val = 1;
    val += 2;
    val -= 1;
    val *= 3;
    val /= 4;
    val %= 2;
    val = bar *= 3;
    dict["whoa"] = true;
    dict.whoa = true;
    dict.whoa *= 4;
}
)testCode"));

    ACCUMTEST(TestParse("Concat", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    val = "hello" # "world" # "again";
    val #= " one more";
}
)testCode"));

    ACCUMTEST(TestParse("Complex Expansion", scrpt::ParseErr::NoError, false, R"testCode(
func main() {
    foo()[1][2].blah("yo");
}
)testCode"));
}

bool TestParse(const char* testName, scrpt::ParseErr expectedErr, bool dumpAst, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "P|" << testName << "> ";

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
        std::cout << "Passed" << std::endl;
    }
    else
    {
        std::cout << "<<< Failed >>>" << std::endl;
    }

    return passed;
}
