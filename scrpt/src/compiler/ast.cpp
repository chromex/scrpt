#include "../scrpt.h"

#define COMPONENTNAME "AST"

namespace scrpt
{
	AstNode::AstNode(Symbol sym)
		: _sym(sym)
	{
		Assert(_sym != Symbol::Error, "Cannot create error node in AST");
		Assert(_sym != Symbol::End, "Cannot create end node in AST");
	}

	Symbol AstNode::GetSym() const
	{
		return _sym;
	}
}