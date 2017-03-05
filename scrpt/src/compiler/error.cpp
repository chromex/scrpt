#include "../scrpt.h"

#define COMPONENTNAME "Error"

namespace scrpt
{
	CompilerException::CompilerException(const std::string& message, std::shared_ptr<Token> token, ParseErr parseErr)
		: _message(message)
		, _token(token)
		, _parseErr(parseErr)
	{
	}

	CompilerException::CompilerException(const std::string& message, std::shared_ptr<Token> token, LexErr lexErr)
		: _message(message)
		, _token(token)
		, _lexErr(lexErr)
	{
	}

	CompilerException::CompilerException(const std::string& message, std::shared_ptr<Token> token, BytecodeGenErr bytecodeGenErr)
		: _message(message)
		, _token(token)
		, _bytecodeGenErr(bytecodeGenErr)
	{
	}

	CompilerException::CompilerException(const std::string& message, RuntimeErr err)
		: _message(message)
		, _runtimeErr(err)
	{
	}

	const char* CompilerException::what() const
    {
        return _message.c_str();
    }

    std::shared_ptr<Token> CompilerException::GetToken() const
    {
        return _token;
    }

    ParseErr CompilerException::GetParseErr() const
    {
        return _parseErr;
    }

    LexErr CompilerException::GetLexErr() const
    {
        return _lexErr;
    }

    BytecodeGenErr CompilerException::GetBytecodeGenErr() const
    {
        return _bytecodeGenErr;
    }

    CompilerException CreateLexerEx(LexErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Lexical Analysis Failure: " << LexErrToString(err) << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err);
    }

    CompilerException CreateParseEx(ParseErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Parser Failure: " << ParseErrToString(err) << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err);
    }

    CompilerException CreateParseEx(const std::string& message, ParseErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Parser Failure: " << ParseErrToString(err) << ": " << message << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err);
    }

    CompilerException CreateBytecodeGenEx(BytecodeGenErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Bytecode Gen Failure: " << BytecodeGenErrToString(err) << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err);
    }

    CompilerException CreateBytecodeGenEx(const std::string& message, BytecodeGenErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Bytecode Gen Failure: " << BytecodeGenErrToString(err) << ": " << message << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err);
    }

	CompilerException CreateRuntimeEx(const std::string& message, RuntimeErr err)
	{
		std::stringstream ss;
		ss << "Runtime Failure: " << RuntimeErrToString(err) << ": " << message << std::endl;

		return CompilerException(ss.str(), err);
	}
}
