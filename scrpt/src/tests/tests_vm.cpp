#include "../scrpt.h"
#include "tests.h"

#define COMPONENTNAME "Tests_VM"

static bool TestVM(const char* testName, int resultValue, const char* source);

// TODO: Runtime speed checks
// TODO: Bubble sort, quick sort
// TODO: Conways game of life

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

    ACCUMTEST(TestVM("Simple call", 1, R"testCode(
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

    ACCUMTEST(TestVM("Factorial", 479001600, R"testCode(
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

	ACCUMTEST(TestVM("FFI Test", 1234, R"testCode(
func main() {
    return print(12);
}
)testCode"));
}

static bool TestVM(const char* testName, int resultValue, const char* source)
{
    AssertNotNull(testName);
    AssertNotNull(source);

    std::cout << "VM: " << testName << std::endl;

    bool err = false;
    try
    {
		scrpt::VM vm;
		//scrpt::RegiserSTL(vm); -- NEW
		vm.AddSource(scrpt::Tests::DuplicateSource(source));
		vm.Finalize();
		scrpt::VM::StackVal* ret = vm.Execute("main");
		err = ret == nullptr || ret->integer != resultValue;
    }
    catch (scrpt::CompilerException& ex)
    {
        std::cout << ex.what() << std::endl;
        err = true;
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
