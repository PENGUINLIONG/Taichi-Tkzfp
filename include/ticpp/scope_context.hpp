// AST parsing scope context.
// @PENGUINLIONG
#pragma once
#include "ticpp/common.hpp"

namespace ticpp {

struct Stmt;

struct ScopeContext {
  size_t indent = 4;
  std::stringstream ss;

  void commit_stmt(const Stmt* stmt);
  std::string finish();
};

extern thread_local ScopeContext scope_context_;

} // namespace ticpp
