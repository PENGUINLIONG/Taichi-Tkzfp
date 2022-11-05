// AST expressions.
// @PENGUINLIONG
#pragma once
#include <ticpp/common.hpp>

namespace ticpp {

struct Expr {
  virtual ~Expr() {}
  virtual void to_string(std::stringstream& ss) const = 0;

  template<typename T>
  inline static std::shared_ptr<Expr> create(T&& x) {
    return std::shared_ptr<Expr>(static_cast<Expr*>(new T(std::move(x))));
  }
};
typedef std::shared_ptr<Expr> ExprRef;



struct AddExpr : public Expr {
  ExprRef a_;
  ExprRef b_;

  inline static ExprRef create(const ExprRef& a, const ExprRef& b) {
    AddExpr out {};
    out.a_ = a;
    out.b_ = b;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << "(";
    a_->to_string(ss);
    ss << "+";
    b_->to_string(ss);
    ss << ")";
  }
};
struct SubExpr : public Expr {
  ExprRef a_;
  ExprRef b_;

  inline static ExprRef create(const ExprRef& a, const ExprRef& b) {
    SubExpr out {};
    out.a_ = a;
    out.b_ = b;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << "(";
    a_->to_string(ss);
    ss << "-";
    b_->to_string(ss);
    ss << ")";
  }
};

struct IntImmExpr : public Expr {
  std::string arg_name_;
  int32_t value_;

  inline static ExprRef create(const std::string& arg_name, int32_t value) {
    IntImmExpr out {};
    out.arg_name_ = arg_name;
    out.value_ = value;
    return Expr::create(std::move(out));
  }
  inline static ExprRef create(int32_t value) {
    IntImmExpr out {};
    out.value_ = value;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    if (arg_name_.empty()) {
      ss << "ti.i32(" << value_ << ")";
    } else {
      ss << arg_name_;
    }
  }
};

struct FloatImmExpr : public Expr {
  std::string arg_name_;
  float value_;

  inline static ExprRef create(const std::string& arg_name, float value) {
    FloatImmExpr out {};
    out.arg_name_ = arg_name;
    out.value_ = value;
    return Expr::create(std::move(out));
  }
  inline static ExprRef create(int32_t value) {
    FloatImmExpr out {};
    out.value_ = value;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    if (arg_name_.empty()) {
      ss << "ti.f32(" << value_ << ")";
    } else {
      ss << arg_name_;
    }
  }
};

struct IterVarExpr : public Expr {
  std::string name_;
  size_t begin_;
  size_t end_;
  size_t step_;

  inline static ExprRef create(size_t begin, size_t end, size_t step) {
    static size_t id_counter_ = 0;
    IterVarExpr out {};
    out.name_ = "it_" + std::to_string(id_counter_++);
    out.begin_ = begin;
    out.end_ = end;
    out.step_ = step;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << name_;
  }
};

struct IndexExpr : public Expr {
  ExprRef alloc_;
  ExprRef index_;

  inline static ExprRef create(const ExprRef& alloc, const ExprRef& index) {
    IndexExpr out {};
    out.alloc_ = alloc;
    out.index_ = index;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    alloc_->to_string(ss);
    ss << "[";
    index_->to_string(ss);
    ss << "]";
  }
};

struct TupleExpr : public Expr {
  std::vector<ExprRef> elems_;

  inline static ExprRef create(std::vector<ExprRef>&& elems) {
    TupleExpr out {};
    out.elems_ = elems;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    if (elems_.empty()) {
      ss << ", ";
    } else {
      for (const auto& elem : elems_) {
        ss << "(";
        elem->to_string(ss);
        ss << "),";
      }
    }
  }
};

struct NdArrayAllocExpr : public Expr {
  std::string arg_name_;
  TiNdArray ndarray_;

  inline static ExprRef create(const std::string& arg_name, const TiNdArray& ndarray) {
    static size_t id_counter_ = 0;
    NdArrayAllocExpr out {};
    out.arg_name_ = arg_name;
    out.ndarray_ = ndarray;
    return Expr::create(std::move(out));
  }

  virtual void to_string(std::stringstream& ss) const override {
    ss << arg_name_;
  }
};

} // namespace ticpp
