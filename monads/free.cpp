// monads.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// 
// https://toby-allsopp.github.io/2016/10/12/free-monads-in-cpp.html

#include "free_monad.h"
#include "functor.h"
#include "list_monad.h"
#include "monad.h"

// #include "catch.hpp" -- I haven't installed catch

#include <boost/variant.hpp>

#include <functional>
#include <string>
#include <iostream>

/*
* Unit
* Haskell type () aka "unit" is a type with one value ()
*/ 
using unit = std::tuple<>;
std::ostream& operator<<(std::ostream& os, unit) { return os << "unit{}"; }

/*
* Identity
* id :: a -> a
* id x = x
*/
template <typename A>
A id(A a)
{
    return a;
}

/*
* Function Composition
* compose :: (b -> c) -> (a -> b) -> a -> c
* compose f g = \x -> f (g x)
*/
template <typename F, typename G>
auto compose(F&& f, G&& g)
{
    return [=](auto&& x) { return f(g(std::forward<decltype(x)>(x))); };
}

/*
* Language Definition:
* In our example there are only two operations: Read, which takes the
* value or values so far written and does something, and Write, which
* “writes” a value and then does something.
* 
* data Prog a =
*      Read (Int -> a)
*    | Write Int (() -> a)
*/
template <typename Next>
struct Read {
    std::function<Next(int)> next;
};
template <typename Next>
struct Write {
    int x;
    std::function<Next(unit)> next;
};

template <typename Next>
struct Prog {
    boost::variant<Read<Next>, Write<Next>> v;
};

// std::result_of_t is decrepcated use std::invoke_result_t instead
template <typename F>
Prog <std::invoke_result_t<F,int>> make_read(F next)
{
    return { Read<std::invoke_result_t<F,int>> };
}

template <typename F>
Prog <std::invoke_result_t<F, unit>> make_write(int x, F next)
{
    return { Write<std::invoke_result_t<F,unit>>{x, next} };
}

// instance Functor Prog where
namespace Functor {

}

int main()
{
    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
