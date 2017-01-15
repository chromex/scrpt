#pragma once

namespace scrpt
{
    class AstNode
    {
        typedef std::list<AstNode> ChildList;

    public:
        AstNode();

        AstNode* AddChild(std::shared_ptr<Token> token);
        AstNode* AddChild(AstNode&& other);
        AstNode* GetParent() const;
        AstNode* CondenseBinaryOp(std::shared_ptr<Token> token);
        std::shared_ptr<Token> GetToken() const;
        const ChildList& GetChildren() const;

    private:
        AstNode(AstNode* parent, std::shared_ptr<Token> token);

        std::shared_ptr<Token> _token;
        AstNode* _parent;
        ChildList _children;
    };

    void DumpAst(const AstNode* node);
}
