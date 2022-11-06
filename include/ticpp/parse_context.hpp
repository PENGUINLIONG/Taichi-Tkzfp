// AST parsing scope context.
// @PENGUINLIONG
#pragma once
#include "ticpp/stmt.hpp"

namespace ticpp {

template<typename T>
struct arg_conv_t {};
template<>
struct arg_conv_t<int32_t> {
  static inline void to_ti_arg(TiArgument& arg, int32_t value) {
    arg.type = TI_ARGUMENT_TYPE_I32;
    arg.value.i32 = value;
  }
};
template<>
struct arg_conv_t<float> {
  static inline void to_ti_arg(TiArgument& arg, float value) {
    arg.type = TI_ARGUMENT_TYPE_F32;
    arg.value.f32 = value;
  }
};
template<>
struct arg_conv_t<TiNdArray> {
  static inline void to_ti_arg(TiArgument& arg, const TiNdArray& value) {
    arg.type = TI_ARGUMENT_TYPE_NDARRAY;
    arg.value.ndarray = value;
  }
};


// Helper type to ensure the string in `TiNamedArgument` is always assigned.
struct NamedArgument {
  std::string arg_name;
  TiNamedArgument arg;
};
typedef std::unique_ptr<NamedArgument> NamedArgumentRef;



struct ParseResult {
  std::vector<NamedArgumentRef> args;
  std::vector<StmtRef> stmts;
};


struct ParseFrame {
  std::vector<NamedArgumentRef> args;
  std::vector<StmtRef> stmts;
};
struct ParseContext {
  std::vector<ParseFrame> frames;

  template<typename T>
  inline uint32_t reg_arg(const T& x) {
    uint32_t iarg = frames.back().args.size();

    NamedArgumentRef arg = std::make_unique<NamedArgument>();
    arg->arg_name = "_" + std::to_string(iarg);
    arg->arg.name = arg->arg_name.c_str();
    arg_conv_t<T>::to_ti_arg(arg->arg.argument, x);
    frames.back().args.emplace_back(std::move(arg));

    return iarg;
  }

  void commit_stmt(const StmtRef& stmt);

  void start();
  ParseResult stop();
};

extern thread_local ParseContext PARSE_CONTEXT;




struct IntValue {
  ExprRef expr_;

  IntValue(int32_t value) : expr_(IntImmExpr::create(value)) {}
  IntValue(ExprRef&& expr) : expr_(std::move(expr)) {}

  friend IntValue operator+(const IntValue& a, const IntValue& b) {
    return IntValue { AddExpr::create(a.expr_, b.expr_) };
  }
  friend IntValue operator-(const IntValue& a, const IntValue& b) {
    return IntValue { SubExpr::create(a.expr_, b.expr_) };
  }
};
struct FloatValue {
  ExprRef expr_;

  FloatValue(float value) : expr_(FloatImmExpr::create(value)) {}
  FloatValue(ExprRef&& expr) : expr_(std::move(expr)) {}

  friend FloatValue operator+(const FloatValue& a, const FloatValue& b) {
    return FloatValue { AddExpr::create(a.expr_, b.expr_) };
  }
};
struct VectorValue {
  ExprRef expr_;

  VectorValue(std::vector<ExprRef>&& exprs) : expr_(VectorExpr::create(std::move(exprs))) {}
  VectorValue(std::initializer_list<IntValue> values) {
    std::vector<ExprRef> values2;
    for (const auto& value : values) {
      values2.emplace_back(value.expr_);
    }
    expr_ = VectorExpr::create(std::move(values2));
  }
  VectorValue(std::initializer_list<FloatValue> values) {
    std::vector<ExprRef> values2;
    for (const auto& value : values) {
      values2.emplace_back(value.expr_);
    }
    expr_ = VectorExpr::create(std::move(values2));
  }
};
struct IterVarValue {
  ExprRef expr_;

  IterVarValue(const ExprRef& expr) : expr_(expr) {}

  IntValue operator[](int32_t i) const {
    return IntValue { IndexExpr::create(expr_, IntImmExpr::create(i)) };
  }
};
struct NdArrayValue {
  ExprRef expr_;

  NdArrayValue(const std::string& name, const TiNdArray& ndarray) :
    expr_(NdArrayAllocExpr::create(name, ndarray)) {}
  NdArrayValue(ExprRef&& expr) : expr_(std::move(expr)) {}

  NdArrayValue operator[](const IterVarValue& itervar) {
    return NdArrayValue { IndexExpr::create(expr_, itervar.expr_) };
  }
  NdArrayValue operator[](const std::vector<IntValue>& idxs) {
    std::vector<ExprRef> idxs2(idxs.size());
    for (size_t i = 0; i < idxs.size(); ++i) {
      idxs2.at(i) = idxs.at(i).expr_;
    }
    return NdArrayValue { IndexExpr::create(expr_, VectorExpr::create(std::move(idxs2))) };
  }

  NdArrayValue& operator=(const IntValue& x) {
    StoreStmt::create(expr_, x.expr_)->commit();
    return *this;
  }
  NdArrayValue& operator=(const FloatValue& x) {
    StoreStmt::create(expr_, x.expr_)->commit();
    return *this;
  }
  NdArrayValue& operator=(const VectorValue& x) {
    StoreStmt::create(expr_, x.expr_)->commit();
    return *this;
  }
};

struct ForControlFlow {
  ExprRef itervar_;
  ExprRef range_;
  StmtRef stmt_;

  ForControlFlow(const ExprRef& range) :
    itervar_(IterVarExpr::create()),
    range_(range) {}

  template<typename T>
  void operator<<(T block) {
    PARSE_CONTEXT.start();
    block(IterVarValue { itervar_ });
    ParseResult res = PARSE_CONTEXT.stop();
    assert(res.args.empty());

    stmt_ = ForStmt::create(std::move(itervar_), std::move(range_), std::move(res.stmts));
    stmt_->commit();
  }
};

#define TICPP_FOR(itervar, range) \
  ::ticpp::ForControlFlow(range.expr_) << [&](const ::ticpp::IterVarValue& itervar)

inline IntValue to_int(const FloatValue& value) {
  return IntValue { TypeCastExpr::create("ti.i32", value.expr_) };
}
inline FloatValue to_float(const IntValue& value) {
  return FloatValue { TypeCastExpr::create("ti.f32", value.expr_) };
}










template<typename T>
struct expr_conv_t {};
template<>
struct expr_conv_t<int32_t> {
  static inline IntValue to_expr(int32_t value) {
    return IntValue { IntImmExpr::create("_" + std::to_string(PARSE_CONTEXT.reg_arg(value)), value) };
  }
};
template<>
struct expr_conv_t<float> {
  static inline FloatValue to_expr(float value) {
    return FloatValue { FloatImmExpr::create("_" + std::to_string(PARSE_CONTEXT.reg_arg(value)), value) };
  }
};
template<>
struct expr_conv_t<TiNdArray> {
  static inline NdArrayValue to_expr(const TiNdArray& value) {
    return NdArrayValue { NdArrayAllocExpr::create("_" + std::to_string(PARSE_CONTEXT.reg_arg(value)), value) };
  }
};



} // namespace ticpp
