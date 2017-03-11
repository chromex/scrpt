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

    struct StackVal
    {
        StackType type;
        union
        {
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

    typedef std::vector<StackObj> Stack;
}
