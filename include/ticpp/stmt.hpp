// AST statements.
// @PENGUINLIONG
#pragma once
#include "ticpp/expr.hpp"

namespace ticpp {

struct Stmt : std::enable_shared_from_this<Stmt> {
  virtual ~Stmt() {}
  virtual void to_string(std::stringstream& ss) const = 0;

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

  virtual void to_string(std::stringstream& ss) const override {
    dst_->to_string(ss);
    ss << "=";
    value_->to_string(ss);
  }
};

} // namespace ticpp
