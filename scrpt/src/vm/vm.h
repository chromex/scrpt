#pragma once

// Design
// * Callstack generation for runtime errors
// * Debug information

//namespace scrpt
//{
//	enum class RuntimeErr
//	{
//		NoError,
//		FailedFunctionLookup,
//		UnsupportedOperandType,
//		OperandMismatch,
//        StackOverflow,
//        StackUnderflow,
//        UnexpectedParamType,
//        BadParamRequest,
//        NotImplemented,
//	};
//	const char* RuntimeErrToString(RuntimeErr err);
//
//    enum class ParamId : int
//    {
//        _0 = 0,
//        _1,
//        _2,
//        _3,
//        _4,
//    };
//
//    class VM
//    {
//    public:
//        VM();
//        ~VM();
//
//        void AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func);
//		void AddSource(std::shared_ptr<const char> source);
//		void Finalize();
//        void Decompile();
//        StackVal* Execute(const char* funcName);
//
//        void PushNull(size_t num = 1);
//        void PushInt(StackType type, int val);
//        void PushFloat(float val);
//        void PushString(const char* string);
//        template<typename T> T GetParam(ParamId id);
//        template<> int GetParam<int>(ParamId id);
//        template<> float GetParam<float>(ParamId id);
//        template<> bool GetParam<bool>(ParamId id);
//        template<> const char* GetParam<const char*>(ParamId id);
//        template<> List* GetParam<List*>(ParamId id);
//        template<> StackVal* GetParam<StackVal*>(ParamId id);
//
//    private:
//		std::unique_ptr<Parser> _parser;
//        std::unique_ptr<BytecodeGen> _compiler;
//        Bytecode _bytecode;
//        std::map<std::string, unsigned int> _functionMap;
//
//        unsigned int _ip;
//        std::vector<StackObj> _stack;
//		StackObj* _stackRoot;
//        StackObj* _stackPointer;
//        StackObj* _framePointer;
//        StackObj _returnValue;
//        int _currentExternArgN;
//
//        void Run();
//
//        inline void PushStackFrame(unsigned int returnIp, int framePointerOffset);
//        inline void PushString(unsigned int id);
//        inline void PushList(List* list);
//        inline void ThrowErr(RuntimeErr err) const;
//        StackObj* GetParamBase(ParamId id);
//
//        void ConditionalJump(int test, unsigned int dest);
//    };
//
//    template<>
//    inline int VM::GetParam(ParamId id)
//    {
//        StackObj* obj = this->GetParamBase(id);
//        if (obj->v.type != StackType::Int)
//        {
//            this->ThrowErr(RuntimeErr::UnexpectedParamType);
//        }
//
//        return obj->v.integer;
//    }
//
//    template<>
//    inline float VM::GetParam(ParamId id)
//    {
//        StackObj* obj = this->GetParamBase(id);
//        if (obj->v.type != StackType::Float)
//        {
//            this->ThrowErr(RuntimeErr::UnexpectedParamType);
//        }
//
//        return obj->v.fp;
//    }
//
//    template<>
//    inline bool VM::GetParam(ParamId id)
//    {
//        StackObj* obj = this->GetParamBase(id);
//        if (obj->v.type != StackType::Boolean)
//        {
//            this->ThrowErr(RuntimeErr::UnexpectedParamType);
//        }
//
//        return !(obj->v.integer == 0);
//    }
//
//    template<>
//    inline const char* VM::GetParam(ParamId id)
//    {
//        StackObj* obj = this->GetParamBase(id);
//        if (obj->v.type == StackType::DynamicString)
//        {
//            return obj->v.ref->string->c_str();
//        }
//        else if (obj->v.type == StackType::StaticString)
//        {
//            return _bytecode.strings[(unsigned int)obj->v.integer].c_str();
//        }
//
//        this->ThrowErr(RuntimeErr::UnexpectedParamType);
//        return nullptr;
//    }
//
//    template<>
//    inline List* VM::GetParam(ParamId id)
//    {
//        StackObj* obj = this->GetParamBase(id);
//        if (obj->v.type != StackType::List)
//        {
//            this->ThrowErr(RuntimeErr::UnexpectedParamType);
//        }
//
//        return obj->v.ref->list;
//    }
//
//    template<>
//    inline StackVal* VM::GetParam(ParamId id)
//    {
//        return &(this->GetParamBase(id)->v);
//    }
//}