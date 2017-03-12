#include "../scrpt.h"

#define COMPONENTNAME "stdlib"

void scrpt::RegisterStdLib(VM& vm)
{
    vm.AddExternFunc("print", 1, [](VM* vm) 
    {
        scrpt::StackVal* val = vm->GetParam<scrpt::StackVal*>(scrpt::ParamId::_0);
        std::cout << ">>> ";
        switch (val->type)
        {
        case scrpt::StackType::Null: std::cout << "<NULL>" << std::endl; break;
        case scrpt::StackType::Boolean: std::cout << (val->integer == 0 ? "false" : "true") << std::endl; break;
        case scrpt::StackType::Int: std::cout << val->integer << std::endl; break;
        case scrpt::StackType::Float: std::cout << val->fp << std::endl; break;
        case scrpt::StackType::DynamicString: std::cout << vm->GetParam<const char*>(scrpt::ParamId::_0) << std::endl; break;

        default:
            AssertFail("Unsupported type for print");
            std::cout << std::endl;
        }
        vm->PushNull();
    });

    vm.AddExternFunc("strlen", 1, [](VM* vm)
    {
        const char* str = vm->GetParam<const char*>(scrpt::ParamId::_0);
        vm->PushInt(StackType::Int, (int)strlen(str));
    });
}
