#include "../scrpt.h"

#define COMPONENTNAME "stdlib"

void ToString(scrpt::VM* vm, scrpt::StackVal* val, std::ostream& os)
{
    AssertNotNull(val);

    switch (val->type)
    {
    case scrpt::StackType::Null: os << "<NULL>"; break;
    case scrpt::StackType::Boolean: os << (val->integer == 0 ? "false" : "true"); break;
    case scrpt::StackType::Int: os << val->integer; break;
    case scrpt::StackType::Float: os << val->fp; break;
    case scrpt::StackType::Func:
        {
            auto& fd = vm->GetFunction(val->id);
            os << "<" << fd.name << "/" << (int)fd.nParam << ">";
        }
        break;
    case scrpt::StackType::DynamicString: os << *val->ref->string; break;
    case scrpt::StackType::StaticString: os << val->staticString; break;
    case scrpt::StackType::List:
        {
            scrpt::List* list = val->ref->list;
            os << "[";
            bool showComma = false;
            for (auto item = list->begin(); item != list->end(); ++item)
            {
                if (showComma)
                    os << ", ";
                ToString(vm, &item->v, os);
                showComma = true;
            }
            os << "]";
        }
        break;
    case scrpt::StackType::Map:
        {
            scrpt::Map* map = val->ref->map;
            os << "{";
            bool showComma = false;
            for (auto& entry : *map)
            {
                if (showComma)
                    os << ", ";
                os << entry.first << ": ";
                ToString(vm, &entry.second.v, os);
                showComma = true;
            }
            os << "}";
        }
        break;

    default:
        AssertFail("Unsupported type for ToString");
    }
}

void scrpt::RegisterStdLib(VM& vm)
{
    vm.AddExternFunc("print", 1, [](VM* vm)
    {
        std::ostringstream os;
        scrpt::StackVal* val = vm->GetParam<scrpt::StackVal*>(scrpt::ParamId::_0);
        os << ">>> ";
        ToString(vm, val, os);
        std::cout << os.str() << std::endl;
        vm->SetExternResult(StackType::Null, 0);
    });

    vm.AddExternFunc("strlen", 1, [](VM* vm)
    {
        const char* str = vm->GetParam<const char*>(scrpt::ParamId::_0);
        vm->SetExternResult(StackType::Int, (int)strlen(str));
    });

    vm.AddExternFunc("length", 1, [](VM* vm)
    {
        List* list = vm->GetParam<List*>(scrpt::ParamId::_0);
        vm->SetExternResult(StackType::Int, (int)list->size());
    });
}
