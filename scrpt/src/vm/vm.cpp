#include "../scrpt.h"

#define COMPONENTNAME "VM"
#define STACKSIZE 10000

namespace scrpt
{
    VM::VM()
        : _parser(new Parser())
        , _compiler(new BytecodeGen())
        , _ip(0)
        , _stack(STACKSIZE)
        , _stackPointer(nullptr)
        , _framePointer(nullptr)
        , _currentExternArgN(0)
    {
    }

    VM::~VM()
    {
        this->Deref(&_returnValue.v);
    }

    void VM::AddExternFunc(const char* name, unsigned char nParam, const std::function<void(VM*)>& func)
    {
        AssertNotNull(_compiler.get());
        AssertNotNull(name);

        _compiler.get()->AddExternFunc(name, nParam, func);
    }

    void VM::AddSource(std::shared_ptr<const char> source)
	{
		AssertNotNull(_parser.get());
		AssertNotNull(source);

		Lexer lexer(source);
		_parser.get()->Consume(&lexer);
	}

	void VM::Finalize()
	{
		AssertNotNull(_parser.get());

		_compiler.get()->Consume(*(_parser.get()->GetAst()));
		_bytecode = _compiler.get()->GetBytecode();
		_parser.reset(nullptr);
        _compiler.reset(nullptr);

		for (unsigned int id = 0; id < _bytecode.functions.size(); ++id)
		{
			_functionMap[_bytecode.functions[id].name] = id;
		}
	}

    void VM::Decompile()
    {
        scrpt::Decompile(_bytecode);
    }

	StackVal* VM::Execute(const char* funcName)
    {
        AssertNotNull(funcName);

		if (_parser.get() != nullptr)
		{
			this->Finalize();
		}

        // Clear out any previous return value
        this->Deref(&_returnValue.v);

        auto funcIter = _functionMap.find(funcName);
        if (funcIter != _functionMap.end())
        {
            const FunctionData& fd = _bytecode.functions[funcIter->second];
            size_t nLocals = fd.localLookup.size();

            _stackPointer = &_stack[0];
            _ip = fd.entry;
            // TODO: Push params
            this->PushStackFrame(0, 0);
            _framePointer = _stackPointer;
            this->PushNull(nLocals - fd.nParam);
            this->Run();
            // TODO: Pop params

            Assert(_stackPointer == &_stack[0], "Stack must be empty after executing");
            return &_returnValue.v;
        }
        else
        {
			throw CreateRuntimeEx(funcName, RuntimeErr::FailedFunctionLookup);
        }
	}

	#define INCREMENTOP(IntOp, FloatOp) \
    { \
        StackObj* obj = _framePointer + GetOperand(int); \
        StackType t = obj->v.type; \
        if (t == StackType::Int) \
            this->PushInt(StackType::Int, IntOp); \
        else if (t == StackType::Float) \
            this->PushFloat(FloatOp); \
        else \
			this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        _ip += 4; \
    } 

	#define MATHOP(Op) \
    { \
        StackObj* v1 = _stackPointer - 2; \
        StackObj* v2 = _stackPointer - 1; \
        StackType t1 = v1->v.type; \
        StackType t2 = v2->v.type; \
        this->Pop(2); \
        if (t1 != StackType::Int && t1 != StackType::Float) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        if (t2 != StackType::Int && t2 != StackType::Float) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        if (t1 == StackType::Int && t2 == StackType::Int) \
        { \
            this->PushInt(StackType::Int, v1->v.integer Op v2->v.integer); \
        } \
        else \
        { \
            float fv1 = t1 == StackType::Float ? v1->v.fp : (float)v1->v.integer; \
            float fv2 = t2 == StackType::Float ? v2->v.fp : (float)v2->v.integer; \
            this->PushFloat(fv1 Op fv2); \
        } \
    }

    #define ASSIGNMATHOP(Op) \
    {\
        StackObj* target = _framePointer + GetOperand(int);\
        StackObj* value = _stackPointer - 1;\
        StackType t1 = target->v.type;\
        StackType t2 = value->v.type;\
        this->Pop(1);\
        if (t1 != StackType::Int && t1 != StackType::Float) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        if (t2 != StackType::Int && t2 != StackType::Float) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        if (t1 == StackType::Float)\
        {\
            float result = target->v.fp Op t2 == StackType::Float ? value->v.fp : (float)value->v.integer;\
            this->PushFloat(result);\
        }\
        else\
        {\
            int result = target->v.integer Op t2 == StackType::Int ? value->v.integer : (int)value->v.fp;\
            this->PushInt(StackType::Int, result);\
        }\
        _ip += 4;\
    }

    #define COMPOP(Op)  \
    { \
        bool result; \
        StackObj* v1 = _stackPointer - 2; \
        StackObj* v2 = _stackPointer - 1; \
        StackType t1 = v1->v.type; \
        StackType t2 = v2->v.type; \
        if (t1 != StackType::Int && t1 != StackType::Float) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        if (t2 != StackType::Int && t2 != StackType::Float) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        if (t1 == StackType::Int && t2 == StackType::Int) \
        { \
            result = v1->v.integer Op v2->v.integer; \
        } \
        else \
        { \
            float fv1 = t1 == StackType::Float ? v1->v.fp : (float)v1->v.integer; \
            float fv2 = t2 == StackType::Float ? v2->v.fp : (float)v2->v.integer; \
            result = fv1 Op fv2; \
        } \
        this->Pop(2); \
        this->PushInt(StackType::Boolean, result); \
    }

    #define BOOLOP(Op) \
    { \
        StackObj* v1 = _stackPointer - 2; \
        StackObj* v2 = _stackPointer - 1; \
        StackType t1 = v1->v.type; \
        StackType t2 = v2->v.type; \
        if (t1 != StackType::Boolean && t2 != StackType::Boolean) this->ThrowErr(RuntimeErr::UnsupportedOperandType); \
        this->Pop(2); \
        this->PushInt(StackType::Boolean, v1->v.integer Op v2->v.integer); \
    }

    void VM::Run()
    {
        const unsigned char* data = &(_bytecode.data[0]);

        bool running = true;
        while (running)
        {
            #define GetOperand(Type) *((Type*)(data + _ip + 1))
            switch ((const OpCode)(data[_ip]))
            {
            case OpCode::Unknown: this->ThrowErr(RuntimeErr::UnsupportedOperandType); break;

            /// 
            /// Push Null
            ///
            case OpCode::PushNull: this->PushNull(); break;

            /// 
            /// Push True
            ///
            case OpCode::PushTrue: this->PushInt(StackType::Boolean, 1); break;

            /// 
            /// Push False
            ///
            case OpCode::PushFalse: this->PushInt(StackType::Boolean, 0); break;

            /// 
            /// Pop
            ///
            case OpCode::Pop: this->Pop(); break;

            /// 
            /// Equals
            ///
            case OpCode::Eq:
                {
                    bool result;
                    StackObj* v1 = _stackPointer - 2;
                    StackObj* v2 = _stackPointer - 1;
                    StackType t1 = v1->v.type;
                    StackType t2 = v2->v.type;

                    if (t1 == t2)
                    {
                        switch (t1)
                        {
                        case StackType::Boolean:
                        case StackType::Int:
                            result = v1->v.integer == v2->v.integer;
                            break;
                        case StackType::Float:
                            result = v1->v.fp == v2->v.fp;
                            break;
                        default:
                            this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                            break;
                        }
                    }
                    else
                    {
                        this->ThrowErr(RuntimeErr::OperandMismatch);
                    }

                    this->Pop(2);
                    this->PushInt(StackType::Boolean, result);
                }
                break;

            /// 
            /// Or
            ///
            case OpCode::Or:
                BOOLOP(||);
                break;

            /// 
            /// And
            ///
            case OpCode::And:
                BOOLOP(&&);
                break;

            /// 
            /// Add
            ///
            case OpCode::Add:
                MATHOP(+);
                break;

            /// 
            /// Subtract
            ///
            case OpCode::Sub:
                MATHOP(-);
                break;

            /// 
            /// Multiply
            ///
            case OpCode::Mul:
                MATHOP(*);
                break;

            /// 
            /// Divide
            ///
            case OpCode::Div:
                MATHOP(/);
                break;

            /// 
            /// Modulo
            ///
            case OpCode::Mod: this->ThrowErr(RuntimeErr::UnsupportedOperandType); break;

            ///
            /// Concat
            ///
            case OpCode::Concat:
            {
                StackObj* v1 = _stackPointer - 2; 
                StackObj* v2 = _stackPointer - 1; 
                StackType t1 = v1->v.type; 
                StackType t2 = v2->v.type; 
                if (t1 != StackType::DynamicString) this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                std::stringstream ss(*(std::string*)v1->v.ref->value, std::ios_base::ate | std::ios_base::out);
                switch (t2)
                {
                case StackType::Boolean:
                    ss << (v2->v.integer == 0 ? "false" : "true");
                    break;
                case StackType::DynamicString:
                    ss << *(std::string*)v2->v.ref->value;
                    break;
                case StackType::Float:
                    ss << v2->v.fp;
                    break;
                case StackType::Int:
                    ss << v2->v.integer;
                    break;
                case StackType::List:
                    this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                    break;
                case StackType::Map:
                    this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                    break;
                case StackType::Null:
                    ss << "null";
                    break;
                case StackType::StaticString:
                    this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                    break;
                default:
                    ThrowErr(RuntimeErr::NotImplemented);
                }
                this->Pop(2);
                this->PushString(ss.str().c_str());
            }
            break;

            /// 
            /// Less Than
            ///
            case OpCode::LT: 
                COMPOP(<);
                break;

            /// 
            /// Greater Than
            ///
            case OpCode::GT:
                COMPOP(>);
                break;

            ///  
            /// Less Than or Equal
            ///
            case OpCode::LTE:
                COMPOP(<=);
                break;

            /// 
            /// Greater Than or Equal
            ///
            case OpCode::GTE:
                COMPOP(>=);
                break;

            /// 
            /// Return
            ///
            case OpCode::Ret:
                {
                    this->Copy(_stackPointer - 1, &_returnValue);
                    this->Pop(); // return value
                    while (_stackPointer > _framePointer)
                    {
                        this->Pop();
                    }
                    StackObj* stackFrame = _stackPointer - 1;
                    if (stackFrame->frame.framePointerOffset != 0)
                    {
                        _ip = stackFrame->frame.returnIp - 1;
                        _framePointer -= stackFrame->frame.framePointerOffset;
                    }
                    else
                    {
                        running = false;
                    }
                    this->Pop(); // Stack frame
                }
                break;

            /// 
            /// Restore Return Value
            ///
            case OpCode::RestoreRet:
                this->PushNull();
                this->Copy(&_returnValue, _stackPointer - 1);
                this->Deref(&_returnValue.v);
                _returnValue.v.type = StackType::Null;
                _returnValue.v.ref = nullptr;
                break;

            /// 
            /// Push Integer
            ///
            case OpCode::PushInt: 
                this->PushInt(StackType::Int, GetOperand(int)); 
                _ip += 4;
                break;

            /// 
            /// Push Float
            ///
            case OpCode::PushFloat: 
                this->PushFloat(GetOperand(float)); 
                _ip += 4;
                break;

            /// 
            /// Push String
            ///
            case OpCode::PushString:
                this->PushString(_bytecode.strings[GetOperand(unsigned int)].c_str());
                _ip += 4;
                break;

            /// 
            /// Push Ident Value
            ///
            case OpCode::PushIdent:
                {
                    StackObj* obj = _stackPointer;
                    this->PushNull();
                    this->Copy(_framePointer + GetOperand(int), obj);
                }
                _ip += 4;
                break;

            /// 
            /// Call Function
            ///
            case OpCode::Call:
                {
                    unsigned int funcId = GetOperand(unsigned int);
                    const FunctionData& fd = _bytecode.functions[funcId];
                    if (!fd.external)
                    {
                        // Stack size limits guarentee this will fit in a 32bit int even on 64bit builds
                        int framePointerOffset = (int)(_stackPointer - _framePointer + 1);
                        this->PushStackFrame(_ip + 5, framePointerOffset);
                        _framePointer = _stackPointer;
                        this->PushNull(fd.localLookup.size() - fd.nParam);
                        _ip = fd.entry - 1;
                    }
                    else
                    {
                        // TODO: Re-entrant externals
                        StackObj* startingStack = _stackPointer;
                        _currentExternArgN = fd.nParam;

                        fd.func(this);
                        // TODO: Support for no return value
                        // TODO: Error on more than one return value
                        this->Copy(_stackPointer - 1, &_returnValue);
                        this->Pop(); 
                        _ip += 4;
                    }
                }
                break;

            /// 
            /// Identifier Assign
            ///
            case OpCode::AssignI: 
                this->Copy(_stackPointer - 1, _framePointer + GetOperand(int));
                _ip += 4;
                break;

            /// 
            /// Identifier Add Assign
            ///
            case OpCode::PlusEqI:
                ASSIGNMATHOP(+=);
                break;

            /// 
            /// Identifier Subtract Assign
            ///
            case OpCode::MinusEqI:
                ASSIGNMATHOP(-=);
                break;

            /// 
            /// Identifier Multiply Assign
            ///
            case OpCode::MultEqI:
                ASSIGNMATHOP(*=);
                break;

            /// 
            /// Identifier Divide Assign
            ///
            case OpCode::DivEqI:
                ASSIGNMATHOP(/=);
                break;

            /// 
            /// Identifier Modulo Assign
            ///
            case OpCode::ModuloEqI: this->ThrowErr(RuntimeErr::UnsupportedOperandType); break;

            /// 
            /// Identifier Concatenate Assign
            ///
            case OpCode::ConcatEqI:
                {
                    this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                }
                break;

            /// 
            /// Prefix Increment Identifier
            ///
            case OpCode::IncI:
                INCREMENTOP(++obj->v.integer, ++obj->v.fp);
                break;

            /// 
            /// Prefix Decrement Identifier
            ///
            case OpCode::DecI:
                INCREMENTOP(--obj->v.integer, --obj->v.fp);
                break;

            /// 
            /// Postfix Increment Identifier
            ///
            case OpCode::PostIncI:
                INCREMENTOP(obj->v.integer++, obj->v.fp++);
                break;

            /// 
            /// Postfix Decrement Identifier
            ///
            case OpCode::PostDecI:
                INCREMENTOP(obj->v.integer--, obj->v.fp--);
                break;

            /// 
            /// Branch True
            ///
            case OpCode::BrT:
                this->ConditionalJump(1, GetOperand(unsigned int));
                break;

            /// 
            /// Branch False
            ///
            case OpCode::BrF:
                this->ConditionalJump(0, GetOperand(unsigned int));
                break;

            /// 
            /// Jump
            ///
            case OpCode::Jmp:
                _ip = GetOperand(unsigned int) - 1;
                break;

            default:
                this->ThrowErr(RuntimeErr::UnsupportedOperandType);
                break;
            }

            ++_ip;
        }
    }

    #define CHECKSTACK if (_stackPointer - &_stack[0] >= STACKSIZE) this->ThrowErr(RuntimeErr::StackOverflow);

    void VM::PushStackFrame(unsigned int returnIp, int framePointerOffset)
    {
        CHECKSTACK
        _stackPointer->frame.returnIp = returnIp;
        _stackPointer->frame.framePointerOffset = framePointerOffset;
        ++_stackPointer;
    }

    void VM::PushNull(size_t num /* = 1 */)
    {
        CHECKSTACK
        Assert(num <= 256, "Mass null push over limit");
        while (num-- > 0)
        {
            _stackPointer->v.type = StackType::Null;
            ++_stackPointer;
        }
    }

    void VM::PushId(StackType type, unsigned int id)
    {
        CHECKSTACK
        _stackPointer->v.type = type;
        _stackPointer->v.id = id;
        ++_stackPointer;
    }

    void VM::PushInt(StackType type, int val)
    {
        CHECKSTACK
        _stackPointer->v.type = type;
        _stackPointer->v.integer = val;
        ++_stackPointer;
    }

    void VM::PushFloat(float val)
    {
        CHECKSTACK
        _stackPointer->v.type = StackType::Float;
        _stackPointer->v.fp = val;
        ++_stackPointer;
    }

    void VM::PushString(const char* string)
    {
        AssertNotNull(string);
        
        CHECKSTACK
        _stackPointer->v.type = StackType::DynamicString;
        _stackPointer->v.ref = new StackRef{ 1, new std::string(string) };
        ++_stackPointer;
    }

    StackObj* VM::GetParamBase(ParamId id)
    {
        int pOffset = (int)id;
        if (pOffset >= _currentExternArgN)
        {
            this->ThrowErr(RuntimeErr::BadParamRequest);
        }

        int offset = -_currentExternArgN + (int)id;
        return _stackPointer + offset;
    }

    void VM::Copy(StackObj* src, StackObj* dest)
    {
        AssertNotNull(src);
        AssertNotNull(dest);

        dest->v.type = src->v.type;
        dest->v.ref = src->v.ref;
        if (dest->v.type == StackType::DynamicString)
        {
            ++dest->v.ref->refCount;
        }
    }

    void VM::Pop(size_t num /* = 1 */)
    {
        while (num-- > 0)
        {
            if (_stackPointer == &_stack[0])
            {
                this->ThrowErr(RuntimeErr::StackUnderflow);
            }

            _stackPointer -= 1;
            this->Deref(&_stackPointer->v);
            _stackPointer->v.type = StackType::Top;
        }
    }

    inline void VM::Deref(StackVal* stackVal)
    {
        AssertNotNull(stackVal);
        if (stackVal->type == StackType::DynamicString)
        {
            stackVal->ref->refCount -= 1;
            if (stackVal->ref->refCount == 0)
            {
                delete (std::string*)stackVal->ref->value;
                stackVal->ref->value = nullptr;
                delete stackVal->ref;
                stackVal->ref = nullptr;
            }
        }
    }

    void VM::ThrowErr(RuntimeErr err)
    {
        // TODO: Create stack trace
        throw CreateRuntimeEx("", err);
    }

    void VM::ConditionalJump(int test, unsigned int dest)
    {
        StackObj *obj = _stackPointer - 1;
        if (obj->v.type != StackType::Boolean) this->ThrowErr(RuntimeErr::UnsupportedOperandType);
        if (obj->v.integer == test)
        {
            _ip = dest - 1;
        }
        else
        {
            _ip += 4;
        }
        this->Pop();
    }

	const char* StackTypeToString(StackType type)
	{
		switch (type)
		{
			ENUM_CASE_TO_STRING(StackType::Top);
			ENUM_CASE_TO_STRING(StackType::Null);
			ENUM_CASE_TO_STRING(StackType::Boolean);
			ENUM_CASE_TO_STRING(StackType::Int);
			ENUM_CASE_TO_STRING(StackType::Float);
			ENUM_CASE_TO_STRING(StackType::StaticString);
			ENUM_CASE_TO_STRING(StackType::DynamicString);
			ENUM_CASE_TO_STRING(StackType::List);
			ENUM_CASE_TO_STRING(StackType::Map);

		default: 
			AssertFail("Mising case for StackType");
		}

		return nullptr;
	}

	const char* RuntimeErrToString(RuntimeErr err)
	{
		switch (err)
		{
			ENUM_CASE_TO_STRING(RuntimeErr::NoError);
			ENUM_CASE_TO_STRING(RuntimeErr::FailedFunctionLookup);
			ENUM_CASE_TO_STRING(RuntimeErr::UnsupportedOperandType);
			ENUM_CASE_TO_STRING(RuntimeErr::OperandMismatch);
            ENUM_CASE_TO_STRING(RuntimeErr::StackOverflow);
            ENUM_CASE_TO_STRING(RuntimeErr::StackUnderflow);
            ENUM_CASE_TO_STRING(RuntimeErr::UnexpectedParamType);
            ENUM_CASE_TO_STRING(RuntimeErr::BadParamRequest);
            ENUM_CASE_TO_STRING(RuntimeErr::NotImplemented);

		default:
			AssertFail("Missing case for RuntimeErr");
		}

		return nullptr;
	}
}
