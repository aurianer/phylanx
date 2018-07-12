// Copyright (c) 2017-2018 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <phylanx/config.hpp>
#include <phylanx/ast/generate_ast.hpp>
#include <phylanx/ast/node.hpp>
#include <phylanx/execution_tree/compile.hpp>
#include <phylanx/execution_tree/compiler/compiler.hpp>
#include <phylanx/execution_tree/primitives/base_primitive.hpp>

#include <hpx/include/naming.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace phylanx { namespace execution_tree
{
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        compiler::function compile(std::string const& name,
            ast::expression const& expr, compiler::function_list& snippets,
            compiler::environment& env, hpx::id_type const& default_locality)
        {
            pattern_list const& patterns = get_all_known_patterns();
            ++snippets.compile_id_;
            return compiler::compile(name, expr, snippets, env,
                compiler::generate_patterns(patterns), default_locality);
        }

        compiler::function compile(std::string const& name,
            ast::expression const& expr, compiler::function_list& snippets,
            hpx::id_type const& default_locality)
        {
            pattern_list const& patterns = get_all_known_patterns();
            compiler::environment env =
                compiler::default_environment(default_locality);

            ++snippets.compile_id_;
            return compiler::compile(name, expr, snippets, env,
                compiler::generate_patterns(patterns), default_locality);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    compiler::function compile(std::string const& name,
        std::vector<ast::expression> const& exprs,
        compiler::function_list& snippets, compiler::environment& env,
        hpx::id_type const& default_locality)
    {
        compiler::function f;
        for (auto const& expr : exprs)
        {
            // always keep objects alive that are generated by the compiler
            snippets.snippets_.emplace_back(
                detail::compile(name, expr, snippets, env, default_locality));

            // we assume that each compiled snippet needs evaluation in order
            // to resolve to the desired result
            f = snippets.snippets_.back()();
        }

        return f;
    }

    compiler::function compile(std::string const& name, std::string const& expr,
        compiler::function_list& snippets, compiler::environment& env,
        hpx::id_type const& default_locality)
    {
        return compile(
            name, ast::generate_ast(expr), snippets, env, default_locality);
    }

    compiler::function compile(std::string const& name,
        std::vector<ast::expression> const& exprs,
        compiler::function_list& snippets, hpx::id_type const& default_locality)
    {
        compiler::environment env =
            compiler::default_environment(default_locality);

        compiler::function f;
        for (auto const& expr : exprs)
        {
            // always keep objects alive that are generated by the compiler
            snippets.snippets_.emplace_back(
                detail::compile(name, expr, snippets, env, default_locality));

            // we assume that each compiled snippet needs evaluation in order
            // to resolve to the desired result
            f = snippets.snippets_.back()();
        }

        // always return the last of all generated compiler-functions
        return f;
    }

    compiler::function compile(std::string const& name, std::string const& expr,
        compiler::function_list& snippets, hpx::id_type const& default_locality)
    {
        return compile(name, ast::generate_ast(expr), snippets, default_locality);
    }

    ///////////////////////////////////////////////////////////////////////////
    compiler::function compile(std::vector<ast::expression> const& exprs,
        compiler::function_list& snippets, compiler::environment& env,
        hpx::id_type const& default_locality)
    {
        return compile("<unknown>", exprs, snippets, env, default_locality);
    }

    compiler::function compile(std::string const& expr,
        compiler::function_list& snippets, compiler::environment& env,
        hpx::id_type const& default_locality)
    {
        return compile(
            "<unknown>", ast::generate_ast(expr), snippets, env, default_locality);
    }

    compiler::function compile(std::vector<ast::expression> const& exprs,
        compiler::function_list& snippets, hpx::id_type const& default_locality)
    {
        return compile("<unknown>", exprs, snippets, default_locality);
    }

    compiler::function compile(std::string const& expr,
        compiler::function_list& snippets, hpx::id_type const& default_locality)
    {
        return compile(
            "<unknown>", ast::generate_ast(expr), snippets, default_locality);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Add the given variable to the compilation environment
    compiler::function define_variable(std::string const& codename,
        compiler::primitive_name_parts const& name_parts,
        compiler::function_list& snippets, compiler::environment& env,
        primitive_argument_type body, hpx::id_type const& default_locality)
    {
        return compiler::define_variable(
            codename, name_parts, snippets, env, body, default_locality)();
    }
}}

