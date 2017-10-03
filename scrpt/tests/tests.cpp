#include "scrpt.h"
#include "tests.h"
#include <Windows.h>

#define COMPONENTNAME "Tests"

enum class Phase
{
    Lexer,
    Parser,
    BytecodeGen,
    VM,
};

static bool ExecuteTest(const char* testName, Phase phase, int resultValue, scrpt::Err resultErr, bool verbose, bool perfTest, const char* source);
static std::shared_ptr<const char> DuplicateSource(const char* source);

void scrpt::RunTests()
{
    unsigned int passed = 0;
    unsigned int failed = 0;
    const bool testPerf = false;

#define ACCUMTEST(Name, Phase, ResultVal, ResultErr, Verbose, PerfTest, Source) \
    if (!testPerf || PerfTest) \
    { \
        if (ExecuteTest(Name, Phase, ResultVal, ResultErr, Verbose, testPerf && PerfTest, Source)) \
            ++passed; \
        else \
            ++failed; \
    }

    // == Adding a Test case ==
    // Test cases, in general, should be fully executable VM tests unless they are expected to trigger an 
    // error during an earlier phase. While debugging issues you can use the Phase parameter to manually stop
    // a test early to enable targetted debugging rather than needing to add a phase-specific test.
    //
    // Note that verbose only affects the target phase. E.g. a BytecodeGen test that encounters a lexer error
    // will not print out the AST if verbose is set. This is because it is not an expected error and the test
    // or bug should be fixed. 
    //
    // Test Template:
    //
    //ACCUMTEST("", Phase::, 0, scrpt::Err::, false, false, R"testCode(
    //)testCode");

    // Lexer
    // 

    ACCUMTEST("Illegal Symbol", Phase::Lexer, 0, scrpt::Err::Lexer_UnknownSymbol, false, false, R"testCode(
func main() {
	a = true ? "yes" : "no";
}
)testCode");

    ACCUMTEST("Non Terminated String", Phase::Lexer, 0, scrpt::Err::Lexer_NonTerminatedString, false, false, R"testCode(
func main() {
	a = "aksjdf;
}
)testCode");

    ACCUMTEST("Illegal Number", Phase::Lexer, 0, scrpt::Err::Lexer_InvalidNumber, false, false, R"testCode(
func main() {
	a = 1.;
}
)testCode");

    ACCUMTEST("Unsupported Escape", Phase::Lexer, 0, scrpt::Err::Lexer_UnknownStringEscape, false, false, R"testCode(
func main() {
	a =	"asdf\0asdf";
}
)testCode");

    // Parser
    //

    // Bytecode Gen
    //

    // TODO: Missing var decl
    // TODO: Double decl

    // VM
    //

    // Inheritance
    // Properties
    // Static methods
    // Type testing (e.g. tc2 is TestClass)
    // Operator overloading
    // Native classes
    // Default values
    // Remove old : syntax
    // this.
    // ctor chaining

    ACCUMTEST("Full class", Phase::BytecodeGen, 0, scrpt::Err::NoError, true, false, R"testCode(
class TestClass
{
    TestClass()
    {
        a = 7;
        b = 8;
    }

    TestClass(p1, p2)
    {
        a = p1;
        b = p2;
    }

    func Sum()
    {
        return a + b;
    }

    var a;
    var b;
}

func main() {
    var tc1 = new TestClass();
    var tc2 = new TestClass(15, 9);
    var sum = tc1.Sum() + tc2.Sum();
    tc2.b = 0;
    return sum + tc2.Sum();
}
)testCode");

    ACCUMTEST("Complex Expansion", Phase::VM, 11, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    foo()[1][2].blah("hello world");
}

func foo() {
    return [0, [1, 2, {"blah": getVal}]];
}

func getVal(str) {
    return strlen(str);
}
)testCode");

    ACCUMTEST("Variable masking", Phase::VM, 30, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var v1 = 4 + 5;
    var v2;
    v2 = v1 + 7;
    if (v2 > 0)
    {
        var v1 = 3;
        v2 = v1 * 7;
    }
    return v2 + v1;
}
)testCode");

    ACCUMTEST("Loop counting", Phase::VM, 300000, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var sum = 0;
    var max = 100000;
    do
    {
        ++sum;
    } while(sum < max);

    while (sum < max * 2)
    {
        ++sum;
    }

    for (;sum < max * 3; ++sum) {}

    return sum;
}
)testCode");

    ACCUMTEST("Fibonacci Iterative", Phase::VM, 317811, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var v0 = 0;
    var v1 = 1;
    var max = 300000;
    while (v1 < max)
    {
        var t = v0 + v1;
        v0 = v1;
        v1 = t;
    }
    return v1;
}
)testCode");

    ACCUMTEST("Fibonacci Recursive", Phase::VM, 6765, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    return fib(20);
}

func fib(n) {
    if (n > 2) return fib(n - 1) + fib(n - 2);
    return 1;
}
)testCode");

    ACCUMTEST("Factorial", Phase::VM, 479001600, scrpt::Err::NoError, false, true, R"testCode(
func main() {
    return Fact(12);
}

func Fact(v) {
    if (v == 0)
        return 1;
    else
        return v * Fact(v - 1);
}
)testCode");

    ACCUMTEST("Left to Right for Equal Expression Precedence", Phase::VM, 10, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    return 2 * 3 - 1 * 2 + 5 + 1;
}
)testCode");

    ACCUMTEST("Parenthesis preservation", Phase::VM, -5, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    return 2 - (5 + 2);
}
)testCode");

    ACCUMTEST("FFI Stress", Phase::VM, 1234, scrpt::Err::NoError, false, true, R"testCode(
func main() {
    for (var i = 0; i < 10000; ++i)
        testextern(12, 34);

    return testextern(12, 34);
}
)testCode");

    ACCUMTEST("String concat", Phase::VM, 12, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var a = "hello world" # "!";
    return strlen(a);
}
)testCode");

    ACCUMTEST("Quick sort", Phase::VM, 1, scrpt::Err::NoError, false, true, R"testCode(
func main() {
	var numElements = 1000;
    var lst = [];
    for (var count = 0; count < numElements; ++count)
        lst #= randomInt();
    quickSort(lst, 0, length(lst) - 1);
	var correct = true;
	for (var count = 0; count < length(lst) - 1 && correct; ++count)
		if (lst[count] > lst[count+1]) correct = false;
    if (correct) return 1;
	return 0;
}

func quickSort(list, left, right) {
    var index = partition(list, left, right);
    if (left < index - 1)
        quickSort(list, left, index - 1);
    if (index < right)
        quickSort(list, index, right);
}

func partition(list, left, right) {
    var i = left;
    var j = right;
    var pivot = list[(left + right) / 2];

    while (i <= j) {
        while (list[i] < pivot)
            ++i;
        while (list[j] > pivot)
            --j;
        if (i <= j) {
            var tmp = list[i];
            list[i] = list[j];
            list[j] = tmp;
            ++i;
            --j;
        }
    }

    return i;
}
)testCode");

    ACCUMTEST("First class function", Phase::VM, 7, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var fun = Gen();
    var lst = [fun, 2];
	return lst[0](3, 4);
}

func TestFunc(one, two) {
    return one + two;
}

func Gen() {
    return TestFunc;
}
)testCode");

    ACCUMTEST("Assignments", Phase::VM, 35, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var val = 4;
    val += 7;
    val *= 2;
    val -= 2;
    val /= 4;
    var lst = [1];
    lst #= [];
    lst[1][0] = true;
    lst[0] += 4;
    lst[0] *= 3;
    lst[1] #= 5;
    lst[1][1] *= 7;
    return lst[1][1];
}
)testCode");

    ACCUMTEST("Simple class", Phase::VM, 29, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var v1 = MakeVec(3, 7);
    var v2 = MakeVec(7, 12);
    var obj = {"v1": v1};
    obj.v1:Add(v2);
    return v1.x + v1.y;
}

func MakeVec(x, y) {
    return {
        "x": x,
        "y": y,
        "Add": Vec_Add,
    };
}

func Vec_Add(this, other) {
    this.x += other.x;
    this.y += other.y;
}
)testCode");

    ACCUMTEST("Conway's Game of Life", Phase::VM, 29, scrpt::Err::NoError, false, false, R"testCode(
func main() {
    var game = MakeGame(15);
    var generations = 5;
    while (generations > 0) {
        //game:Draw();
        if (game:Update())
            --generations;
        else
            generations = 0;
    }

    //game:Draw();
    return 29;
}

func MakeGame(size) {
    var obj = {
        "data": [],
        "data2": [],
        "size": size,
        "Draw": Game_Draw,
        "Update": Game_Update,
        "GetIndex": Game_GetIndex,
        "SumNeighbors": Game_SumNeighbors,
        "IsAlive": Game_IsAlive,
    };

    var numElements = size * size;
    for (var index = 0; index < numElements; ++index) {
        if (randomInt() > 25000)
            obj.data #= true;
        else
            obj.data #= false;
    }

    return obj;
}

func Game_Draw(this) {
    print("State");
    for (var row = 0; row < this.size; ++row) {
        var str = "";
        for (var col = 0; col < this.size; ++col) {
            if (this.data[this:GetIndex(col, row)])
                str #= "# ";
            else
                str #= ". ";
        }
        print(str);
    }
    print("");
}

func Game_Update(this) {
    var any = false;
    for (var row = 0; row < this.size; ++row) {
        for (var col = 0; col < this.size; ++col) {
            var cellIndex = Game_GetIndex(this, col, row);
            var nNeighbors = Game_SumNeighbors(this, col, row);
            var output = false;
            if (this.data[cellIndex] && (nNeighbors == 2 || nNeighbors == 3)) {
                output = true;
            }
            else 
            {
                if (nNeighbors == 3) {
                    output = true;
                }
            }
            this.data2[cellIndex] = output;
            if (output) any = true;
        }
    }
    var tmp = this.data;
    this.data = this.data2;
    this.data2 = tmp;
    return any;
}

func Game_GetIndex(this, x, y) {
    return this.size * y + x;
}

func Game_SumNeighbors(this, x, y) {
    var sum = 0;
    if (Game_IsAlive(this, x-1, y-1)) ++sum;
    if (Game_IsAlive(this, x-1, y)) ++sum;
    if (Game_IsAlive(this, x-1, y+1)) ++sum;
    if (Game_IsAlive(this, x, y-1)) ++sum;
    if (Game_IsAlive(this, x, y+1)) ++sum;
    if (Game_IsAlive(this, x+1, y-1)) ++sum;
    if (Game_IsAlive(this, x+1, y)) ++sum;
    if (Game_IsAlive(this, x+1, y+1)) ++sum;
    return sum;
}

func Game_IsAlive(this, x, y) {
    if (x >= 0 && x < this.size && y >= 0 && y < this.size)
        return this.data[Game_GetIndex(this, x, y)];
    return false;
}
)testCode");

    std::cout << passed << " passed and " << failed << " failed" << std::endl;
}

std::shared_ptr<const char> DuplicateSource(const char* source)
{
    AssertNotNull(source);
    size_t len = strlen(source);
    char* copy(new char[len + 1]);
    strcpy_s(copy, len + 1, source);
    return std::shared_ptr<const char>(copy, std::default_delete<const char[]>());
}

LARGE_INTEGER GetTime()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time;
}

double ConvertTimeMS(LONGLONG time)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (time / (double)freq.QuadPart) * 1000.0;
}

void randomInt(scrpt::VM* vm)
{
    vm->SetExternResult(scrpt::StackType::Int, rand() % 100000);
}

void testextern(scrpt::VM* vm)
{
    int i = vm->GetParam<int>(scrpt::ParamId::_0);
    int i2 = vm->GetParam<int>(scrpt::ParamId::_1);
    vm->SetExternResult(scrpt::StackType::Int, i * 100 + i2);
}

bool ExecuteTest(const char* testName, Phase phase, int resultValue, scrpt::Err resultErr, bool verbose, bool perfTest, const char* source)
{
    const int nTimedRuns = 10;

    // Setup log line
    std::ostringstream os;
    switch (phase)
    {
    case Phase::Lexer: os << "L"; break;
    case Phase::Parser: os << "P"; break;
    case Phase::BytecodeGen: os << "B"; break;
    case Phase::VM: os << "V"; break;
    }
    os << "|" << testName << "> ";

    // Run test
    bool passed = false;
    switch (phase)
    {
    case Phase::Lexer:
        {
            scrpt::Lexer lexer(DuplicateSource(source));
            std::shared_ptr<scrpt::Token> token;
            scrpt::Err err = scrpt::Err::NoError;
            try
            {
                do
                {
                    lexer.Advance();
                    token = lexer.Current();
                    if (verbose)
                    {
                        os << token->SymToString() << std::endl;
                    }
                } while (token->GetSym() != scrpt::Symbol::End);
            }
            catch (scrpt::Exception& ex)
            {
                token = ex.GetToken();
                err = ex.GetErr();
                if (resultErr != err)
                    os << std::endl << ex.what() << std::endl;
            }

            passed = resultErr == err;
        }
        break;

    case Phase::Parser:
        {
            scrpt::Err err = scrpt::Err::NoError;
            scrpt::Parser parser;
            scrpt::Lexer lexer(DuplicateSource(source));
            try
            {
                parser.Consume(&lexer);
            }
            catch (scrpt::Exception& ex)
            {
                err = ex.GetErr();
                if (err != resultErr) os << ex.what() << std::endl;
            }

            // TODO: This should be added to the stringstream
            if (verbose) parser.DumpAst();

            passed = err == resultErr;
        }
        break;

    case Phase::BytecodeGen:
        {
            scrpt::Err err = scrpt::Err::NoError;
            scrpt::Lexer lexer(DuplicateSource(source));
            scrpt::Parser parser;
            scrpt::BytecodeGen compiler;
            try
            {
                parser.Consume(&lexer);
                compiler.Consume(*parser.GetAst());
            }
            catch (scrpt::Exception& ex)
            {
                err = ex.GetErr();
                if (err != resultErr) os << ex.what() << std::endl;
            }

            if (verbose) Decompile(compiler.GetBytecode(), os);

            passed = err == resultErr;
        }
        break;

    case Phase::VM:
        {
            scrpt::Err err = scrpt::Err::NoError;
            bool gotExpectedResult = true;
            double runtime = 0.0;
            try
            {
                scrpt::VM vm;
                scrpt::RegisterStdLib(vm);
                vm.AddExternFunc("testextern", 2, testextern);
                vm.AddExternFunc("randomInt", 0, randomInt);
                vm.AddSource(DuplicateSource(source));
                vm.Finalize();
                if (verbose) vm.Decompile(os);
                // Run the first, untimed test to validate test and ensure the code path is warm
                scrpt::StackVal* ret = vm.Execute("main");
                gotExpectedResult = ret != nullptr && ret->integer == resultValue;

                if (gotExpectedResult && nTimedRuns > 0 && perfTest)
                {
                    LARGE_INTEGER startTime = GetTime();
                    for (int i = 0; i < nTimedRuns; ++i)
                    {
                        vm.Execute("main");
                    }
                    LARGE_INTEGER endTime = GetTime();
                    runtime = ConvertTimeMS(endTime.QuadPart - startTime.QuadPart) / (double)nTimedRuns;
                    os << "[" << runtime << "] ";
                }
            }
            catch (scrpt::Exception& ex)
            {
                err = ex.GetErr();
                if (err != resultErr) os << ex.what() << std::endl;
            }

            passed = err == resultErr && gotExpectedResult;
        }
        break;
    }

    // Display Status
    if (passed)
    {
        os << "Passed" << std::endl;
    }
    else
    {
        os << "<<< Failed >>>" << std::endl;
    }

    std::cout << os.str();

    return passed;
}
