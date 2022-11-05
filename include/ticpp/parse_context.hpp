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
  std::vector<StmtRef> stmts;
};
struct ParseContext {
  std::vector<NamedArgumentRef> args;
  std::vector<ParseFrame> frames;

  template<typename T>
  inline uint32_t reg_arg(const T& x) {
    uint32_t iarg = args.size();

    NamedArgumentRef arg = std::make_unique<NamedArgument>();
    arg->arg_name = "_" + std::to_string(iarg);
    arg->arg.name = arg->arg_name.c_str();
    arg_conv_t<T>::to_ti_arg(arg->arg.argument, x);
    args.emplace_back(std::move(arg));

    return iarg;
  }

  void commit_stmt(const StmtRef& stmt);

  void start();
  ParseResult stop();
};

extern thread_local ParseContext PARSE_CONTEXT;



template<typename T>
struct expr_conv_t {};
template<>
struct expr_conv_t<int32_t> {
  static inline ExprRef to_expr(int32_t value) {
    return IntImmExpr::create("_" + std::to_string(PARSE_CONTEXT.reg_arg(value)), value);
  }
};
template<>
struct expr_conv_t<float> {
  static inline ExprRef to_expr(float value) {
    return FloatImmExpr::create("_" + std::to_string(PARSE_CONTEXT.reg_arg(value)), value);
  }
};
template<>
struct expr_conv_t<TiNdArray> {
  static inline ExprRef to_expr(const TiNdArray& value) {
    return NdArrayAllocExpr::create("_" + std::to_string(PARSE_CONTEXT.reg_arg(value)), value);
  }
};



} // namespace ticpp
