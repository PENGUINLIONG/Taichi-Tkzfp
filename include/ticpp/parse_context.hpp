// AST parsing scope context.
// @PENGUINLIONG
#pragma once
#include "ticpp/stmt.hpp"

namespace ticpp {

struct ParseFrame {
  std::vector<StmtRef> stmts;
};

struct ParseContext {
  uint32_t i = 0;
  std::vector<ParseFrame> frames;

  inline uint32_t acquire_arg_idx() {
    return i++;
  }

  void commit_stmt(const StmtRef& stmt);

  void start();
  std::vector<StmtRef> stop();
};

extern thread_local ParseContext PARSE_CONTEXT;

} // namespace ticpp
