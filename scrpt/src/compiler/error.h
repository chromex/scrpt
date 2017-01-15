#pragma once

namespace scrpt
{
    class CompilerException : public std::exception
    {
    public:
        explicit CompilerException(const char* message, std::shared_ptr<Token> token, ParseErr parseErr, LexErr lexErr);
        explicit CompilerException(const std::string& message, std::shared_ptr<Token> token, ParseErr parseErr, LexErr lexErr);

        virtual char const* what() const;
        std::shared_ptr<Token> GetToken() const;
        ParseErr GetParseErr() const;
        LexErr GetLexErr() const;

    private:
        std::string _message;
        std::shared_ptr<Token> _token;
        ParseErr _parseErr;
        LexErr _lexErr;
    };

    CompilerException CreateLexerEx(LexErr err, std::shared_ptr<Token> token);
    CompilerException CreateParseEx(ParseErr err, std::shared_ptr<Token> token);
    CompilerException CreateParseEx(const std::string& message, ParseErr err, std::shared_ptr<Token> token);
}
