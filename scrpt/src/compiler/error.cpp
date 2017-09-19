#include "../scrpt.h"

#define COMPONENTNAME "Error"

namespace scrpt
{
	Exception::Exception(const std::string& message, std::shared_ptr<Token> token, Err err)
		: _message(message)
		, _token(token)
		, _err(err)
	{
	}

	Exception::Exception(const std::string& message, Err err)
		: _message(message)
		, _err(err)
	{
	}

	const char* Exception::what() const
    {
        return _message.c_str();
    }

    std::shared_ptr<Token> Exception::GetToken() const
    {
        return _token;
    }

    Err Exception::GetErr() const
    {
        return _err;
    }

    Exception CreateEx(Err err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << ErrToString(err) << std::endl;
        ss << token->GetFormattedTokenCode();

        return Exception(ss.str(), token, err);
    }

    Exception CreateEx(const std::string& message, Err err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << ErrToString(err) << ": " << message << std::endl;
        ss << token->GetFormattedTokenCode();

        return Exception(ss.str(), token, err);
    }

	Exception CreateEx(const std::string& message, Err err)
	{
		std::stringstream ss;
		ss << ErrToString(err) << ": " << message;

		return Exception(ss.str(), err);
	}

    const char* ErrToString(Err err)
    {
        switch (err)
        {
            ENUM_CASE_TO_STRING(Err::NoError);

            ENUM_CASE_TO_STRING(Err::Lexer_UnknownSymbol);
            ENUM_CASE_TO_STRING(Err::Lexer_UnknownStringEscape);
            ENUM_CASE_TO_STRING(Err::Lexer_NonTerminatedString);
            ENUM_CASE_TO_STRING(Err::Lexer_InvalidNumber);

            ENUM_CASE_TO_STRING(Err::Parser_UnexpectedSymbol);
            ENUM_CASE_TO_STRING(Err::Parser_BlockExpected);
            ENUM_CASE_TO_STRING(Err::Parser_ExpressionExpected);
            ENUM_CASE_TO_STRING(Err::Parser_StatementExpected);

            ENUM_CASE_TO_STRING(Err::BytecodeGen_UnexpectedToken);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_FunctionRedefinition);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_ParameterCountExceeded);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_UndeclaredFunctionReference);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_IncorrectCallArity);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_UndeclaredIdentifierReference);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_MulipleDeclaration);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_DuplicateParameterName);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_InsufficientRegisters);
            ENUM_CASE_TO_STRING(Err::BytecodeGen_ClassRedefinition);

            ENUM_CASE_TO_STRING(Err::VM_FailedFunctionLookup);
            ENUM_CASE_TO_STRING(Err::VM_UnsupportedOperandType);
            ENUM_CASE_TO_STRING(Err::VM_OperandMismatch);
            ENUM_CASE_TO_STRING(Err::VM_StackOverflow);
            ENUM_CASE_TO_STRING(Err::VM_StackUnderflow);
            ENUM_CASE_TO_STRING(Err::VM_UnexpectedParamType);
            ENUM_CASE_TO_STRING(Err::VM_BadParamRequest);
            ENUM_CASE_TO_STRING(Err::VM_NotImplemented);
            ENUM_CASE_TO_STRING(Err::VM_IncorrectArity);

        default:
            AssertFail("Missing case for Err");
        }

        return nullptr;
    }
}
