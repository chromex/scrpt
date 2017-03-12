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

    struct StackRef
    {
        unsigned int refCount;
        void* value;
    };

    // TODO: Consider using a StackRef table to avoid the 64bit ptr
    struct StackVal
    {
        StackType type;
        union
        {
            unsigned int id;
            int integer;
            float fp;
            StackRef* ref;
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
