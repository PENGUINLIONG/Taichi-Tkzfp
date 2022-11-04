// Code generator.
// @PENGUINLIONG
#pragma once
#include "ticpp/stmt.hpp"

namespace ticpp {

// Helper type to ensure the string in `TiNamedArgument` is always assigned.
struct NamedArgument {
  std::string arg_name;
  TiNamedArgument arg;
};
typedef std::unique_ptr<NamedArgument> NamedArgumentRef;



template<typename T>
struct get_arg_ti_arg {};
template<>
struct get_arg_ti_arg<int32_t> {
  static inline void get(TiArgument& arg, int32_t value) {
    arg.type = TI_ARGUMENT_TYPE_I32;
    arg.value.i32 = value;
  }
  static inline ExprRef to_expr(int32_t value) {
    return IntImmExpr::create(value);
  }
};
template<>
struct get_arg_ti_arg<float> {
  static inline void get(TiArgument& arg, float value) {
    arg.type = TI_ARGUMENT_TYPE_F32;
    arg.value.f32 = value;
  }
  static inline ExprRef to_expr(float value) {
    return FloatImmExpr::create(value);
  }
};
template<>
struct get_arg_ti_arg<TiNdArray> {
  static inline void get(TiArgument& arg, const TiNdArray& value) {
    arg.type = TI_ARGUMENT_TYPE_NDARRAY;
    arg.value.ndarray = value;
  }
  static inline ExprRef to_expr(const TiNdArray& value) {
    return NdArrayAllocExpr::create(value);
  }
};

// Collect a list of `NamedArgument`s from variadic parameter pack.
template<typename ... TArgs>
struct collect_args {};
template<typename TFirst, typename ... TArgs>
struct collect_args<TFirst, TArgs ...> {
  static inline void collect(std::vector<NamedArgumentRef>& out, const TFirst& x, const TArgs& ... args) {
    collect_args<TFirst>::collect(out, x);
    collect_args<TArgs ...>::collect(out, args ...);
  }
};
template<typename TLast>
struct collect_args<TLast> {
  static inline void collect(std::vector<NamedArgumentRef>& args, const TLast& x) {
    NamedArgumentRef arg = std::make_unique<NamedArgument>();
    arg->arg_name = "_" + std::to_string(args.size());
    arg->arg.name = arg->arg_name.c_str();
    get_arg_ti_arg<TLast>::get(arg->arg.argument, x);
    args.emplace_back(std::move(arg));
  }
};

std::string gen_code(
    TiArch arch,
    const std::vector<NamedArgumentRef>& args,
    const std::string& code);

template<typename TFunc>
struct Kernel {};

template<typename ... TExprs>
struct Kernel<std::function<void(TExprs ...)>> {
  std::function<void(TExprs ...)> fn_;

  Kernel(std::function<void(TExprs ...)> fn) : fn_(std::move(fn)) {}

  template<typename ... TArgs>
  void compile(const TArgs& ... args) {
    static_assert(sizeof...(TArgs) == sizeof...(TExprs), "");

    std::vector<NamedArgumentRef> args2;
    args2.reserve(sizeof...(args));
    collect_args<TArgs ...>::collect(args2, args ...);
    fn_(get_arg_ti_arg<TArgs>::to_expr(args) ...);
    std::cout << gen_code(TI_ARCH_VULKAN, args2, scope_context_.finish());
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
  return Kernel<typename get_func_ty<TFunc>::type> { std::move(func) };
}



} // namespace ticpp
