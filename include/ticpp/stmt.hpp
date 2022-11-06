// AST statements.
// @PENGUINLIONG
#pragma once
#include "ticpp/expr.hpp"

namespace ticpp {

struct Stmt : std::enable_shared_from_this<Stmt> {
  virtual ~Stmt() {}
  virtual void to_string(PythonScriptWriter& ss) const = 0;

  template<typename T>
  inline static std::shared_ptr<Stmt> create(T&& x) {
    return std::shared_ptr<Stmt>(static_cast<Stmt*>(new T(std::move(x))));
  }

  void commit();
};
typedef std::shared_ptr<Stmt> StmtRef;




struct StoreStmt : public Stmt {
  ExprRef dst_;
  ExprRef value_;

  inline static StmtRef create(const ExprRef& dst, const ExprRef& value) {
    StoreStmt out {};
    out.dst_ = dst;
    out.value_ = value;
    return Stmt::create(std::move(out));
  }

  virtual void to_string(PythonScriptWriter& ss) const override {
    dst_->to_string(ss);
    ss << " = (";
    value_->to_string(ss);
    ss << ")";
  }
};

struct ForStmt : public Stmt {
  ExprRef index_;
  ExprRef range_;
  std::vector<StmtRef> then_block_;

  inline static StmtRef create(
    ExprRef&& index,
    ExprRef&& range,
    std::vector<StmtRef>&& then_block
  ) {
    ForStmt out {};
    out.index_ = std::move(index);
    out.range_ = std::move(range);
    out.then_block_ = std::move(then_block);
    return Stmt::create(std::move(out));
  }

  virtual void to_string(PythonScriptWriter& ss) const override {
    ss << "for ";
    index_->to_string(ss);
    ss << " in ti.grouped(";
    range_->to_string(ss);
    ss << "):";
    ss.commit_line();
    ss.push_indent();
    for (const StmtRef& stmt : then_block_) {
      stmt->to_string(ss);
      ss.commit_line();
    }
    ss.pop_indent();
  }
};

} // namespace ticpp
