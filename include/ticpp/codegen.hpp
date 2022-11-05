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
    return IntImmExpr::create("_" + std::to_string(PARSE_CONTEXT.acquire_arg_idx()), value);
  }
};
template<>
struct arg_conv_t<float> {
  static inline void to_ti_arg(TiArgument& arg, float value) {
    arg.type = TI_ARGUMENT_TYPE_F32;
    arg.value.f32 = value;
  }
  static inline ExprRef to_expr(float value) {
    return FloatImmExpr::create("_" + std::to_string(PARSE_CONTEXT.acquire_arg_idx()), value);
  }
};
template<>
struct arg_conv_t<TiNdArray> {
  static inline void to_ti_arg(TiArgument& arg, const TiNdArray& value) {
    arg.type = TI_ARGUMENT_TYPE_NDARRAY;
    arg.value.ndarray = value;
  }
  static inline ExprRef to_expr(const TiNdArray& value) {
    return NdArrayAllocExpr::create("_" + std::to_string(PARSE_CONTEXT.acquire_arg_idx()), value);
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


struct CodeGenIntermediate {
  std::vector<NamedArgumentRef> args;
  std::vector<StmtRef> stmts;

  template<typename TFunc, typename ... TArgs> 
  static CodeGenIntermediate create(TFunc& fn, const TArgs& ... args) {
    CodeGenIntermediate itm {};

    itm.args.reserve(sizeof...(args));
    collect_args_impl_t<TArgs ...>::collect(itm.args, args ...);

    int32_t i = 0;
    PARSE_CONTEXT.start();
    fn(arg_conv_t<TArgs>::to_expr(args) ...);
    itm.stmts = PARSE_CONTEXT.stop();

    return itm;
  }
};

extern std::string composite_python_script(
  TiArch arch,
  const CodeGenIntermediate& itm
);



template<typename TFunc, typename ... TArgs>
std::string run_codegen(TiArch arch, TFunc& fn, TArgs ... args) {
  CodeGenIntermediate itm = CodeGenIntermediate::create(fn, args ...);
  std::string out = composite_python_script(arch, itm);
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
