// Code generator.
// @PENGUINLIONG
#pragma once
#include "ticpp/parse_context.hpp"

namespace ticpp {

// Helper type to ensure the string in `TiNamedArgument` is always assigned.
struct NamedArgument {
  std::string arg_name;
  TiNamedArgument arg;
};
typedef std::unique_ptr<NamedArgument> NamedArgumentRef;



template<typename T>
struct arg_conv_t {};
template<>
struct arg_conv_t<int32_t> {
  static inline void to_ti_arg(TiArgument& arg, int32_t value) {
    arg.type = TI_ARGUMENT_TYPE_I32;
    arg.value.i32 = value;
  }
  static inline ExprRef to_expr(int32_t value) {
    return IntImmExpr::create(value);
  }
};
template<>
struct arg_conv_t<float> {
  static inline void to_ti_arg(TiArgument& arg, float value) {
    arg.type = TI_ARGUMENT_TYPE_F32;
    arg.value.f32 = value;
  }
  static inline ExprRef to_expr(float value) {
    return FloatImmExpr::create(value);
  }
};
template<>
struct arg_conv_t<TiNdArray> {
  static inline void to_ti_arg(TiArgument& arg, const TiNdArray& value) {
    arg.type = TI_ARGUMENT_TYPE_NDARRAY;
    arg.value.ndarray = value;
  }
  static inline ExprRef to_expr(const TiNdArray& value) {
    return NdArrayAllocExpr::create(value);
  }
};

// Collect a list of `NamedArgument`s from variadic parameter pack.
template<typename ... TArgs>
struct collect_args_impl_t {};
template<typename TFirst, typename ... TArgs>
struct collect_args_impl_t<TFirst, TArgs ...> {
  static inline void collect(std::vector<NamedArgumentRef>& out, const TFirst& x, const TArgs& ... args) {
    collect_args_impl_t<TFirst>::collect(out, x);
    collect_args_impl_t<TArgs ...>::collect(out, args ...);
  }
};
template<typename TLast>
struct collect_args_impl_t<TLast> {
  static inline void collect(std::vector<NamedArgumentRef>& args, const TLast& x) {
    NamedArgumentRef arg = std::make_unique<NamedArgument>();
    arg->arg_name = "_" + std::to_string(args.size());
    arg->arg.name = arg->arg_name.c_str();
    arg_conv_t<TLast>::to_ti_arg(arg->arg.argument, x);
    args.emplace_back(std::move(arg));
  }
};



template<typename ... TArgs>
std::vector<NamedArgumentRef> collect_args(const TArgs& ... args) {
  std::vector<NamedArgumentRef> out;
  out.reserve(sizeof...(args));
  collect_args_impl_t<TArgs ...>::collect(out, args ...);
  return out;
}
template<typename TFunc, typename ... TArgs>
std::vector<StmtRef> collect_stmts(TFunc& fn, const TArgs& ... args) {
  PARSE_CONTEXT.start();
  fn(arg_conv_t<TArgs>::to_expr(args) ...);
  std::vector<StmtRef> stmts = PARSE_CONTEXT.stop();
  return stmts;
}



extern std::string composite_python_script(
  TiArch arch,
  const std::vector<NamedArgumentRef>& args,
  const std::vector<StmtRef>& stmts
);



template<typename TFunc, typename ... TArgs>
std::string run_codegen(TiArch arch, TFunc& fn, TArgs ... args) {
  std::vector<NamedArgumentRef> args2 = collect_args(args ...);
  std::vector<StmtRef> stmts = collect_stmts(fn, args ...);
  std::string out = composite_python_script(arch, args2, stmts);
  return out;
}


template<typename TFunc>
struct Kernel {};
template<typename ... TExprs>
struct Kernel<std::function<void(TExprs ...)>> {
  TiArch arch_;
  std::function<void(TExprs ...)> fn_;

  Kernel(TiArch arch, std::function<void(TExprs ...)> fn) :
    arch_(arch), fn_(std::move(fn)) {}

  template<typename ... TArgs>
  std::string compile(const TArgs& ... args) {
    static_assert(sizeof...(TArgs) == sizeof...(TExprs), "");
    return run_codegen(arch_, fn_, args ...);
  }
};



template<typename TFunc>
struct get_func_ty {};
template<typename ... TArgs>
struct get_func_ty<void(TArgs ...)> {
  typedef std::function<void(TArgs ...)> type;
};
template<typename ... TArgs>
struct get_func_ty<void(*)(TArgs ...)> {
  typedef std::function<void(TArgs ...)> type;
};


template<typename TFunc>
auto to_kernel(TFunc f) {
  typename get_func_ty<TFunc>::type func { f };
  return Kernel<typename get_func_ty<TFunc>::type>(TI_ARCH_VULKAN, std::move(func));
}



} // namespace ticpp
