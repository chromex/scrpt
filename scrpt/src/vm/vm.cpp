#include "../scrpt.h"

#define COMPONENTNAME "VM"
#define STACKSIZE 10000

namespace scrpt
{
    VM::VM(const Bytecode* bytecode)
        : _bytecode(bytecode)
        , _ip(0)
        , _stack(STACKSIZE)
    {
        AssertNotNull(_bytecode);

        for (auto& fd : _bytecode->functions)
        {
            _functionMap[fd.name] = &fd;
        }
    }

    VM::StackVal* VM::Execute(const char* funcName)
    {
        AssertNotNull(funcName);

        auto funcIter = _functionMap.find(funcName);
        if (funcIter != _functionMap.end())
        {
            size_t nLocals = funcIter->second->localLookup.size();

            _stackPointer = &_stack[0];
            _ip = funcIter->second->entry;
            // TODO: Push params
            this->PushStackFrame(0, 0);
            _framePointer = _stackPointer;
            this->PushNull(nLocals);
            this->Run();
            // TODO: Pop params

            //Assert(_stackPointer == &_stack[0], "Stack must be empty after executing");
            return &_returnValue.v;
        }
        else
        {
            AssertFail("Real error");
            // TODO
            return nullptr;
        }
    }

    void VM::Run()
    {
        const unsigned char* data = &(_bytecode->data[0]);

        bool running = true;
        while (running)
        {
            #define GetOperand(Type) *((Type*)(data + _ip + 1))
            switch ((const OpCode)(data[_ip]))
            {
            case OpCode::PushNull: this->PushNull(); break;
            case OpCode::PushTrue: this->PushInt(StackType::Boolean, 1); break;
            case OpCode::PushFalse: this->PushInt(StackType::Boolean, 0); break;
            case OpCode::Pop: this->Pop(); break;
            //case OpCode::Eq:
            //case OpCode::Or:
            //case OpCode::And:
            case OpCode::Add:
                {
                    StackObj* v1 = _stackPointer - 2;
                    StackObj* v2 = _stackPointer - 1;
                    StackType t1 = v1->v.type;
                    StackType t2 = v2->v.type;
                    if (t1 != StackType::Int && t1 != StackType::Float) AssertFail("Runtime error");
                    if (t2 != StackType::Int && t2 != StackType::Float) AssertFail("Runtime error");
                    if (t1 == StackType::Int && t2 == StackType::Int)
                    {
                        this->Pop(2);
                        this->PushInt(StackType::Int, v1->v.integer + v2->v.integer);
                    }
                    else
                    {
                        float fv1 = t1 == StackType::Float ? v1->v.fp : (float)v1->v.integer;
                        float fv2 = t2 == StackType::Float ? v2->v.fp : (float)v2->v.integer;
                        this->PushFloat(fv1 + fv2);
                    }
                }
                break;
            //case OpCode::Sub:
            //case OpCode::Mul:
            //case OpCode::Div:
            //case OpCode::Mod:
            case OpCode::LT: 
                {
                    bool result;
                    StackObj* v1 = _stackPointer - 2;
                    StackObj* v2 = _stackPointer - 1;
                    StackType t1 = v1->v.type;
                    StackType t2 = v2->v.type;
                    if (t1 != StackType::Int && t1 != StackType::Float) AssertFail("Runtime error");
                    if (t2 != StackType::Int && t2 != StackType::Float) AssertFail("Runtime error");
                    if (t1 == StackType::Int && t2 == StackType::Int)
                    {
                        result = v1->v.integer < v2->v.integer;
                    }
                    else
                    {
                        float fv1 = t1 == StackType::Float ? v1->v.fp : (float)v1->v.integer;
                        float fv2 = t2 == StackType::Float ? v2->v.fp : (float)v2->v.integer;
                        result = fv1 < fv2;
                    }
                    this->Pop(2);
                    this->PushInt(StackType::Boolean, result);
                }
                break;
            //case OpCode::GT:
            //case OpCode::LTE:
            //case OpCode::GTE:
            case OpCode::Ret:
                this->Copy(_stackPointer - 1, &_returnValue);
                this->Pop(); // return value
                while (_stackPointer > _framePointer)
                {
                    this->Pop();
                }
                running = false;
                // TODO: Restore return ip and frame pointer 
                // TODO: Pop stack frame
                break;
            //case OpCode::RestoreRet:

            case OpCode::PushInt: 
                this->PushInt(StackType::Int, GetOperand(int)); 
                _ip += 4;
                break;
            case OpCode::PushFloat: 
                this->PushFloat(GetOperand(float)); 
                _ip += 4;
                break;
            //case OpCode::PushString:
            case OpCode::PushIdent:
                {
                    StackObj* obj = _stackPointer;
                    this->PushNull();
                    this->Copy(_framePointer + GetOperand(int), obj);
                }
                _ip += 4;
                break;
            //case OpCode::Call:
            case OpCode::AssignI: 
                this->Copy(_stackPointer - 1, _framePointer + GetOperand(int));
                _ip += 4;
                break;
            //case OpCode::PlusEqI:
            //case OpCode::MinusEqI:
            //case OpCode::MultEqI:
            //case OpCode::DivEqI:
            //case OpCode::ModuloEqI:
            case OpCode::IncI:
                {
                    StackObj* obj = _framePointer + GetOperand(int);
                    switch (obj->v.type)
                    {
                    case StackType::Int:
                        this->PushInt(StackType::Int, ++obj->v.integer);
                        break;
                    default:
                        AssertFail("Unsupported operation");
                        break;
                    }
                }
                _ip += 4;
                break;
            //case OpCode::DecI:
            //case OpCode::PostIncI:
            //case OpCode::PostDecI:
            case OpCode::BrT:
                this->ConditionalJump(1, GetOperand(unsigned int));
                break;
            case OpCode::BrF:
                this->ConditionalJump(0, GetOperand(unsigned int));
                break;
            case OpCode::Jmp:
                _ip = GetOperand(unsigned int) - 1;
                break;
            default:
                AssertFail("Unknown code");
                running = false;
                break;
            }

            ++_ip;
        }
    }

    void VM::PushStackFrame(unsigned int returnIp, int framePointerOffset)
    {
        _stackPointer->frame.returnIp = returnIp;
        _stackPointer->frame.framePointerOffset = framePointerOffset;
        ++_stackPointer;
    }

    void VM::PushNull(size_t num /* = 1 */)
    {
        Assert(num <= 256, "Mass null push over limit");
        while (num-- > 0)
        {
            _stackPointer->v.type = StackType::Null;
            ++_stackPointer;
        }
    }

    void VM::PushId(StackType type, unsigned int id)
    {
        _stackPointer->v.type = type;
        _stackPointer->v.id = id;
        ++_stackPointer;
    }

    void VM::PushInt(StackType type, int val)
    {
        _stackPointer->v.type = type;
        _stackPointer->v.integer = val;
        ++_stackPointer;
    }

    void VM::PushFloat(float val)
    {
        _stackPointer->v.type = StackType::Float;
        _stackPointer->v.fp = val;
        ++_stackPointer;
    }

    void VM::Copy(StackObj* src, StackObj* dest)
    {
        AssertNotNull(src);
        AssertNotNull(dest);

        // TODO: Ref count
        dest->v.type = src->v.type;
        dest->v.integer = src->v.integer;
    }

    void VM::Pop(size_t num /* = 1 */)
    {
        // TODO: De-ref
        while (num-- > 0)
        {
            _stackPointer -= 1;
            _stackPointer->v.type = StackType::Top;
        }
    }

    void VM::ConditionalJump(int test, unsigned int dest)
    {
        StackObj *obj = _stackPointer - 1;
        if (obj->v.type != StackType::Boolean) AssertFail("Runtime error");
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
}
