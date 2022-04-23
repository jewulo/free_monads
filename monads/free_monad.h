#pragma once

/*
* Every type f that is a Functor has a “Free” Monad.
* A Free Monad is some category theory gobbledygook
* but it’s basically the simplest possible Monad that
* doesn’t throw any information away.
* 
* In Haskell it is defined quite simply:
* 
* data Free f a = Return a | Bind (f (Free f a))
* instance (Functor f) => Monad (Free f) where
*/

#include "monad.h"
#include "functor.h"

#include <boost/variant.hpp>

namespace Free {
	template <template <typename> class F, typename A>
	struct Return;

	template <template <typename> class F, typename A>
	struct Bind;

	template <template <typename> class F, typename A>
	struct Free {
		using ContainedType = A;
		using ReturnType	= Return<F, A>;

		boost::variant<boost::recursive_wrapper<Return<F, A>>,
					   boost::recursive_wrapper<Bind<F, A>>>
			v;
	};

	// specific case of Return and Bind
	template <template <typename> class F, typename A>
	struct Return {
		A a;
	};

	template <template <typename> class F, typename A>
	struct Bind {
		F<Free<F, A>> x;
	};

	// Helper Function Templates for Return and Bind

	// This one is for when you know the Functor template argument
	// - it can deduce the contained type.
	template <template <typename> class F, typename A>
	Free<F, A> make_return(const A& x)
	{
		return { Return<F,A>{x} };
	}

	// This one is for when you know the resulting Free type but not
	// the template arguments.
	template <typename class FA>
	FA make_return(const typename FA::ContainedType& x)
	{
		return { typename FA::ReturnType{x} };
	}

	template <template <typename> class F, typename A>
	Free<F, A> make_bind(const F<Free<F, A>>& x)
	{
		return { Bind<F,A>{x} };
	}

	template <template <typename> class F, typename A>
	std::ostream& operator<<(std::ostream& os, const Free<F, A>& free)
	{
		struct Visitor {
			std::ostream& os;
			std::ostream& operator()(const Return<F, A>& r)
			{
				return os << "Return{" << r.a << "}";
			}
			std::ostream& operator()(const Bind<F, A>& b)
			{
				return os << "Bind{" << b.x; << "}";
			}
		};
		Visitor v{ os };
		return boost::apply_visitor(v, free.v);
	}

   /*
	* The Functor instance for Free is hilariously
	* short and to-the-point in Haskell:
	* 
	* instance Functor fun => Functor (Free fun) where
	*	fmap fun (Return x)	= Return (fun x)
	*	fmap fun (Bind x)		= Bind (fmap (fmap fun) x)
	*/

	template <template <typename> class F, typename A>
	struct Wrapper : Free<F, A> {};

	template <template <typename> class Wrapper>
	struct FunctorImpl {
		// The visitor struct can't be defined inside the fmap function because it
		// contains member function templates, which we use to get the compiler to
		// tell us what the template F is in the Free<F, A> inside the wrapper.
		template <typename A, typename Fn>
		struct Visitor {
			Fn& fun;

			template <template <typename> class F>
			auto operator()(const Return<F, A>& r) const
			{
				return make_return<F>(fun(r.a));
			}
			template <template <typename> class F>
			auto operator()(const Bind<F, A>& b) const
			{
				using Functor::fmap;
				return make_bind(fmap([&](const auto& f) { return fmap(fun, f); }, b.x));
			}
		};

		// fmap :: (a -> b) -> Free f a -> Free f b
		template <typename A, typename Fn>
		static Wrapper<std::result_of_t<Fn(A)>> fmap(Fn&& fun, const Wrapper<A>& f)
		{
			return boost::apply_visitor(Visitor<A, Fn>{fun}, f.v);
		}
	};

	/*
	* Make Wrapper<A>, aka Free<F, A> a Functor
	* Haskell definition:
	* instance (Functor f) => Monad (Free f) where
	*	return = Return
	*	(Bind x)   >>= f = Bind (fmap (>>= f) x)
	*	(Return r) >>= f = f r
	*/
	template <typename A>
	struct Wrapper : Free<F, A> {
		using WrappedFree = Free<F, A>;
	};

	template <template <typename> class Wrapper>
	struct MonadImpl {
		template <typename A>
		using M = Wrapper<A>;

		// pure :: a -> m a
		// pure :: a -> Free<F>
		template <typename A>
		static M<A> pure(const A& x)
		{
			return make_return<typename Wrapper<A>::WrappedFree>(x);
		}

		// bind :: Free f a -> (a -> Free f b) -> Free f b
		template <typename A, typename Fn>
		struct BindVisitor {
			using result_type = std::result_of_t<Fn(A)>;
			using B			  = typename result_type::ContainedType;
			static_assert(std::is_same<result_type, Wrapper<B>>::value, "");
			Fn&& fun;

			// bind (Bind x) f = Bind (fmap (\m -> bind m f) x)
			template <template <typename> class F>
			result_type operator()(const Bind<F,A>& b)
			{
				auto& f = this->fun;
				return make_bind(Functor::fmap(
					[f](const Free<F, A>& m) {
						return static_cast<Free<F, B>>(Monad::bind(Wrapper<A>(m), f));
					},
					b.x));
			}

			// bind (Return r) f = f r
			template <template <typename> class F>
			result_type operator()(const Return<F, A>& r)
			{
				return fun(r.a);
			}
		};

		template <typename A, typename Fn>
		static auto bind(const M<A>& x, Fn&& fun)
		{
			BindVisitor<A, Fn> v{std::forward<Fn>(fun)};
			return boost::apply_visitor(v, x.v);
		}
	};

	// Helpers for free monads

	/* 
	* liftFree - lifts a functor F into a free monad.
	* liftFree :: (Functor f) => f a -> Free f a
	* liftFree x = Bind (fmap Return x)	
	*/
	template <template <typename> class F, typename A>
	Free<F, A> liftFree(const F<A>& x)
	{
		return make_bind<F>(Functor::fmap(&make_return<F,A>, x));
	}

	/*
	* foldFree - takes a value of Free<F, A> and evaluate it in
	*            some way to yeild another monadic value.
	* foldFree :: (Monad m) => (forall x . f x -> m x) -> Free f a -> m a
	* foldFree _ (Return a) = return a
	* foldFree f (Bind as)	= f as >>= foldFree f
	*/
	template <template <typename> class M,
			  template <typename> class F,
			  typename Fun,
			  typename A>
	M<A> foldFree(Fun fun, const Free<F, A>& free)
	{
		struct Visitor {
			Fun fun;

			auto operator()(const Return<F, A>& r) { return Monad::pure<M>(r.a); }
			auto operator()(const Bind<F, A>& b)
			{
				return fun(b.x) >>= [&](const auto& x) { return foldFree<M>(fun, x); };
			}
		};
		Visitor v{fun};
		return boost::apply_visitor(v, free.v);
	}
}

namespace Functor {
	template <template <typename> class Wrapper>
	struct Functor<Wrapper,
				   std::enable_if_t<std::is_base_of<typename Wrapper<void>::WrappedFree,
													Wrapper<void>>::value>>
		: Free::FunctorImpl<Wrapper> {};
}

namespace Monad {
	template <template <typename> class Wrapper>
	struct Monad<Wrapper,
				 std::enable_if_t<std::is_base_of<typename Wrapper<void>::WrappedFree,
												  Wrapper<void>>::value>>
		: Free::MonadImpl<Wrapper> {};
}

namespace Free {
	namespace Test {
		template <typename A>
		struct NullFreeWrapper : Free<Functor::Test::NullFunctor, A> {
			using WrappedFree = Free<Functor::Test::NullFunctor, A>;
		};

		static_assert(Functor::IsFunctor<NullFreeWrapper>,
			"Functor specialization for Free wrappers works");
		static_assert(Monad::IsMonad<NullFreeWrapper>,
			"Monad specialization for Free wrappers works");
	}
}