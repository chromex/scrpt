#include "../scrpt.h"

#define COMPONENTNAME "Error"

namespace scrpt
{
    CompilerException::CompilerException(const char* message, std::shared_ptr<Token> token, ParseErr parseErr, LexErr lexErr, BytecodeGenErr bytecodeGenErr)
        : _message(message)
        , _token(token)
        , _parseErr(parseErr)
        , _lexErr(lexErr)
        , _bytecodeGenErr(bytecodeGenErr)
    {
    }

    CompilerException::CompilerException(const std::string& message, std::shared_ptr<Token> token, ParseErr parseErr, LexErr lexErr, BytecodeGenErr bytecodeGenErr)
        : _message(message)
        , _token(token)
        , _parseErr(parseErr)
        , _lexErr(lexErr)
        , _bytecodeGenErr(bytecodeGenErr)
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

        return CompilerException(ss.str(), token, ParseErr::NoError, err, BytecodeGenErr::NoError);
    }

    CompilerException CreateParseEx(ParseErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Parser Failure: " << ParseErrToString(err) << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err, LexErr::NoError, BytecodeGenErr::NoError);
    }

    CompilerException CreateParseEx(const std::string& message, ParseErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Parser Failure: " << ParseErrToString(err) << ": " << message << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, err, LexErr::NoError, BytecodeGenErr::NoError);
    }

    CompilerException CreateBytecodeGenEx(const std::string& message, BytecodeGenErr err, std::shared_ptr<Token> token)
    {
        AssertNotNull(token);

        std::stringstream ss;
        ss << "Bytecode Gen Failure: " << BytecodeGenErrToString(err) << ": " << message << std::endl;
        ss << token->GetFormattedTokenCode();

        return CompilerException(ss.str(), token, ParseErr::NoError, LexErr::NoError, err);
    }
}
