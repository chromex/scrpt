#pragma once

// Design
// * Callstack generation for runtime errors
// * Debug information

namespace scrpt
{
	enum class RuntimeErr
	{
		NoError,
		FailedFunctionLookup,
		UnsupportedOperandType,
		OperandMismatch,
        StackOverflow,
	};
	const char* RuntimeErrToString(RuntimeErr err);

    class VM
    {
    public:
        VM(const Bytecode* bytecode);

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

        StackVal* Execute(const char* funcName);

    private:
        const Bytecode* _bytecode;
        std::map<std::string, const FunctionData*> _functionMap;

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

        unsigned int _ip;
        std::vector<StackObj> _stack;
        StackObj* _stackPointer;
        StackObj* _framePointer;
        StackObj _returnValue;

        void Run();

        void PushStackFrame(unsigned int returnIp, int framePointerOffset);
        void PushNull(size_t num = 1);
        void PushId(StackType type, unsigned int id);
        void PushInt(StackType type, int val);
        void PushFloat(float val);
        void Copy(StackObj* src, StackObj* dest);
        void Pop(size_t num = 1);

        void ConditionalJump(int test, unsigned int dest);
    };
}