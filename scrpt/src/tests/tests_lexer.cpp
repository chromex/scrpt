#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_Lexer"

static bool TestLexFile(const char* source, const char* testName, scrpt::LexErr expectedErr);

static const char* validSyntax = R"testCode(
func main(arg1, arg2)
{
	// Values
	strings = {"", "simple", "new\nline", "\"quote\" of doom", "\ttab\t\tmore"};
	numbers = {0, 1, 0123, 0.1234, 19203.1, -1.23, -0};
	_bo0leans = {true, false};

    // Basic expressions
	val = numbers[2];
	slice = numbers[1:];
	a = !b;
	t = a != b;
	t = a < b;
	t = a > b;
	t = a <= b;
	t = a >= b;

    // Math
	v = 1 + 3.2 - 2.2 * 3.0 / 0.0 % 2;
	v += 3;
	v -= val;
	v *= 5;
	v /= 2;
	v %= 2;
	++a;
	a++;
	--a;
	a--;

    // Concat
    str = "hello " # "world";
    lst = [];
    lst #= 4;

    // Control flow
	test(1, 0, "foo");
	if (true == false && true || false)
	{
		print("truth");
	}
	elif (true) { print("maybe"); }
	else { print("no"); }
	switch(arg1)
	{
	case "yes": print("y"); break;
	default: print("n");
	}
	while(false)
	{
	}
	for(i = 0; i < 10; i += 1)
	{
	}
	do
	{
	} while(false);
}

func test(a, b, c)
{
	return true;
}
)testCode";

static const char* illegalSymbol = R"testCode(
func main() {
	a = true ? "yes" : "no";
}
)testCode";

static const char* nonTerminatedString = R"testCode(
func main() {
	a = "aksjdf;
}
)testCode";

static const char* illegalNumber1 = R"testCode(
func main() {
	a = 1.;
}
)testCode";

static const char* unsupportedEscape = R"testCode(
func main() {
	a =	"asdf\0asdf";
}
)testCode";

void scrpt::Tests::RunTestsLexer(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

#define ACCUMTEST(T) T ? ++*passed : ++*failed
    ACCUMTEST(TestLexFile(validSyntax, "All Valid Syntax", scrpt::LexErr::NoError));
    ACCUMTEST(TestLexFile(illegalSymbol, "Illegal Symbol Test", scrpt::LexErr::UnknownSymbol));
    ACCUMTEST(TestLexFile(nonTerminatedString, "Non Terminated String", scrpt::LexErr::NonTerminatedString));
    ACCUMTEST(TestLexFile(illegalNumber1, "Missing digits after decimal", scrpt::LexErr::InvalidNumber));
    ACCUMTEST(TestLexFile(unsupportedEscape, "Unknown escape", scrpt::LexErr::UnknownStringEscape));
}

bool TestLexFile(const char* source, const char* testName, scrpt::LexErr expectedErr)
{
    std::cout << "Lexer: " << testName << std::endl;

    scrpt::Lexer lexer(scrpt::Tests::DuplicateSource(source));
    std::shared_ptr<scrpt::Token> token;
    scrpt::LexErr err = scrpt::LexErr::NoError;
    try
    {
        do
        {
            lexer.Advance();
            token = lexer.Current();
        } while (token->GetSym() != scrpt::Symbol::End && token->GetSym() != scrpt::Symbol::Error);
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        token = ex.GetToken();
        err = ex.GetLexErr();
    }

    bool passed = false;
    switch (token->GetSym())
    {
    case scrpt::Symbol::Error:
        passed = expectedErr == err;
        break;

    case scrpt::Symbol::End:
        passed = expectedErr == scrpt::LexErr::NoError;
        break;
    }

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