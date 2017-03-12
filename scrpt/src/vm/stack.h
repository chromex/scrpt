#pragma once

namespace scrpt
{
    enum class StackType : int
    {
        Top,
        Null,
        Boolean,
        Int,
        Float,
        StaticString,
        DynamicString,
        List,
        Map,
    };
    const char* StackTypeToString(StackType type);

    union StackObj;
    typedef std::vector<StackObj> Stack;
    typedef std::vector<StackObj> List;

    struct StackRef
    {
        unsigned int refCount;
        union
        {
            void* value;
            std::string* string;
            List* list;
        };
    };

    struct StackVal
    {
        StackType type;
        union
        {
            StackRef* ref;
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
        StackVal v;
        StackFrame frame;
    };
}
