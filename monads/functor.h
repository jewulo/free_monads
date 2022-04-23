#pragma once

#include <utility>

namespace Functor {
	/*
	* Haskell definition
	* class Functor f where
	*	fmap :: (a -> b) -> f a -> f b
	*/
	template <template <typename> class T, typename = void>
	struct Functor;

	namespace detail {
		// the default case is that any given class template is not a Functor
		template <template <typename> class T, typename = void>
		struct IsFunctorT : std::false_type {};

		// general case
		struct dummy1 {};
		struct dummy2 {};
		template <template <typename> class T>
		struct IsFunctorT <T,
						  std::enable_if_t<std::is_same<T<dummy2>,
														decltype(Functor<T>::fmap(std::declval<dummy2(dummy1)>(),
																				  std::declval<T<dummy1>>()))>::value>>
			: std::true_type {};
	}

	template <template <typename> class T>
	constexpr bool IsFunctor = detail::IsFunctorT<T>::value;
	
	template <template <typename> class F,
			  typename A,
			  typename Fun,
			  typename = std::enable_if_t<IsFunctor<F>>>
	F<std::invoke_result_t<Fun, A>> fmap(Fun&& fun, const F<A>& f) {
		return Functor<F>::fmap(std::forward<Fun>(fun), f);
	}

	namespace Test {
		// NullFunctor contains zero values
		template <typename A>
		struct  NullFunctor {};
	}
	template <>
	struct Functor<Test::NullFunctor> {
		template <typename F, typename A>
		static Test::NullFunctor<std::invoke_result_t<F, A>> fmap(F, Test::NullFunctor<A>) {
			return {};
		}
	};

	static_assert(IsFunctor<Test::NullFunctor>, "NullFunctor must be a Functor");
}
