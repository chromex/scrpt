#pragma once

namespace scrpt
{
    enum class StackType : int
    {
        Null,
        Boolean,
        Int,
        Float,
        Func,
        StaticString,
        DynamicString,
        List,
        Map,
    };
    const char* StackTypeToString(StackType type);

    union StackObj;
    typedef std::vector<StackObj> Stack;
    typedef std::vector<StackObj> List; 
    typedef std::unordered_map<std::string, StackObj> Map;

    struct StackRef
    {
        unsigned int refCount;
        union
        {
            void* value;
            std::string* string;
            List* list;
            Map* map;
        };
    };

    struct StackVal
    {
        StackVal() : type(StackType::Null), ref(nullptr) {}
        StackVal(StackType t, StackRef* r) : type(t), ref(r) {}

        StackType type;
        union
        {
            StackRef* ref;
            const char* staticString;
            unsigned int id;
            int integer;
            float fp;
        };
    };

    struct StackFrame
    {
        unsigned int returnIp;
        int framePointerOffset;
    };

    union StackObj
    {
        StackObj() : v(StackType::Null, nullptr) {}
        StackObj(StackType t, StackRef* r) : v(t, r) {}

        StackVal v;
        StackFrame frame;
    };
}
