// Copyright (c) 2020 Hartmut Kaiser
// Copyright (c) 2020 Bita Hasheminezhad
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(PHYLANX_PRIMITIVES_DIST_RANDOM)
#define PHYLANX_PRIMITIVES_DIST_RANDOM

#include <phylanx/config.hpp>
#include <phylanx/execution_tree/primitives/base_primitive.hpp>
#include <phylanx/execution_tree/primitives/node_data_helpers.hpp>
#include <phylanx/execution_tree/primitives/primitive_component_base.hpp>
#include <phylanx/util/random.hpp>

#include <hpx/lcos/future.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <blaze/Math.h>

namespace phylanx { namespace dist_matrixops { namespace primitives
{
    using distribution_parameters_type =
        std::tuple<std::string, int, double, double>;

    class dist_random
      : public execution_tree::primitives::primitive_component_base
      , public std::enable_shared_from_this<dist_random>
    {
    public:
        static execution_tree::match_pattern_type const match_data;

        dist_random() = default;

        dist_random(execution_tree::primitive_arguments_type&& operands,
            std::string const& name, std::string const& codename);

    protected:
        hpx::future<execution_tree::primitive_argument_type> eval(
            execution_tree::primitive_arguments_type const& operands,
            execution_tree::primitive_arguments_type const& args,
            execution_tree::eval_context ctx) const override;

        execution_tree::primitive_argument_type dist_random1d(std::size_t dim,
            std::uint32_t const& tile_idx, std::uint32_t const& numtiles,
            std::string&& given_name, double const& mean, double const& std,
            std::string const& name_, std::string const& codename_) const;
        execution_tree::primitive_argument_type dist_random2d(
            std::array<std::size_t, PHYLANX_MAX_DIMENSIONS> const& dims,
            std::uint32_t const& tile_idx, std::uint32_t const& numtiles,
            std::string&& given_name, std::string const& tiling_type,
            double const& mean, double const& std, std::string const& name_,
            std::string const& codename_) const;
    };

    inline execution_tree::primitive create_dist_random(hpx::id_type const& locality,
        execution_tree::primitive_arguments_type&& operands,
        std::string const& name = "", std::string const& codename = "")
    {
        return create_primitive_component(
            locality, "random_d", std::move(operands), name, codename);
    }
}}}

#endif
