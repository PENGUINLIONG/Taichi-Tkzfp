// AST parsing scope context.
// @PENGUINLIONG
#pragma once
#include "ticpp/stmt.hpp"

namespace ticpp {

struct ParseFrame {
  std::vector<StmtRef> stmts;
};

struct ParseContext {
  std::vector<ParseFrame> frames;

  void commit_stmt(const StmtRef& stmt);

  void start();
  std::vector<StmtRef> stop();
};

extern thread_local ParseContext PARSE_CONTEXT;

} // namespace ticpp
