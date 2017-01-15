#include "../scrpt.h"
#include "ast.h"

#define COMPONENTNAME "AST"

namespace scrpt
{
	AstNode::AstNode()
		: _parent(nullptr)
	{
	}

	AstNode::AstNode(AstNode* parent, std::shared_ptr<Token> token)
		: _token(token)
		, _parent(parent)
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

	AstNode* AstNode::GetParent() const
	{
		return _parent;
	}

	AstNode* AstNode::CondenseBinaryOp(std::shared_ptr<Token> token)
	{
		Assert(_children.size() >= 2, "Must have at least two children to perform binary op condense");

		auto t2 = std::move(_children.back());
		_children.pop_back();
		auto t1 = std::move(_children.back());
		_children.pop_back();

		AstNode* newNode = this->AddChild(token);
		newNode->AddChild(std::move(t1))->_parent = this;
		newNode->AddChild(std::move(t2))->_parent = this;
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

	static void DumpAst(const AstNode* node, unsigned int depth, std::stringstream& ss)
	{
		AssertNotNull(node);

		for (unsigned int i = 0; i < depth; ++i)
		{
			ss << "  ";
		}

		Symbol sym = node->GetToken()->GetSym();
		ss << SymbolToString(sym);
		switch (sym)
		{
		case Symbol::Ident:
		case Symbol::Terminal: ss << ": '" << node->GetToken()->GetString() << "' "; break;
		case Symbol::Number: ss << ": " << node->GetToken()->GetNumber(); break;
		}
		ss << std::endl;

		for (const AstNode& child : node->GetChildren())
		{
			DumpAst(&child, depth + 1, ss);
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