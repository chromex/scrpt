#include "scrpt.h"
#include "tests.h"

int main(int argc, char** argv)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    scrpt::RunTests();
    system("pause");
}