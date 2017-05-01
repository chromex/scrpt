#include "scrpt.h"
#include "tests.h"
#include <windows.h>

#define COMPONENTNAME "Tests_VM"

static bool TestVM(const char* testName, int resultValue, bool, const char* source);

// TODO: Conways game of life

void scrpt::Tests::RunTestsVM(unsigned int* passed, unsigned int* failed)
{
    AssertNotNull(passed);
    AssertNotNull(failed);

    srand((unsigned int)time(NULL));

#define ACCUMTEST(T) T ? ++*passed : ++*failed
    ACCUMTEST(TestVM("Simple counting", 100000, false, R"testCode(
func main() {
    sum = 0;
    max = 100000;
    do
    {
        ++sum;
    } while(sum < max);

            return sum;
}
)testCode"));

    ACCUMTEST(TestVM("Fibonacci", 317811, false, R"testCode(
func main() {
    v0 = 0;
    v1 = 1;
    max = 300000;
    while (v1 < max)
    {
        t = v0 + v1;
        v0 = v1;
        v1 = t;
    }
    return v1;
}
)testCode"));

    ACCUMTEST(TestVM("Fibonacci 2", 6765, false, R"testCode(
func main() {
    return fib(20);
}

func fib(n) {
    if (n > 2) return fib(n - 1) + fib(n - 2);
    return 1;
}
)testCode"));

    ACCUMTEST(TestVM("Simple call", 1, false, R"testCode(
func main() {
    if (Test(1, 2, 3))
        return 1;
    
    return 0;
}

func Test(a, b, c) {
    sum = a + b + c;
    return sum == 6;
}
)testCode"));

    ACCUMTEST(TestVM("Factorial", 479001600, false, R"testCode(
func main() {
    return Fact(12);
}

func Fact(v) {
    if (v == 0)
        return 1;
    else
        return v * Fact(v - 1);
}
)testCode"));

    ACCUMTEST(TestVM("FFI", 1234, false, R"testCode(
func main() {
    for (i = 0; i < 10000; ++i)
        testextern(12, 34);

    return testextern(12, 34);
}
)testCode"));

    ACCUMTEST(TestVM("Strings", 1, false, R"testCode(
func main() {
    for (i = 0; i < 10000; ++i)
    {
        test("whut");
    }

    return 1;
}

func test(str) {
    return str;
}
)testCode"));

    ACCUMTEST(TestVM("Concat", 12, false, R"testCode(
func main() {
    a = "hello world" # "!";
    return strlen(a);
}
)testCode"));

    ACCUMTEST(TestVM("List Test", 5, false, R"testCode(
func main() {
    lst = [1, true, 5, ["yes"], [], "no"];
    return lst[2];
}
)testCode"));

    ACCUMTEST(TestVM("Quick sort", 1, false, R"testCode(
func main() {
	numElements = 1000;
    lst = [];
    for (count = 0; count < numElements; ++count)
        lst #= randomInt();
    quickSort(lst, 0, length(lst) - 1);
	correct = true;
	for (count = 0; count < length(lst) - 1 && correct; ++count)
		if (lst[count] > lst[count+1]) correct = false;
    if (correct) return 1;
	return 0;
}

func quickSort(list, left, right) {
    index = partition(list, left, right);
    if (left < index - 1)
        quickSort(list, left, index - 1);
    if (index < right)
        quickSort(list, index, right);
}

func partition(list, left, right) {
    i = left;
    j = right;
    pivot = list[(left + right) / 2];

            while (i <= j) {
        while (list[i] < pivot)
            ++i;
        while (list[j] > pivot)
            --j;
        if (i <= j) {
            tmp = list[i];
            list[i] = list[j];
            list[j] = tmp;
            ++i;
            --j;
        }
    }

    return i;
}
)testCode"));

    ACCUMTEST(TestVM("Basic Maps", 1, false, R"testCode(
func main() {
	map = {};
    staticmap = {"one": 2, "whoa": [true, []]};
    map["bar"] = "foo";
    map["baz"] = staticmap;
    staticmap["three"] = 1234;
    map["far"] = staticmap.one;
    return 1;
}
)testCode"));

    ACCUMTEST(TestVM("First class function", 7, false, R"testCode(
func main() {
    fun = Gen();
    lst = [fun, 2];
	return lst[0](3, 4);
}

func TestFunc(one, two) {
    return one + two;
}

func Gen() {
    return TestFunc;
}
)testCode"));

    // TODO: Update this to use += in Vec_Add
    ACCUMTEST(TestVM("Class calls", 29, false, R"testCode(
func main() {
    v1 = MakeVec(3, 7);
    v2 = MakeVec(7, 12);
    wrap = {"v1": v1};
    wrap.v1:Add(v2);
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
    this["x"] = this.x + other.x;
    this["y"] = this.y + other.y;
}
)testCode"));
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

static bool TestVM(const char* testName, int resultValue, bool decompile, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "V|" << testName << "> ";

    bool err = false;
    double runtime = 0.0;
    try
    {
		scrpt::VM vm;
        scrpt::RegisterStdLib(vm);
        vm.AddExternFunc("testextern", 2, testextern);
        vm.AddExternFunc("randomInt", 0, randomInt);
		vm.AddSource(scrpt::Tests::DuplicateSource(source));
		vm.Finalize();
        if (decompile) vm.Decompile();
        LARGE_INTEGER startTime = GetTime();
        scrpt::StackVal* ret = vm.Execute("main");
        LARGE_INTEGER endTime = GetTime();
        err = ret == nullptr || ret->integer != resultValue;
        runtime = ConvertTimeMS(endTime.QuadPart - startTime.QuadPart);
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        err = true;
    }

    if (!err)
    {
        std::cout << "Passed [" << runtime << "]" << std::endl;
    }
    else
    {
        std::cout << "<<< Failed >>>" << std::endl;
    }

    return !err;
}
