#include "../scrpt.h"

#define COMPONENTNAME "stdlib"

//void ToString(scrpt::VM* vm, scrpt::StackVal* val, std::stringstream& ss)
//{
//    AssertNotNull(val);
//
//    switch (val->type)
//    {
//    case scrpt::StackType::Null: ss << "<NULL>"; break;
//    case scrpt::StackType::Boolean: ss << (val->integer == 0 ? "false" : "true"); break;
//    case scrpt::StackType::Int: ss << val->integer; break;
//    case scrpt::StackType::Float: ss << val->fp; break;
//    case scrpt::StackType::DynamicString:
//    case scrpt::StackType::StaticString:
//        ss << vm->GetParam<const char*>(scrpt::ParamId::_0);
//        break;
//    case scrpt::StackType::List:
//        {
//            scrpt::List* list = vm->GetParam<scrpt::List*>(scrpt::ParamId::_0);
//            ss << "[";
//            bool showComma = false;
//            for (auto item = list->begin(); item != list->end(); ++item)
//            {
//                if (showComma)
//                    ss << ", ";
//                ToString(vm, &item->v, ss);
//                showComma = true;
//            }
//            ss << "]";
//        }
//        break;
//
//    default:
//        AssertFail("Unsupported type for ToString");
//    }
//}
//
//void scrpt::RegisterStdLib(VM& vm)
//{
//    vm.AddExternFunc("print", 1, [](VM* vm) 
//    {
//        std::stringstream ss;
//        scrpt::StackVal* val = vm->GetParam<scrpt::StackVal*>(scrpt::ParamId::_0);
//        ss << ">>> ";
//        ToString(vm, val, ss);
//        std::cout << ss.str() << std::endl;
//        vm->PushNull();
//    });
//
//    vm.AddExternFunc("strlen", 1, [](VM* vm)
//    {
//        const char* str = vm->GetParam<const char*>(scrpt::ParamId::_0);
//        vm->PushInt(StackType::Int, (int)strlen(str));
//    });
//
//    vm.AddExternFunc("length", 1, [](VM* vm)
//    {
//        List* list = vm->GetParam<List*>(scrpt::ParamId::_0);
//        vm->PushInt(StackType::Int, (int)list->size());
//    });
//}
