#include "tests.h"
#include "../scrpt.h"

#define COMPONENTNAME "Tests"

static std::unique_ptr<char[]> DuplicateSource(const char* source);
static bool TestLexFile(const char* source, const char* testName, scrpt::LexErr expectedErr);

static const char* validSyntax = R"testCode(
func main(arg1, arg2)
{
	// Values
	strings = {"", "simple", "new\nline", "\"quote\" of doom", "\ttab\t\tmore"};
	numbers = {0, 1, 0123, 0.1234, 19203.1};
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

	// Control flow
	test(1, 0, "foo");
	if (true == false && true || false)
	{
		print("truth");
	}
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

static const char* parserTest = R"testCode(
func main(p1, _p2,) {
}
)testCode";

void scrpt::RunTests()
{
	int passed = 0;
	int failed = 0;

	#define ACCUMTEST(T) T ? ++passed : ++failed
	ACCUMTEST(TestLexFile(validSyntax, "All Valid Syntax", scrpt::LexErr::NoError));
	ACCUMTEST(TestLexFile(illegalSymbol, "Illegal Symbol Test", scrpt::LexErr::UnknownSymbol));
	ACCUMTEST(TestLexFile(nonTerminatedString, "Non Terminated String", scrpt::LexErr::NonTerminatedString));
	ACCUMTEST(TestLexFile(illegalNumber1, "Missing digits after decimal", scrpt::LexErr::InvalidNumber));
	ACCUMTEST(TestLexFile(unsupportedEscape, "Unknown escape", scrpt::LexErr::UnknownStringEscape));

	std::cout << passed << " passed and " << failed << " failed" << std::endl;

	Parser parser;
	Lexer lexer(DuplicateSource(parserTest));
	parser.Consume(&lexer);
}

bool TestLexFile(const char* source, const char* testName, scrpt::LexErr expectedErr)
{
	std::cout << "Lexer: " << testName << std::endl;

	scrpt::Lexer lexer(DuplicateSource(source));
	scrpt::Symbol token;
	do
	{
		lexer.Advance();
		token = lexer.Current();
	} while (token != scrpt::Symbol::End && token != scrpt::Symbol::Error);

	bool passed = false;
	switch (token)
	{
		case scrpt::Symbol::Error:
			std::cout << lexer.GetErrorString() << std::endl;
			passed = expectedErr == lexer.GetError();
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

std::unique_ptr<char[]> DuplicateSource(const char* source)
{
	AssertNotNull(source);
	size_t len = strlen(source);
	std::unique_ptr<char[]> copy(new char[len + 1]);
	strcpy_s(copy.get(), len+1, source);
	return copy;
}
