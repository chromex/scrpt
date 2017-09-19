#pragma once

namespace scrpt
{
    // Note: Add new error codes at the end of the relevant group so old errors persist their number code
    enum class Err
    {
        NoError = 0,

        Lexer_UnknownSymbol = 1,
        Lexer_UnknownStringEscape,
        Lexer_NonTerminatedString,
        Lexer_InvalidNumber,
        
        Parser_UnexpectedSymbol = 1000,
        Parser_BlockExpected,
        Parser_ExpressionExpected,
        Parser_StatementExpected,

        BytecodeGen_UnexpectedToken = 2000,
        BytecodeGen_FunctionRedefinition,
        BytecodeGen_ParameterCountExceeded,
        BytecodeGen_UndeclaredFunctionReference,
        BytecodeGen_IncorrectCallArity,
        BytecodeGen_UndeclaredIdentifierReference,
        BytecodeGen_MulipleDeclaration,
        BytecodeGen_DuplicateParameterName,
        BytecodeGen_InsufficientRegisters,
        BytecodeGen_ClassRedefinition,
        BytecodeGen_BadConstructorName,

        VM_FailedFunctionLookup = 3000,
        VM_UnsupportedOperandType,
        VM_OperandMismatch,
        VM_StackOverflow,
        VM_StackUnderflow,
        VM_UnexpectedParamType,
        VM_BadParamRequest,
        VM_NotImplemented,
        VM_IncorrectArity,
    };
    const char* ErrToString(Err err);

    class Exception : public std::exception
    {
    public:
		explicit Exception(const std::string& message, std::shared_ptr<Token> token, Err err);
		explicit Exception(const std::string& message, Err err);

        virtual char const* what() const;
        std::shared_ptr<Token> GetToken() const;
        Err GetErr() const;

    private:
        std::string _message;
        std::shared_ptr<Token> _token;
        Err _err;
    };

    Exception CreateEx(Err err, std::shared_ptr<Token> token);
    Exception CreateEx(const std::string& message, Err err, std::shared_ptr<Token> token);
	Exception CreateEx(const std::string& message, Err err);
}
