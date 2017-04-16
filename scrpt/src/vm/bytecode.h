#pragma once

namespace scrpt
{
    enum class OpCode : unsigned char
    {
        Unknown,
        LoadNull, // reg0 
        LoadTrue, // reg0 
        LoadFalse, // reg0
		LoadInt, // reg0, int
		LoadFloat, // reg0, float
		LoadString, // reg0, unsigned int string id
		Store, // reg0, reg1
		StoreIdx, // reg0, reg1, reg2
        Eq, // reg0, reg1, reg2
        Or, // reg0, reg1, reg2
        And, // reg0, reg1, reg2
        Add, // reg0, reg1, reg2
        Sub, // reg0, reg1, reg2
        Mul, // reg0, reg1, reg2
        Div, // reg0, reg1, reg2
        Mod, // reg0, reg1, reg2
        Concat, // reg0, reg1, reg2
		Inc, // reg0
		Dec, // reg0
		PostInc, // reg0
		PostDec, // reg0
		PlusEq, // reg0, reg1
		PlusEqIdx, // reg0, reg1, reg2
		MinusEq, // reg0, reg1
		MinusEqIdx, // reg0, reg1, reg2
		MultEq, // reg0, reg1
		MultEqIdx, // reg0, reg1, reg2
		DivEq, // reg0, reg1
		DivEqIdx, // reg0, reg1, reg2
		ModuloEq, // reg0, reg1
		ModuloEqIdx, // reg0, reg1, reg2
		ConcatEq, // reg0, reg1
		ConcatEqIdx, // reg0, reg1, reg2
        LT, // reg0, reg1, reg2
        GT, // reg0, reg1, reg2
        LTE, // reg0, reg1, reg2
        GTE, // reg0, reg1, reg2
		Call, // unsigned int function offset
        Ret, // reg0
        RestoreRet, // reg0
		BrT, // reg0, unsigned int bytecode location
		BrF, // reg0, unsigned int bytecode location
		Jmp, // unsigned int bytecode location
		Index, // reg0, reg1, reg2
        MakeList, // reg0, unsigned int number of items
        __Num,
    };
    const char* OpCodeToString(OpCode code);

    struct FunctionData
    {
        std::string name;
        unsigned char nParam;
        size_t nLocalRegisters;
        unsigned int entry;
        bool external;
        std::function<void(VM*)> func;
        std::map<int, std::string> localLookup;
    };

    struct Bytecode
    {
        std::vector<unsigned char> data;
        std::vector<FunctionData> functions;
        std::vector<std::string> strings;
    };

    void Decompile(const Bytecode& bytecode);
}