#include "ticpp/stmt.hpp"
#include "ticpp/parse_context.hpp"

namespace ticpp {

void Stmt::commit() {
  PARSE_CONTEXT.commit_stmt(shared_from_this());
}

} // namespace ticpp
