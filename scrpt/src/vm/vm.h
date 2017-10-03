#pragma once

// Design
// * Callstack generation for runtime errors
// * Debug information

namespace scrpt
{
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
        ~VM();

        void AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func);
		void AddSource(std::shared_ptr<const char> source);
		void Finalize();
        void Decompile(std::ostream& os);
        StackVal* Execute(const char* funcName);

        void SetExternResult(StackType type, int val);

        void PushNull(size_t num = 1);
        void LoadNull(char reg);
        void LoadInt(char reg, StackType type, int val);
        void LoadFloat(char reg, float val);
        void LoadString(char reg, const char* string);
        template<typename T> T GetParam(ParamId id);
        template<> int GetParam<int>(ParamId id);
        template<> float GetParam<float>(ParamId id);
        template<> bool GetParam<bool>(ParamId id);
        template<> const char* GetParam<const char*>(ParamId id);
        template<> List* GetParam<List*>(ParamId id);
        template<> StackVal* GetParam<StackVal*>(ParamId id);
        const FunctionData& GetFunction(unsigned int id) const;

    private:
		std::unique_ptr<Parser> _parser;
        std::unique_ptr<BytecodeGen> _compiler;
        Bytecode _bytecode;
        std::map<std::string, unsigned int> _functionMap;

        unsigned int _ip;
        std::vector<StackObj> _stack;
		StackObj* _stackRoot;
        StackObj* _stackPointer;
        StackObj* _framePointer;
        StackObj _returnValue;
        int _currentExternArgN;

        void Run();

        inline void PushStackFrame(unsigned int returnIp, int framePointerOffset);
        inline void LoadString(char reg, unsigned int id);
        inline void LoadList(char reg, List* list);
        inline void LoadMap(char reg, Map* map);
        inline void ThrowErr(Err err) const;
        const FunctionData& LookupFunction(unsigned int ip) const;
        void FormatCallstackFunction(unsigned int ip, std::ostream& os) const;
        std::string CreateCallstack(unsigned int startingIp);
        StackObj* GetParamBase(ParamId id);

        void ConditionalJump(StackObj* obj, int test, unsigned int dest);
    };

    template<>
    inline int VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::Int)
        {
            this->ThrowErr(Err::VM_UnexpectedParamType);
        }

        return obj->v.integer;
    }

    template<>
    inline float VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::Float)
        {
            this->ThrowErr(Err::VM_UnexpectedParamType);
        }

        return obj->v.fp;
    }

    template<>
    inline bool VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::Boolean)
        {
            this->ThrowErr(Err::VM_UnexpectedParamType);
        }

        return !(obj->v.integer == 0);
    }

    template<>
    inline const char* VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type == StackType::DynamicString)
        {
            return obj->v.ref->string->c_str();
        }
        else if (obj->v.type == StackType::StaticString)
        {
            return obj->v.staticString;
        }

        this->ThrowErr(Err::VM_UnexpectedParamType);
        return nullptr;
    }

    template<>
    inline List* VM::GetParam(ParamId id)
    {
        StackObj* obj = this->GetParamBase(id);
        if (obj->v.type != StackType::List)
        {
            this->ThrowErr(Err::VM_UnexpectedParamType);
        }

        return obj->v.ref->list;
    }

    template<>
    inline StackVal* VM::GetParam(ParamId id)
    {
        return &(this->GetParamBase(id)->v);
    }
}