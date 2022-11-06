#include "ticpp/parse_context.hpp"
#include "ticpp/stmt.hpp"

namespace ticpp {

void ParseContext::commit_stmt(const StmtRef& stmt) {
  frames.back().stmts.emplace_back(stmt);
}

void ParseContext::start() {
  frames.emplace_back();
}
ParseResult ParseContext::stop() {
  ParseResult out {};
  out.args = std::move(frames.back().args);
  out.stmts = std::move(frames.back().stmts);

  frames.pop_back();
  return out;
}

thread_local ParseContext PARSE_CONTEXT;


} // namespace ticpp
