#pragma once

namespace scrpt
{
    enum class Symbol
    {
        Error,
        Start,
        LParen,
        RParen,
        LBracket,//
        RBracket,
        LSquare,//
        RSquare,
        Int,
        Float,
        Terminal,
        Ident,
        True,
        False,
        Colon,
        Comma,
        Dot,//
        If,
        Else,//
        ElseIf,//
        Do,
        While,
        For,
        Continue,//
        Switch,//
        Case,//
        Break,//
        Default,//
        Return,
        Eq,
        Assign,
        Not,//
        NotEq,//
        LessThan,
        LessThanEq,
        GreaterThan,
        GreaterThanEq,
        Plus,
        PlusPlus,
        Minus,
        MinusMinus,
        Mult,
        Div,
        Modulo,
        Concat,
        PlusEq,
        MinusEq,
        MultEq,
        DivEq,
        ModuloEq,
        ConcatEq,
        And,
        Or,
        SemiColon,
        Func,
        End,
    };
    const char* SymbolToString(Symbol sym);

    enum class LexErr
    {
        NoError,
        UnknownSymbol,
        UnknownStringEscape,
        NonTerminatedString,
        InvalidNumber,
    };
    const char* LexErrToString(LexErr err);

    class Token
    {
    public:
        Token(
            Symbol sym,
            std::shared_ptr<const char> sourceData,
            const char* symLocation,
            const char* symLineStart,
            size_t lineNumber,
            size_t linePosition,
            std::unique_ptr<const char[]>&& string,
            int integer,
            float fp);

        Symbol GetSym() const;
        const char* SymToString() const;
        const char* GetString() const;
        int GetInt() const;
        float GetFloat() const;
        std::string GetFormattedTokenCode() const;

        bool Match(Symbol s1, Symbol s2) const;
        bool Match(Symbol s1, Symbol s2, Symbol s3) const;

    private:
        Symbol _sym;
        std::shared_ptr<const char> _sourceData;
        const char* _symLocation;
        const char* _symLineStart;
        size_t _lineNumber;
        size_t _linePosition;
        std::unique_ptr<const char[]> _string;
        int _int;
        float _float;
    };

    class Lexer
    {
    public:
        Lexer(std::shared_ptr<const char> sourceData);

        std::shared_ptr<Token> Current() const;
        void Advance();

    private:
        void ConsumeWhitespace();
        bool IsIdentCharacter(char c, bool firstChar) const;
        size_t GetIdentLength(const char* c) const;
        std::unique_ptr<const char[]> GetIdent(const char* c) const;
        bool IsEndOfTerm(const char* c) const;
        bool GetRawTermLength(const char* c, size_t* length) const;
        size_t GetTermLength(const char* c) const;
        std::unique_ptr<const char[]> GetTerm(const char* c, LexErr* err);
        bool GetNumber(const char* c, size_t* rawLen, int* integer, float* fp) const;

        std::shared_ptr<const char> _sourceData;
        std::shared_ptr<Token> _token;
        const char* _location;
        const char* _lineStart;
        size_t _line;
        size_t _position;
    };
}
