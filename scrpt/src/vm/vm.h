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
        StackUnderflow,
        UnexpectedParamType,
        BadParamRequest,
	};
	const char* RuntimeErrToString(RuntimeErr err);

    enum class ParamId : int
    {
        _0 = 0,
        _1,
        _2,
        _3,
        _4,
    };

    class VM
    {
    public:
        VM();

        void AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func);
		void AddSource(std::shared_ptr<const char> source);
		void Finalize();
        void Decompile();
        StackVal* Execute(const char* funcName);

        void PushNull(size_t num = 1);
        void PushInt(StackType type, int val);
        void PushFloat(float val);
        template<typename T> T GetParam(ParamId id);
        template<> int GetParam<int>(ParamId id);
        template<> float GetParam<float>(ParamId id);
        template<> bool GetParam<bool>(ParamId id);

    private:
		std::unique_ptr<Parser> _parser;
        std::unique_ptr<BytecodeGen> _compiler;
        Bytecode _bytecode;
        std::map<std::string, unsigned int> _functionMap;

        unsigned int _ip;
        std::vector<StackObj> _stack;
        StackObj* _stackPointer;
        StackObj* _framePointer;
        StackObj _returnValue;
        int _currentExternArgN;

        void Run();

        void PushStackFrame(unsigned int returnIp, int framePointerOffset);
        void PushId(StackType type, unsigned int id);
        void Copy(StackObj* src, StackObj* dest);
        void Pop(size_t num = 1);
        void ThrowErr(RuntimeErr err);
        inline StackObj* GetParamBase(ParamId id);

        void ConditionalJump(int test, unsigned int dest);
    };

    template<>
    inline int VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::Int)
        {
            this->ThrowErr(RuntimeErr::UnexpectedParamType);
        }

        return obj->v.integer;
    }

    template<>
    inline float VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::Float)
        {
            this->ThrowErr(RuntimeErr::UnexpectedParamType);
        }

        return obj->v.fp;
    }

    template<>
    inline bool VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::Boolean)
        {
            this->ThrowErr(RuntimeErr::UnexpectedParamType);
        }

        return !(obj->v.integer == 0);
    }
}