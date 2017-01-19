#include "../scrpt.h"
#include <ctype.h>

#define COMPONENTNAME "Lexer"

namespace scrpt
{
    Lexer::Lexer(std::shared_ptr<const char> sourceData)
        : _sourceData(sourceData)
        , _token(nullptr)
        , _location(nullptr)
        , _lineStart(nullptr)
        , _line(1)
        , _position(1)
    {
        AssertNotNull(_sourceData);

        _lineStart = _location = _sourceData.get();
    }

    std::shared_ptr<Token> Lexer::Current() const
    {
        return _token;
    }

    void Lexer::Advance()
    {
        if (_token != nullptr && _token->GetSym() == Symbol::Error)
        {
            AssertFail("Lexical analysis cannot continue once an error has been hit");
            return;
        }

        // Reset
        _token = nullptr;

        // Starting token properties
        Symbol sym = Symbol::Error;
        LexErr err = LexErr::UnknownSymbol;
        std::unique_ptr<const char[]> string;
        double number = nan(nullptr);

        // Burn whitespace
        this->ConsumeWhitespace();

        _position = _location - _lineStart + 1;

        // Parse
        char c = *_location;
        char cn = *(_location + 1);
        if (c == '\0')
        {
            sym = Symbol::End;
        }
        else if (this->IsIdentCharacter(c, true))
        {
            std::unique_ptr<const char[]> ident(this->GetIdent(_location));
            size_t len = strlen(ident.get());

            if (strcmp(ident.get(), "do") == 0)				sym = Symbol::Do;
            else if (strcmp(ident.get(), "if") == 0)		sym = Symbol::If;
            else if (strcmp(ident.get(), "for") == 0)		sym = Symbol::For;
            else if (strcmp(ident.get(), "case") == 0)		sym = Symbol::Case;
            else if (strcmp(ident.get(), "else") == 0)		sym = Symbol::Else;
            else if (strcmp(ident.get(), "elif") == 0)		sym = Symbol::ElseIf;
            else if (strcmp(ident.get(), "func") == 0)		sym = Symbol::Func;
            else if (strcmp(ident.get(), "true") == 0)		sym = Symbol::True;
            else if (strcmp(ident.get(), "break") == 0)		sym = Symbol::Break;
            else if (strcmp(ident.get(), "false") == 0)		sym = Symbol::False;
            else if (strcmp(ident.get(), "while") == 0)		sym = Symbol::While;
            else if (strcmp(ident.get(), "return") == 0)	sym = Symbol::Return;
            else if (strcmp(ident.get(), "switch") == 0)	sym = Symbol::Switch;
            else if (strcmp(ident.get(), "default") == 0)	sym = Symbol::Default;
            else if (strcmp(ident.get(), "continue") == 0)  sym = Symbol::Continue;
            else
            {
                sym = Symbol::Ident;
                string.swap(ident);
            }

            _location += len;
        }
        else if (isdigit(c))
        {
            size_t rawLen = 0;
            number = this->GetNumber(_location, &rawLen);
            if (!isnan(number))
            {
                sym = Symbol::Number;
                _location += rawLen;
            }
            else
            {
                err = LexErr::InvalidNumber;
            }
        }
        else if (c == '"')
        {
            size_t rawLen;
            if (this->GetRawTermLength(_location, &rawLen))
            {
                string = this->GetTerm(_location, &err);
                if (string != nullptr)
                {
                    sym = Symbol::Terminal;
                    _location += rawLen;
                }
            }
            else
            {
                err = LexErr::NonTerminatedString;
            }
        }
#define SINGLE_CHAR_SYM(C, Sym) else if (c == C) { sym = Sym; _location += 1; }
        SINGLE_CHAR_SYM('(', Symbol::LParen)
        SINGLE_CHAR_SYM(')', Symbol::RParen)
        SINGLE_CHAR_SYM('{', Symbol::LBracket)
        SINGLE_CHAR_SYM('}', Symbol::RBracket)
        SINGLE_CHAR_SYM('[', Symbol::LSquare)
        SINGLE_CHAR_SYM(']', Symbol::RSquare)
        SINGLE_CHAR_SYM(';', Symbol::SemiColon)
        SINGLE_CHAR_SYM(':', Symbol::Colon)
        SINGLE_CHAR_SYM(',', Symbol::Comma)
        SINGLE_CHAR_SYM('.', Symbol::Dot)

#define DOUBLE_SIMPLE_SYM(C, V, Sym) else if (c == C && cn == V) { sym = Sym; _location += 2; }
        DOUBLE_SIMPLE_SYM('&', '&', Symbol::And)
        DOUBLE_SIMPLE_SYM('|', '|', Symbol::Or)

#define DOUBLE_COMPLEX_SYM(C, V, SymC, SymV) else if (c == C && cn == V) { sym = SymV; _location += 2; } SINGLE_CHAR_SYM(C, SymC)
        DOUBLE_COMPLEX_SYM('=', '=', Symbol::Assign, Symbol::Eq)
        DOUBLE_COMPLEX_SYM('*', '=', Symbol::Mult, Symbol::MultEq)
        DOUBLE_COMPLEX_SYM('/', '=', Symbol::Div, Symbol::DivEq)
        DOUBLE_COMPLEX_SYM('!', '=', Symbol::Not, Symbol::NotEq)
        DOUBLE_COMPLEX_SYM('<', '=', Symbol::LessThan, Symbol::LessThanEq)
        DOUBLE_COMPLEX_SYM('>', '=', Symbol::GreaterThan, Symbol::GreaterThanEq)
        DOUBLE_COMPLEX_SYM('%', '=', Symbol::Modulo, Symbol::ModuloEq)

#define TRIPLE_COMPLEX_SYM(C, V, Z, SymC, SymV, SymZ) else if (c == C && cn == Z) { sym = SymZ; _location += 2; } DOUBLE_COMPLEX_SYM(C, V, SymC, SymV)
        TRIPLE_COMPLEX_SYM('-', '=', '-', Symbol::Minus, Symbol::MinusEq, Symbol::MinusMinus)
        TRIPLE_COMPLEX_SYM('+', '=', '+', Symbol::Plus, Symbol::PlusEq, Symbol::PlusPlus)

        err = sym != Symbol::Error ? LexErr::NoError : err;

        // Construct token
        _token = std::make_shared<Token>(sym, _sourceData, _location, _lineStart, _line, _position, std::move(string), number);
        if (err != LexErr::NoError)
        {
            throw CreateLexerEx(err, _token);
        }
    }

    void Lexer::ConsumeWhitespace()
    {
        const char* pos = nullptr;
        do
        {
            pos = _location;

            while (*_location != '\0' && isspace(*_location))
            {
                if (*_location == '\n')
                {
                    ++_line;
                    _lineStart = _location + 1;
                }

                ++_location;
            }

            if (*_location == '/' && *(_location + 1) == '/')
            {
                _location += 2;
                while (*_location != '\0' && *_location != '\n') ++_location;
            }
        } while (pos != _location);
    }

    bool Lexer::IsIdentCharacter(char c, bool firstChar) const
    {
        return
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c == '_') ||
            (!firstChar && c >= '0' && c <= '9');
    }

    size_t Lexer::GetIdentLength(const char* c) const
    {
        unsigned int len = 0;
        while (*c != '\0' && this->IsIdentCharacter(*c++, len == 0)) ++len;
        return len;
    }

    std::unique_ptr<const char[]> Lexer::GetIdent(const char* c) const
    {
        size_t len = this->GetIdentLength(c);
        Assert(len > 0, "GetIdent has zero length in ident lexer");

        char* ident(new char[len + 1]);
        strncpy_s(ident, len + 1, c, len);

        return std::unique_ptr<const char[]>(ident);
    }

    bool Lexer::IsEndOfTerm(const char* c) const
    {
        return *c == '\0' || *c == '\n' || (*c == '"' && *(c - 1) != '\\');
    }

    bool Lexer::GetRawTermLength(const char* c, size_t* length) const
    {
        Assert(*c == '"', "Term must start with a quote");
        const char* start = c;

        do { ++c; } while (!this->IsEndOfTerm(c));
        if (*c == '\0' || *c == '\n') return false;

        *length = c - start + 1;
        return true;
    }

    size_t Lexer::GetTermLength(const char* c) const
    {
        Assert(*c == '"', "Term must start with a quote");
        const char* start = c++;
        size_t len = 0;

        while (!this->IsEndOfTerm(c))
        {
            ++len;
            if (*c++ == '\\')
            {
                ++c;
            }
        }

        return len;
    }

    std::unique_ptr<const char[]> Lexer::GetTerm(const char* c, LexErr* err)
    {
        size_t len = this->GetTermLength(c);
        char* term(new char[len + 1]);
        term[len] = '\0';
        size_t pos = 0;
        while (!this->IsEndOfTerm(++c))
        {
            if (*c == '\\')
            {
                ++c;
                switch (*c)
                {
                case 't': term[pos] = '\t'; break;
                case 'n': term[pos] = '\n'; break;
                case '\\': term[pos] = '\\'; break;
                case '"': term[pos] = '\"'; break;
                default:
                    delete[] term;
                    *err = LexErr::UnknownStringEscape;
                    return nullptr;
                }
            }
            else
            {
                term[pos] = *c;
            }

            ++pos;
        }

        return std::unique_ptr<const char[]>(term);
    }

    double Lexer::GetNumber(const char* c, size_t* rawLen) const
    {
        AssertNotNull(c);
        AssertNotNull(rawLen);

        const char* start = c;
        double num = 0;

        // Parse numerator
        do
        {
            num *= 10.0;
            num += *c - '0';
        } while (*++c != '\0' && isdigit(*c));

        // Parse denominator
        if (*c == '.')
        {
            if (!isdigit(*++c))
            {
                // error!
                return nan(nullptr);
            }

            unsigned int div = 1;
            do
            {
                div *= 10;
                num += (*c - '0') / (double)div;
            } while (*++c != '\0' && isdigit(*c));
        }

        *rawLen = c - start;
        return num;
    }

    const char* SymbolToString(Symbol sym)
    {
        switch (sym)
        {
            ENUM_CASE_TO_STRING(Symbol::Error);
            ENUM_CASE_TO_STRING(Symbol::Start);
            ENUM_CASE_TO_STRING(Symbol::LParen);
            ENUM_CASE_TO_STRING(Symbol::RParen);
            ENUM_CASE_TO_STRING(Symbol::LBracket);
            ENUM_CASE_TO_STRING(Symbol::RBracket);
            ENUM_CASE_TO_STRING(Symbol::LSquare);
            ENUM_CASE_TO_STRING(Symbol::RSquare);
            ENUM_CASE_TO_STRING(Symbol::Number);
            ENUM_CASE_TO_STRING(Symbol::Terminal);
            ENUM_CASE_TO_STRING(Symbol::Ident);
            ENUM_CASE_TO_STRING(Symbol::True);
            ENUM_CASE_TO_STRING(Symbol::False);
            ENUM_CASE_TO_STRING(Symbol::Colon);
            ENUM_CASE_TO_STRING(Symbol::Comma);
            ENUM_CASE_TO_STRING(Symbol::Dot);
            ENUM_CASE_TO_STRING(Symbol::If);
            ENUM_CASE_TO_STRING(Symbol::Else);
            ENUM_CASE_TO_STRING(Symbol::ElseIf);
            ENUM_CASE_TO_STRING(Symbol::Do);
            ENUM_CASE_TO_STRING(Symbol::While);
            ENUM_CASE_TO_STRING(Symbol::For);
            ENUM_CASE_TO_STRING(Symbol::Continue);
            ENUM_CASE_TO_STRING(Symbol::Switch);
            ENUM_CASE_TO_STRING(Symbol::Case);
            ENUM_CASE_TO_STRING(Symbol::Break);
            ENUM_CASE_TO_STRING(Symbol::Default);
            ENUM_CASE_TO_STRING(Symbol::Return);
            ENUM_CASE_TO_STRING(Symbol::Eq);
            ENUM_CASE_TO_STRING(Symbol::Assign);
            ENUM_CASE_TO_STRING(Symbol::NotEq);
            ENUM_CASE_TO_STRING(Symbol::LessThan);
            ENUM_CASE_TO_STRING(Symbol::LessThanEq);
            ENUM_CASE_TO_STRING(Symbol::GreaterThan);
            ENUM_CASE_TO_STRING(Symbol::GreaterThanEq);
            ENUM_CASE_TO_STRING(Symbol::Plus);
            ENUM_CASE_TO_STRING(Symbol::PlusPlus);
            ENUM_CASE_TO_STRING(Symbol::Minus);
            ENUM_CASE_TO_STRING(Symbol::MinusMinus);
            ENUM_CASE_TO_STRING(Symbol::Mult);
            ENUM_CASE_TO_STRING(Symbol::Div);
            ENUM_CASE_TO_STRING(Symbol::Modulo);
            ENUM_CASE_TO_STRING(Symbol::PlusEq);
            ENUM_CASE_TO_STRING(Symbol::MinusEq);
            ENUM_CASE_TO_STRING(Symbol::MultEq);
            ENUM_CASE_TO_STRING(Symbol::DivEq);
            ENUM_CASE_TO_STRING(Symbol::ModuloEq);
            ENUM_CASE_TO_STRING(Symbol::Not);
            ENUM_CASE_TO_STRING(Symbol::And);
            ENUM_CASE_TO_STRING(Symbol::Or);
            ENUM_CASE_TO_STRING(Symbol::SemiColon);
            ENUM_CASE_TO_STRING(Symbol::Func);
            ENUM_CASE_TO_STRING(Symbol::End);

        default:
            AssertFail("Missing case for symbol");
        }

        return nullptr;
    }

    const char* LexErrToString(LexErr err)
    {
        switch (err)
        {
            ENUM_CASE_TO_STRING(LexErr::NoError);
            ENUM_CASE_TO_STRING(LexErr::UnknownSymbol);
            ENUM_CASE_TO_STRING(LexErr::UnknownStringEscape);
            ENUM_CASE_TO_STRING(LexErr::NonTerminatedString);
            ENUM_CASE_TO_STRING(LexErr::InvalidNumber);

        default:
            AssertFail("Missing case for LexErr");
        }

        return nullptr;
    }

    Token::Token(
        Symbol sym,
        std::shared_ptr<const char> sourceData,
        const char* symLocation,
        const char* symLineStart,
        size_t lineNumber,
        size_t linePosition,
        std::unique_ptr<const char[]>&& string,
        double number)
        : _sym(sym)
        , _sourceData(sourceData)
        , _symLocation(symLocation)
        , _symLineStart(symLineStart)
        , _lineNumber(lineNumber)
        , _linePosition(linePosition)
        , _string(std::move(string))
        , _number(number)
    {
        AssertNotNull(_sourceData);
        AssertNotNull(_symLocation);
        AssertNotNull(_symLineStart);
    }

    Symbol Token::GetSym() const { return _sym; }
    const char * Token::SymToString() const { return SymbolToString(_sym); }
    const char* Token::GetString() const { return _string.get(); }
    double Token::GetNumber() const { return _number; }

    std::string Token::GetFormattedTokenCode() const
    {
        if (_sym == Symbol::Start)
        {
            AssertFail("No token data for Start token");
            return "";
        }

        std::stringstream ss;
        ss << _lineNumber << ":" << _linePosition << "> ";

        // Trim leading whitespace
        const char* start = _symLineStart;
        while (*start != '\0' && isspace(*start)) ++start;
        if (start == '\0')
        {
            TraceWarning("Failed to compute error message");
            return "";
        }

        // Trim newline
        const char* end = strchr(start, '\r');
        if (end == NULL) end = strchr(start, '\n');

        // And add source
        if (end == NULL)
            ss << start;
        else
            ss << std::string(start, end);

        // Start pointer line
        ss << std::endl << _lineNumber << ":" << _linePosition << "> ";
        for (size_t pos = start - _symLineStart; pos < _linePosition - 1; ++pos)
        {
            if (isspace(_symLineStart[pos]))
                ss << _symLineStart[pos];
            else
                ss << ' ';
        }
        ss << '^';

        return ss.str();
    }
}