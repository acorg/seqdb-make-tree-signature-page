// Newick tree parser

#pragma once

#include <iostream>
#include <sstream>
#include <stack>
#include <regex>

#include "tree.hh"

// ----------------------------------------------------------------------

typedef std::stack<Node::Subtree*> NodeStack;

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
class ParsingError : public std::runtime_error
{
 public: using std::runtime_error::runtime_error;
};
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

Tree* parse_newick(std::string data);

// ----------------------------------------------------------------------
