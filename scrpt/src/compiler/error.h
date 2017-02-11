#pragma once

namespace scrpt
{
    class CompilerException : public std::exception
    {
    public:
        explicit CompilerException(const char* message, std::shared_ptr<Token> token, ParseErr parseErr, LexErr lexErr, BytecodeGenErr bytecodeGenErr);
        explicit CompilerException(const std::string& message, std::shared_ptr<Token> token, ParseErr parseErr, LexErr lexErr, BytecodeGenErr bytecodeGenErr);

        virtual char const* what() const;
        std::shared_ptr<Token> GetToken() const;
        ParseErr GetParseErr() const;
        LexErr GetLexErr() const;
        BytecodeGenErr GetBytecodeGenErr() const;

    private:
        std::string _message;
        std::shared_ptr<Token> _token;
        ParseErr _parseErr;
        LexErr _lexErr;
        BytecodeGenErr _bytecodeGenErr;
    };

    CompilerException CreateLexerEx(LexErr err, std::shared_ptr<Token> token);
    CompilerException CreateParseEx(ParseErr err, std::shared_ptr<Token> token);
    CompilerException CreateParseEx(const std::string& message, ParseErr err, std::shared_ptr<Token> token);
    CompilerException CreateBytecodeGenEx(BytecodeGenErr err, std::shared_ptr<Token> token);
    CompilerException CreateBytecodeGenEx(const std::string& message, BytecodeGenErr err, std::shared_ptr<Token> token);
}
