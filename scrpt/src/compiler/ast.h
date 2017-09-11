#pragma once

namespace scrpt
{
    class AstNode
    {
        typedef std::list<AstNode*> ChildList;

    public:
        AstNode();
        ~AstNode();

        AstNode* AddChild(std::shared_ptr<Token> token);
        AstNode* AddChild(AstNode* other);
        AstNode* AddEmptyChild();
        AstNode* GetParent() const;
        //void CondenseBinaryOp(std::shared_ptr<Token> token, const std::vector<Symbol>& ltrMatch);
		AstNode* SwapUnaryOp(std::shared_ptr<Token> token, bool postfix);
        std::shared_ptr<Token> GetToken() const;
        const ChildList& GetChildren() const;
		bool IsPostfix() const;
        void SetConstant();
        bool IsConstant() const;
        bool IsEmpty() const;

        // Helpers
        Symbol GetSym() const;
        const AstNode& GetFirstChild() const;
        const AstNode& GetSecondChild() const;
        const AstNode& GetThirdChild() const;
        const AstNode& GetLastChild() const;

    private:
        AstNode(const AstNode& other) = delete;
        AstNode(AstNode* parent);
        AstNode(AstNode* parent, std::shared_ptr<Token> token);

        std::shared_ptr<Token> _token;
        AstNode* _parent;
        ChildList _children;
		bool _postfix;
        bool _constant;
    };

    void DumpAst(const AstNode* node);
}
