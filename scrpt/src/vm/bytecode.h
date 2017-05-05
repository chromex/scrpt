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
        LoadFunc, // reg0, unsigned int function id
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
        LT, // reg0, reg1, reg2
        GT, // reg0, reg1, reg2
        LTE, // reg0, reg1, reg2
        GTE, // reg0, reg1, reg2
        Call, // reg0, char num params
        Ret, // reg0
        RestoreRet, // reg0
		BrT, // reg0, unsigned int bytecode location
		BrF, // reg0, unsigned int bytecode location
		Jmp, // unsigned int bytecode location
		Index, // reg0, reg1, reg2
        MakeList, // reg0, unsigned int number of items
        MakeMap, // reg0, unsigned int number of items
        Push, // reg0
        PopN, // char number of pops
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