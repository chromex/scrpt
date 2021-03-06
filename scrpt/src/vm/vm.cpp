#include "../scrpt.h"

#define COMPONENTNAME "VM"
#define STACKSIZE 10000

using namespace scrpt;

__forceinline bool IsRefCounted(StackType t)
{
	return (t == StackType::DynamicString || t == StackType::List || t == StackType::Map);
}

__forceinline void Deref(StackVal* val)
{
	AssertNotNull(val);
	StackType type = val->type;
	if (IsRefCounted(type))
	{
		StackRef* ref = val->ref;
		ref->refCount -= 1;
		if (ref->refCount == 0)
		{
			switch (type)
			{
			case StackType::DynamicString: 
                delete ref->string; 
                break;
			case StackType::List: 
                for (auto& entry : *ref->list) Deref(&entry.v);
                delete ref->list; 
                break;
            case StackType::Map:
                for (auto& entry : *ref->map) Deref(&entry.second.v);
                delete ref->map;
                break;
            default:
                AssertFail("Unhandled ref type");
			}
			delete ref;
			val->ref = nullptr;
			val->type = StackType::Null;
		}
	}
}

// Does not dereference the destination location
__forceinline void BlindCopy(StackObj* src, StackObj* dest)
{
	AssertNotNull(src);
	AssertNotNull(dest);

	StackVal& destVal = dest->v;
	StackVal& srcVal = src->v;
	destVal.type = srcVal.type;
	destVal.ref = srcVal.ref;
	if (IsRefCounted(destVal.type))
	{
		++destVal.ref->refCount;
	}
}

// Does dereference the destination location
__forceinline void Copy(StackObj* src, StackObj* dest)
{
	AssertNotNull(src);
	AssertNotNull(dest);

	StackVal& destVal = dest->v;
	StackVal& srcVal = src->v;
	Deref(&destVal);
	destVal.type = srcVal.type;
	destVal.ref = srcVal.ref;
	if (IsRefCounted(destVal.type))
	{
		++destVal.ref->refCount;
	}
}

__forceinline void BlindMove(StackObj* src, StackObj* dest)
{
    AssertNotNull(src);
    AssertNotNull(dest);

    dest->v.type = src->v.type;
    dest->v.ref = src->v.ref;
    src->v.type = StackType::Null;
    src->v.ref = nullptr;
}

__forceinline void Move(StackObj* src, StackObj* dest)
{
	AssertNotNull(src);
	AssertNotNull(dest);

	Deref(&dest->v);
	dest->v.type = src->v.type;
	dest->v.ref = src->v.ref;
	src->v.type = StackType::Null;
	src->v.ref = nullptr;
}

#define POP1 \
{ \
	if (_stackPointer == _stackRoot) \
	{ \
		this->ThrowErr(Err::VM_StackUnderflow); \
	} \
	_stackPointer -= 1; \
	Deref(&_stackPointer->v); \
	_stackPointer->v.type = StackType::Null; \
	_stackPointer->v.ref = nullptr; \
}

#define POP2 POP1 POP1

#define POPN(N) \
{ \
	int num = (N); \
	while (num-- > 0) \
	{ \
		POP1 \
	} \
}

#define REG0 *((char*)(data + _ip + 1))
#define REG1 *((char*)(data + _ip + 2))
#define REG2 *((char*)(data + _ip + 3))

#define CHECKSTACK if (_stackPointer - &_stack[0] >= STACKSIZE) this->ThrowErr(Err::VM_StackOverflow);

namespace scrpt
{
    VM::VM()
        : _parser(new Parser())
        , _compiler(new BytecodeGen())
        , _ip(0)
        , _stack(STACKSIZE)
		, _stackRoot(nullptr)
        , _stackPointer(nullptr)
        , _framePointer(nullptr)
        , _currentExternArgN(0)
    {
		_stackRoot = &_stack[0];
    }

    VM::~VM()
    {
		Deref(&_returnValue.v);
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
		Deref(&_returnValue.v);

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
            this->PushNull(fd.nLocalRegisters);
            try
            {
                this->Run();
                // TODO: Pop params
            }
            catch (Exception& cex)
            {
                throw CreateEx(this->CreateCallstack(_ip), cex.GetErr());
            }

            Assert(_stackPointer == &_stack[0], "Stack must be empty after executing");
            return &_returnValue.v;
        }
        else
        {
			throw CreateEx(funcName, Err::VM_FailedFunctionLookup);
        }
    }

    void VM::SetExternResult(StackType type, int val)
    {
        Deref(&_returnValue.v);
        _returnValue.v.type = type;
        _returnValue.v.integer = val;
    }

    #define INCREMENTOP(IntOp, FloatOp) \
    { \
        StackObj* obj = _framePointer + REG0; \
        StackType t = obj->v.type; \
        if (t == StackType::Int) \
            this->LoadInt(REG0, StackType::Int, IntOp); \
        else if (t == StackType::Float) \
            this->LoadFloat(REG0, FloatOp); \
        else \
			this->ThrowErr(Err::VM_UnsupportedOperandType); \
        ++_ip; \
    } 

    #define MATHOP(Op) \
    { \
        StackObj* v1 = _framePointer + REG0; \
        StackObj* v2 = _framePointer + REG1; \
        StackType t1 = v1->v.type; \
        StackType t2 = v2->v.type; \
        if (t1 != StackType::Int && t1 != StackType::Float) this->ThrowErr(Err::VM_UnsupportedOperandType); \
        if (t2 != StackType::Int && t2 != StackType::Float) this->ThrowErr(Err::VM_UnsupportedOperandType); \
        if (t1 == StackType::Int && t2 == StackType::Int) \
        { \
			int result = v1->v.integer Op v2->v.integer; \
            this->LoadInt(REG2, StackType::Int, result); \
        } \
        else \
        { \
            float fv1 = t1 == StackType::Float ? v1->v.fp : (float)v1->v.integer; \
            float fv2 = t2 == StackType::Float ? v2->v.fp : (float)v2->v.integer; \
            this->LoadFloat(REG2, fv1 Op fv2); \
        } \
        _ip += 3;\
    }

    #define ASSIGNMATHOP(Op) \
    {\
        StackObj* target = _framePointer + REG0;\
        StackObj* value = _framePointer + REG1;\
        StackType t1 = target->v.type;\
        StackType t2 = value->v.type;\
        if (t1 != StackType::Int && t1 != StackType::Float) this->ThrowErr(Err::VM_UnsupportedOperandType); \
        if (t2 != StackType::Int && t2 != StackType::Float) this->ThrowErr(Err::VM_UnsupportedOperandType); \
        if (t1 == StackType::Float)\
        {\
            target->v.fp Op (t2 == StackType::Float ? value->v.fp : (float)value->v.integer);\
        }\
        else\
        {\
            target->v.integer Op (t2 == StackType::Int ? value->v.integer : (int)value->v.fp);\
        }\
        _ip += 2;\
    }

    #define COMPOP(Op)  \
    { \
        bool result; \
        StackObj* v1 = _framePointer + REG0; \
        StackObj* v2 = _framePointer + REG1; \
        StackType t1 = v1->v.type; \
        StackType t2 = v2->v.type; \
        if (t1 != StackType::Int && t1 != StackType::Float) this->ThrowErr(Err::VM_UnsupportedOperandType); \
        if (t2 != StackType::Int && t2 != StackType::Float) this->ThrowErr(Err::VM_UnsupportedOperandType); \
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
        this->LoadInt(REG2, StackType::Boolean, result); \
        _ip += 3;\
    }

    #define BOOLOP(Op) \
    { \
        StackObj* v1 = _framePointer + REG0; \
        StackObj* v2 = _framePointer + REG1; \
        StackType t1 = v1->v.type; \
        StackType t2 = v2->v.type; \
        if (t1 != StackType::Boolean && t2 != StackType::Boolean) this->ThrowErr(Err::VM_UnsupportedOperandType); \
		int result = v1->v.integer Op v2->v.integer; \
        this->LoadInt(REG2, StackType::Boolean, result); \
        _ip += 3;\
    }

    void VM::Run()
    {
        const unsigned char* data = &(_bytecode.data[0]);

        bool running = true;
        while (running)
        {
            #define GetOperand(Type) *((Type*)(data + _ip + 2))
            switch ((const OpCode)(data[_ip]))
            {
            case OpCode::Unknown: this->ThrowErr(Err::VM_NotImplemented); break;

            /// 
            /// Load Null
            ///
            case OpCode::LoadNull: this->LoadNull(REG0); ++_ip; break;

            /// 
            /// Load True
            ///
            case OpCode::LoadTrue: this->LoadInt(REG0, StackType::Boolean, 1); ++_ip; break;

            /// 
            /// Load False
            ///
            case OpCode::LoadFalse: this->LoadInt(REG0, StackType::Boolean, 0); ++_ip; break;

            /// 
            /// Load Integer
            ///
            case OpCode::LoadInt:
                this->LoadInt(REG0, StackType::Int, GetOperand(int));
                _ip += 5;
                break;

            /// 
            /// Load Float
            ///
            case OpCode::LoadFloat:
                this->LoadFloat(REG0, GetOperand(float));
                _ip += 5;
                break;

            /// 
            /// Load String
            ///
            case OpCode::LoadString:
                this->LoadString(REG0, GetOperand(unsigned int));
                _ip += 5;
                break;

            ///
            /// Load Function
            ///
            case OpCode::LoadFunc:
                this->LoadInt(REG0, StackType::Func, GetOperand(unsigned int));
                _ip += 5;
                break;

            /// 
            /// Identifier Assign
            ///
            case OpCode::Store:
                Copy(_framePointer + REG1, _framePointer + REG0);
                _ip += 2;
                break;

            ///
            /// Indexed Identifier Assignment
            ///
            case OpCode::StoreIdx:
            {
                StackObj* target = _framePointer + REG0;
                StackObj* indexObj = _framePointer + REG1;
                StackObj* value = _framePointer + REG2;
                StackType targetType = target->v.type;
                StackType indexType = indexObj->v.type;
                if (targetType == StackType::List)
                {
                    if (indexType != StackType::Int) this->ThrowErr(Err::VM_UnsupportedOperandType);
                    int index = indexObj->v.integer;

                    List* list = target->v.ref->list;
                    if (index < 0)
                    {
                        this->ThrowErr(Err::VM_NotImplemented);
                    }

                    if (index >= list->size())
                    {
                        list->resize(index + 1, StackObj());
                    }

                    StackObj* targetVal = &list->at(index);
                    Copy(value, targetVal);
                }
                else if (targetType == StackType::Map)
                {
                    Map* map = target->v.ref->map;
                    if (indexType == StackType::StaticString)
                    {
                        Copy(value, &(*map)[indexObj->v.staticString]);
                    }
                    else if (indexType == StackType::DynamicString)
                    {
                        Copy(value, &(*map)[*indexObj->v.ref->string]);
                    }
                    else
                    {
                        this->ThrowErr(Err::VM_UnsupportedOperandType);
                    }
                }
                else
                {
                    this->ThrowErr(Err::VM_UnsupportedOperandType);
                }

                _ip += 3;
            }
            break;

            /// 
            /// Equals
            ///
            case OpCode::Eq:
            {
                bool result;
                StackObj* v1 = (_framePointer + REG0);
                StackObj* v2 = (_framePointer + REG1);
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
                        this->ThrowErr(Err::VM_UnsupportedOperandType);
                        break;
                    }
                }
                else
                {
                    this->ThrowErr(Err::VM_OperandMismatch);
                }

                this->LoadInt(REG2, StackType::Boolean, result);
                _ip += 3;
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
            case OpCode::Mod: this->ThrowErr(Err::VM_NotImplemented); break;

            ///
            /// Concat
            ///
            case OpCode::Concat:
            {
                StackObj* v1 = _framePointer + REG0; 
                StackObj* v2 = _framePointer + REG1;
                StackType t1 = v1->v.type; 
                StackType t2 = v2->v.type; 
                if (t1 == StackType::DynamicString || t1 == StackType::StaticString)
                {
                    std::stringstream ss(
                        t1 == StackType::StaticString ? v1->v.staticString : v1->v.ref->string->c_str(),
                        std::ios_base::ate | std::ios_base::out);
                    switch (t2)
                    {
                    case StackType::Boolean:
                        ss << (v2->v.integer == 0 ? "false" : "true");
                        break;
                    case StackType::DynamicString:
                        ss << *v2->v.ref->string;
                        break;
                    case StackType::Float:
                        ss << v2->v.fp;
                        break;
                    case StackType::Int:
                        ss << v2->v.integer;
                        break;
                    case StackType::List:
                        this->ThrowErr(Err::VM_NotImplemented);
                        break;
                    case StackType::Map:
                        this->ThrowErr(Err::VM_NotImplemented);
                        break;
                    case StackType::Null:
                        ss << "null";
                        break;
                    case StackType::StaticString:
                        ss << v2->v.staticString;
                        break;
                    default:
                        ThrowErr(Err::VM_NotImplemented);
                    }
                    this->LoadString(REG2, ss.str().c_str());
                }
                else if (t1 == StackType::List)
                {
                    // TODO: This doesn't properly concat lists, rather it would append the list as a unit
                    StackObj* target = _framePointer + REG2;
                    StackObj obj(StackType::Null, nullptr);
                    Copy(v2, &obj);
                    v1->v.ref->list->push_back(obj);
                    Copy(v1, target);
                }
                else
                {
                    this->ThrowErr(Err::VM_UnsupportedOperandType);
                }
                _ip += 3;
            }
            break;

            /// 
            /// Prefix Increment Identifier
            ///
            case OpCode::Inc:
                INCREMENTOP(++obj->v.integer, ++obj->v.fp);
                break;

            /// 
            /// Prefix Decrement Identifier
            ///
            case OpCode::Dec:
                INCREMENTOP(--obj->v.integer, --obj->v.fp);
                break;

            /// 
            /// Postfix Increment Identifier
            ///
            case OpCode::PostInc:
                INCREMENTOP(obj->v.integer++, obj->v.fp++);
                break;

            /// 
            /// Postfix Decrement Identifier
            ///
            case OpCode::PostDec:
                INCREMENTOP(obj->v.integer--, obj->v.fp--);
                break;

            ///
            /// Prefix negation
            ///
            case OpCode::Neg:
                {
                    StackObj* obj = _framePointer + REG0;
                    StackType t = obj->v.type;
                    if (t == StackType::Int)
                        this->LoadInt(REG1, StackType::Int, -obj->v.integer);
                    else if (t == StackType::Float)
                        this->LoadFloat(REG1, -obj->v.fp);
                    else
                        this->ThrowErr(Err::VM_UnsupportedOperandType);
                    _ip += 2;
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
            /// Call Function
            ///
            case OpCode::Call:
                {
                    StackObj* handle = _framePointer + REG0;
                    if (handle->v.type != StackType::Func) this->ThrowErr(Err::VM_UnsupportedOperandType);
                    const FunctionData& fd = _bytecode.functions[handle->v.id];
                    if (fd.nParam != REG1) this->ThrowErr(Err::VM_IncorrectArity);
                    if (!fd.external)
                    {
                        // Stack size limits guarentee this will fit in a 32bit int even on 64bit builds
                        int framePointerOffset = (int)(_stackPointer - _framePointer + 1);
                        this->PushStackFrame(_ip + 3, framePointerOffset);
                        _framePointer = _stackPointer;
                        this->PushNull(fd.nLocalRegisters);
                        _ip = fd.entry - 1;
                    }
                    else
                    {
                        // TODO: Re-entrant externals
                        StackObj* startingStack = _stackPointer;
                        _currentExternArgN = fd.nParam;

                        fd.func(this);
                        // TODO: Support for no return value
                        _ip += 2;
                    }
                }
                break;

            /// 
            /// Return
            ///
            case OpCode::Ret:
                {
                    Copy(_framePointer + REG0, &_returnValue);
                    while (_stackPointer > _framePointer)
                    {
                        POP1;
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
                    POP1; // Stack frame
                }
                break;

            /// 
            /// Restore Return Value
            ///
            case OpCode::RestoreRet:
                Move(&_returnValue, _framePointer + REG0);
                ++_ip;
                break;

            /// 
            /// Branch True
            ///
            case OpCode::BrT:
                this->ConditionalJump(_framePointer + REG0, 1, GetOperand(unsigned int));
                break;

            /// 
            /// Branch False
            ///
            case OpCode::BrF:
                this->ConditionalJump(_framePointer + REG0, 0, GetOperand(unsigned int));
                break;

            /// 
            /// Jump
            ///
            case OpCode::Jmp:
                _ip = *((unsigned int*)(data + _ip + 1)) - 1;
                break;

            ///
            /// Index An Object
            ///
            case OpCode::Index:
                {
                    StackObj* index = _framePointer + REG1;
                    StackObj* object = _framePointer + REG0;
                    StackType indexType = index->v.type;
                    StackType objectType = object->v.type;
                    if (objectType == StackType::List)
                    {
                        if (indexType != StackType::Int) this->ThrowErr(Err::VM_UnsupportedOperandType);
                        List* list = object->v.ref->list;
                        int idx = index->v.integer;
                        if (idx < 0)
                        {
                            this->ThrowErr(Err::VM_NotImplemented);
                        }

                        if (idx < list->size())
                        {
                            Copy(&list->at(idx), _framePointer + REG2);
                        }
                        else
                        {
                            this->LoadNull(REG2);
                        }
                    }
                    else if (objectType == StackType::Map)
                    {
                        if (indexType != StackType::StaticString && indexType != StackType::DynamicString) this->ThrowErr(Err::VM_UnsupportedOperandType);
                        Map* map = object->v.ref->map;
                        if (indexType == StackType::StaticString)
                        {
                            auto entry = map->find(index->v.staticString);
                            if (entry != map->end())
                                Copy(&entry->second, _framePointer + REG2);
                            else
                                LoadNull(REG2);
                        }
                        else if (indexType == StackType::DynamicString)
                        {
                            auto entry = map->find(*index->v.ref->string);
                            if (entry != map->end())
                                Copy(&entry->second, _framePointer + REG2);
                            else
                                LoadNull(REG2);
                        }
                    }
                    else
                    {
                        this->ThrowErr(Err::VM_UnsupportedOperandType);
                    }

                    _ip += 3;
                }
                break;

            ///
            /// Make list
            ///
            case OpCode::MakeList:
                {
                    // TODO: Max list size must be less than intmax and stack size
                    unsigned int size = GetOperand(unsigned int);
                    List* list = new List(size);
                    for (int index = -(int)size; index < 0; ++index)
                    {
                        BlindMove(_stackPointer + index, &(*list)[index + size]);
                    }
                    POPN(size);
                    this->LoadList(REG0, list);
                }
                _ip += 5;
                break;

            ///
            /// Make map
            ///
            case OpCode::MakeMap:
                {
                    unsigned int size = GetOperand(unsigned int);
                    Map* map = new Map();
                    for (int index = -(int)size; index < 0; index += 2)
                    {
                        StackObj* keyObj = _stackPointer + index;
                        StackObj* valueObj = _stackPointer + index + 1;

                        if (keyObj->v.type != StackType::StaticString && keyObj->v.type != StackType::DynamicString) this->ThrowErr(Err::VM_UnsupportedOperandType);
                        const char* key = keyObj->v.type == StackType::StaticString ? keyObj->v.staticString : keyObj->v.ref->string->c_str();
                        BlindMove(valueObj, &(*map)[key]);
                    }
                    POPN(size);
                    this->LoadMap(REG0, map);
                }
                _ip += 5;
                break;

            ///
            /// Push Register
            ///
            case OpCode::Push:
                CHECKSTACK
                BlindCopy(_framePointer + REG0, _stackPointer);
                ++_stackPointer;
                ++_ip;
                break;

            ///
            /// Pop Num
            ///
            case OpCode::PopN:
                POPN(REG0);
                ++_ip;
                break;

            default:
                this->ThrowErr(Err::VM_NotImplemented);
                break;
            }

            ++_ip;
        }
    }

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

    void VM::LoadNull(char reg)
    {
        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = StackType::Null;
    }

    void VM::LoadInt(char reg, StackType type, int val)
    {
        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = type;
        v.integer = val;
    }

    void VM::LoadFloat(char reg, float val)
    {
        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = StackType::Float;
        v.fp = val;
    }

    void VM::LoadString(char reg, const char* string)
    {
        AssertNotNull(string);

        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = StackType::DynamicString;
        v.ref = new StackRef{ 1, new std::string(string) };
    }

    const FunctionData& VM::GetFunction(unsigned int id) const
    {
        return _bytecode.functions[id];
    }

    void VM::LoadString(char reg, unsigned int id)
    {
        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = StackType::StaticString;
        v.staticString = _bytecode.strings[id].c_str();
    }

    void VM::LoadList(char reg, List* list)
    {
        AssertNotNull(list);

        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = StackType::List;
        v.ref = new StackRef{ 1, list };
    }

    inline void VM::LoadMap(char reg, Map* map)
    {
        AssertNotNull(map);

        StackVal& v = (_framePointer + reg)->v;
        Deref(&v);
        v.type = StackType::Map;
        v.ref = new StackRef{ 1, map };
    }

    StackObj* VM::GetParamBase(ParamId id)
    {
        int pOffset = (int)id;
        if (pOffset >= _currentExternArgN)
        {
            this->ThrowErr(Err::VM_BadParamRequest);
        }

        int offset = -_currentExternArgN + (int)id;
        return _stackPointer + offset;
    }

    void VM::ThrowErr(Err err) const
    {
        throw CreateEx("", err);
    }

    const FunctionData& VM::LookupFunction(unsigned int ip) const
    {
        Assert(_bytecode.functions.size() > 0, "Cannot perform lookup with no functions");

        for (size_t idx = 0; idx < _bytecode.functions.size() - 1; ++idx)
        {
            if (_bytecode.functions[idx].entry <= ip && _bytecode.functions[idx + 1].entry > ip)
            {
                return _bytecode.functions[idx];
            }
        }

        return _bytecode.functions.back();
    }

    void VM::FormatCallstackFunction(unsigned int ip, std::stringstream& ss) const
    {
        const FunctionData& fd = this->LookupFunction(ip);
        ss << "> " << fd.name << "(";
        for (int idx = 0; idx < fd.nParam; ++idx)
        {
            auto entry = fd.localLookup.find(-1 - fd.nParam + idx);
            Assert(entry != fd.localLookup.end(), "Failed to find param in local lookup");
            ss << entry->second;

            if (idx + 1 < fd.nParam)
            {
                ss << ", ";
            }
        }
        ss << ")" << std::endl;
    }

    std::string VM::CreateCallstack(unsigned int startingIp)
    {
        std::stringstream ss;
        ss << std::endl;
        this->FormatCallstackFunction(startingIp, ss);

        while (_stackPointer > _stackRoot)
        {
            if (_stackPointer == _framePointer)
            {
                StackObj* stackFrame = _stackPointer - 1;
                if (stackFrame->frame.framePointerOffset != 0)
                {
                    this->FormatCallstackFunction(stackFrame->frame.returnIp, ss);
                    _framePointer -= stackFrame->frame.framePointerOffset;
                }
                else
                {
                    break;
                }
            }

            POP1
        }

        return ss.str();
    }

    void VM::ConditionalJump(StackObj* obj, int test, unsigned int dest)
    {
        if (obj->v.type != StackType::Boolean) this->ThrowErr(Err::VM_UnsupportedOperandType);
        if (obj->v.integer == test)
        {
            _ip = dest - 1;
        }
        else
        {
            _ip += 5;
        }
    }

	const char* StackTypeToString(StackType type)
	{
		switch (type)
		{
			ENUM_CASE_TO_STRING(StackType::Null);
			ENUM_CASE_TO_STRING(StackType::Boolean);
			ENUM_CASE_TO_STRING(StackType::Int);
            ENUM_CASE_TO_STRING(StackType::Float);
            ENUM_CASE_TO_STRING(StackType::Func);
			ENUM_CASE_TO_STRING(StackType::StaticString);
			ENUM_CASE_TO_STRING(StackType::DynamicString);
			ENUM_CASE_TO_STRING(StackType::List);
			ENUM_CASE_TO_STRING(StackType::Map);

		default: 
			AssertFail("Mising case for StackType");
		}

		return nullptr;
	}
}
