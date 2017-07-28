#include "../scrpt.h"
#include "ast.h"

#define COMPONENTNAME "AST"

namespace scrpt
{
    AstNode::AstNode()
        : _parent(nullptr)
		, _postfix(false)
        , _constant(false)
    {
    }

    AstNode::AstNode(AstNode* parent, std::shared_ptr<Token> token)
        : _token(token)
        , _parent(parent)
		, _postfix(false)
        , _constant(false)
    {
        AssertNotNull(token);
        AssertNotNull(parent);
    }

    AstNode* AstNode::AddChild(std::shared_ptr<Token> token)
    {
        _children.push_back(AstNode(this, token));
        return &_children.back();
    }

    AstNode* AstNode::AddChild(AstNode&& other)
    {
        _children.push_back(other);
        return &_children.back();
    }

    AstNode* AstNode::AddEmptyChild()
    {
        _children.push_back(AstNode());
        return &_children.back();
    }

    AstNode* AstNode::GetParent() const
    {
        return _parent;
    }

    AstNode* AstNode::CondenseBinaryOp(std::shared_ptr<Token> token, const std::vector<Symbol>& ltrMatch)
    {
        Assert(_children.size() >= 2, "Must have at least two children to perform binary op condense");

        auto t2 = std::move(_children.back());
        _children.pop_back();
        auto t1 = std::move(_children.back());
        _children.pop_back();

        AstNode* newNode = nullptr;

        if (std::any_of(ltrMatch.begin(), ltrMatch.end(), [t2](Symbol s) {s == t2.GetSym(); }))
        {

        }
        else
        {
            // Basic RTL associativity or LTR associativity but respecting precedence
            newNode = this->AddChild(token);
            newNode->AddChild(std::move(t1))->_parent = newNode;
            newNode->AddChild(std::move(t2))->_parent = newNode;
        }

        AssertNotNull(newNode);
        return newNode;
    }

	AstNode* AstNode::SwapUnaryOp(std::shared_ptr<Token> token, bool postfix)
	{
		Assert(_children.size() >= 1, "Must have at least one child to perform unary swap");

		auto t1 = std::move(_children.back());
		_children.pop_back();

		AstNode* newNode = this->AddChild(token);
		newNode->_postfix = postfix;
		newNode->AddChild(std::move(t1))->_parent = newNode;
		return newNode;
	}

    std::shared_ptr<Token> AstNode::GetToken() const
    {
        return _token;
    }

    const AstNode::ChildList& AstNode::GetChildren() const
    {
        return _children;
    }

	bool AstNode::IsPostfix() const
	{
		return _postfix;
	}

    void AstNode::SetConstant()
    {
        _constant = true;
    }

    bool AstNode::IsConstant() const
    {
        return _constant;
    }

    bool AstNode::IsEmpty() const
    {
        return _token == nullptr;
    }

    Symbol AstNode::GetSym() const
    {
        AssertNotNull(_token);
        return _token->GetSym();
    }

    const AstNode& AstNode::GetFirstChild() const
    {
        Assert(_children.size() > 0, "Cannot get first child");
        return _children.front();
    }

    const AstNode& AstNode::GetSecondChild() const
    {
        Assert(_children.size() > 1, "Cannot get second child");
        return *(++_children.begin());
    }

    const AstNode & AstNode::GetThirdChild() const
    {
        Assert(_children.size() > 2, "Cannot get third child");
        return *(++++_children.begin());
    }

    const AstNode& AstNode::GetLastChild() const
    {
        Assert(_children.size() > 0, "Cannot get last child");
        return _children.back();
    }

    static void DumpAst(const AstNode* node, unsigned int depth, std::stringstream& ss)
    {
        AssertNotNull(node);

        for (unsigned int i = 0; i < depth; ++i)
        {
            ss << "  ";
        }

        if (!node->IsEmpty())
        {
            Symbol sym = node->GetToken()->GetSym();
            ss << SymbolToString(sym);
            switch (sym)
            {
            case Symbol::Ident:
            case Symbol::Terminal: ss << ": '" << node->GetToken()->GetString() << "' "; break;
            case Symbol::Int: ss << ": " << node->GetToken()->GetInt(); break;
            case Symbol::Float: ss << ": " << node->GetToken()->GetFloat(); break;
            }
            if (node->IsPostfix()) ss << ": POSTFIX";
            if (node->IsConstant()) ss << ": CONSTANT";
            ss << std::endl;

            for (const AstNode& child : node->GetChildren())
            {
                DumpAst(&child, depth + 1, ss);
            }
        }
        else
        {
            ss << "EMTPY" << std::endl;
        }
    }

    void DumpAst(const AstNode* node)
    {
        AssertNotNull(node);

        std::stringstream ss;
        for (const AstNode& child : node->GetChildren())
        {
            DumpAst(&child, 0, ss);
        }
        std::cout << ss.str();
    }
}