// Copyright (c) 2020 Hartmut Kaiser
// Copyright (c) 2020 Bita Hasheminezhad
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <phylanx/config.hpp>
#include <phylanx/execution_tree/annotation.hpp>
#include <phylanx/execution_tree/locality_annotation.hpp>
#include <phylanx/execution_tree/localities_annotation.hpp>
#include <phylanx/execution_tree/meta_annotation.hpp>
#include <phylanx/execution_tree/primitives/node_data_helpers.hpp>
#include <phylanx/execution_tree/tiling_annotations.hpp>
#include <phylanx/ir/node_data.hpp>
#include <phylanx/plugins/dist_matrixops/dist_random.hpp>
#include <phylanx/plugins/dist_matrixops/tile_calculation_helper.hpp>
#include <phylanx/util/random.hpp>

#include <hpx/include/lcos.hpp>
#include <hpx/include/naming.hpp>
#include <hpx/include/util.hpp>
#include <hpx/errors/throw_exception.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <blaze/Math.h>
#include <blaze_tensor/Math.h>

///////////////////////////////////////////////////////////////////////////////
namespace phylanx { namespace dist_matrixops { namespace primitives
{
    ///////////////////////////////////////////////////////////////////////////
    execution_tree::match_pattern_type const dist_random::match_data =
    {
        hpx::util::make_tuple("random_d", std::vector<std::string>{R"(
                random_d(
                    _1_shape,
                    _2_tile_index,
                    _3_numtiles,
                    __arg(_4_name, ""),
                    __arg(_5_tiling_type, "sym"),
                    __arg(_6_mean, 0.0),
                    __arg(_7_std, 1.0)
                )
            )"},
            &create_dist_random,
            &execution_tree::create_primitive<dist_random>, R"(
            shape, tile_index, numtiles, name, tiling_type, mean, std
            Args:

                shape (int or list of ints): overall shape of the array. It
                    only contains positive integers.
                tile_index (int): the tile index we need to generate the
                    random array for. A non-negative integer.
                numtiles (int): number of tiles of the returned array
                name (string, optional): the array given name. If not given, a
                    globally unique name will be generated.
                tiling_type (string, optional): defaults to `sym` which is a
                    balanced way of tiling among all the numtiles localities.
                    Other options are `row` or `column` tiling. For a vector
                    all these three tiling_types are the same.
                mean (float, optional): the mean value of the distribution. It
                    sets to 0.0 by default.
                std (float, optional): the standard deviation of the normal
                    distribution. It sets to 1.0 by default.

            Returns:

            A part of an array of random numbers on tile_index-th tile out of
            numtiles using the nrmal distribution)")
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        static std::atomic<std::size_t> rand_count(0);
        std::string generate_random_name(std::string&& given_name)
        {
            if (given_name.empty())
            {
                return "random_array_" + std::to_string(++rand_count);
            }

            return std::move(given_name);
        }

        std::size_t extract_num_dimensions(ir::range const& shape)
        {
            return shape.size();
        }

        std::array<std::size_t, PHYLANX_MAX_DIMENSIONS> extract_dimensions(
            ir::range const& shape)
        {
            std::array<std::size_t, PHYLANX_MAX_DIMENSIONS> result = {0};
            if (!shape.empty())
            {
                if (shape.size() == 1)
                {
                    result[0] = extract_scalar_positive_integer_value_strict(
                        *shape.begin());
                }
                else if (shape.size() == 2)
                {
                    auto elem_1 = shape.begin();
                    result[0] =
                        extract_scalar_positive_integer_value_strict(*elem_1);
                    result[1] =
                        extract_scalar_positive_integer_value_strict(*++elem_1);
                }
                else if (shape.size() == 3)
                {
                    auto elem_1 = shape.begin();
                    result[0] =
                        extract_scalar_positive_integer_value_strict(*elem_1);
                    result[1] =
                        extract_scalar_positive_integer_value_strict(*++elem_1);
                    result[2] =
                        extract_scalar_positive_integer_value_strict(*++elem_1);
                }
                else if (shape.size() == 4)
                {
                    auto elem_1 = shape.begin();
                    result[0] =
                        extract_scalar_positive_integer_value_strict(*elem_1);
                    result[1] =
                        extract_scalar_positive_integer_value_strict(*++elem_1);
                    result[2] =
                        extract_scalar_positive_integer_value_strict(*++elem_1);
                    result[3] =
                        extract_scalar_positive_integer_value_strict(*++elem_1);
                }
            }
            return result;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    dist_random::dist_random(execution_tree::primitive_arguments_type&& operands,
            std::string const& name, std::string const& codename)
      : primitive_component_base(std::move(operands), name, codename)
    {}

    ///////////////////////////////////////////////////////////////////////////
    execution_tree::primitive_argument_type dist_random::dist_random1d(
        std::size_t dim, std::uint32_t const& tile_idx,
        std::uint32_t const& numtiles, std::string&& given_name,
        double const& mean, double const& std, std::string const& name_,
        std::string const& codename_) const
    {
        using namespace execution_tree;

        std::int64_t start;
        std::size_t size;
        std::tie(start, size) =
            tile_calculation::tile_calculation_1d(tile_idx, dim, numtiles);

        std::normal_distribution<> dist(mean, std);

        tiling_information_1d tile_info(
            tiling_information_1d::tile1d_type::columns,
            tiling_span(start, start + size));
        locality_information locality_info(tile_idx, numtiles);
        annotation locality_ann = locality_info.as_annotation();

        std::string base_name =
            detail::generate_random_name(std::move(given_name));

        annotation_information ann_info(base_name, 0);    //generation 0

        auto attached_annotation =
            std::make_shared<execution_tree::annotation>(localities_annotation(
                locality_ann, tile_info.as_annotation(name_, codename_),
                ann_info, name_, codename_));

        blaze::DynamicVector<double> v(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            v[i] = dist(util::rng_);
        }

        primitive_argument_type res(std::move(v), attached_annotation);

        return std::move(res);
    }

    execution_tree::primitive_argument_type dist_random::dist_random2d(
        std::array<std::size_t, PHYLANX_MAX_DIMENSIONS> const& dims,
        std::uint32_t const& tile_idx, std::uint32_t const& numtiles,
        std::string&& given_name, std::string const& tiling_type,
        double const& mean, double const& std, std::string const& name_,
        std::string const& codename_) const
    {
        using namespace execution_tree;

        std::size_t const rows = dims[0];
        std::size_t const columns = dims[1];
        std::int64_t row_start, column_start;
        std::size_t row_size, column_size;

        std::tie(row_start, column_start, row_size, column_size) =
            tile_calculation::tile_calculation_2d(
                tile_idx, rows, columns, numtiles, tiling_type);

        std::normal_distribution<> dist(mean, std);

        tiling_information_2d tile_info(
            tiling_span(column_start, column_start + column_size),
            tiling_span(row_start, row_start + row_size));

        locality_information locality_info(tile_idx, numtiles);
        annotation locality_ann = locality_info.as_annotation();

        std::string base_name =
            detail::generate_random_name(std::move(given_name));

        annotation_information ann_info(base_name, 0);    //generation 0

        auto attached_annotation =
            std::make_shared<execution_tree::annotation>(localities_annotation(
                locality_ann, tile_info.as_annotation(name_, codename_),
                ann_info, name_, codename_));

        blaze::DynamicMatrix<double> m(rows, columns);
        for (std::size_t i = 0; i != rows; ++i)
        {
            for (std::size_t j = 0; j != columns; ++j)
            {
                m(i, j) = dist(util::rng_);
            }
        }

        primitive_argument_type res(std::move(m), attached_annotation);

        return std::move(res);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<execution_tree::primitive_argument_type> dist_random::eval(
        execution_tree::primitive_arguments_type const& operands,
        execution_tree::primitive_arguments_type const& args,
        execution_tree::eval_context ctx) const
    {
        if (operands.size() < 3 || operands.size() > 7)
        {
            HPX_THROW_EXCEPTION(hpx::bad_parameter,
                "dist_random::eval",
                generate_error_message(
                    "the random_d primitive requires at most three operand"));
        }

        if (!valid(operands[1]) || !valid(operands[2]) || !valid(operands[3]))
        {
            HPX_THROW_EXCEPTION(hpx::bad_parameter,
                "dist_random::eval",
                generate_error_message(
                    "the random_d primitive requires that the arguments "
                        "given by the operands array are valid"));
        }

        auto this_ = this->shared_from_this();
        return hpx::dataflow(hpx::launch::sync, hpx::util::unwrapping(
                [this_ = std::move(this_)](
                        execution_tree::primitive_arguments_type&& args)
                ->  execution_tree::primitive_argument_type
                {
                    using namespace execution_tree;

                    std::array<std::size_t, PHYLANX_MAX_DIMENSIONS> dims{0};
                    std::size_t numdims = 0;
                    if (is_list_operand_strict(args[0]))
                    {
                        ir::range&& shape = extract_list_value_strict(
                            std::move(args[0]), this_->name_, this_->codename_);

                        if (shape.size() > PHYLANX_MAX_DIMENSIONS)
                        {
                            HPX_THROW_EXCEPTION(hpx::bad_parameter,
                                "dist_random::eval",
                                this_->generate_error_message(
                                    "the given shape has a number of "
                                    "dimensions that is not supported"));
                        }

                        dims = detail::extract_dimensions(shape);
                        numdims = detail::extract_num_dimensions(shape);
                    }
                    else if (is_numeric_operand(args[0]))
                    {
                        numdims = 1;
                        dims[0] = extract_scalar_positive_integer_value_strict(
                            std::move(args[0]), this_->name_, this_->codename_);
                    }

                    std::uint32_t tile_idx =
                        extract_scalar_nonneg_integer_value_strict(
                            std::move(args[1]), this_->name_, this_->codename_);
                    std::uint32_t numtiles =
                        extract_scalar_positive_integer_value_strict(
                            std::move(args[2]), this_->name_, this_->codename_);
                    if (tile_idx >= numtiles)
                    {
                        HPX_THROW_EXCEPTION(hpx::bad_parameter,
                            "dist_constant::eval",
                            this_->generate_error_message(
                                "invalid tile index. Tile indices start from 0 "
                                "and should be smaller than number of tiles"));
                    }

                    std::string given_name = "";
                    if (valid(args[3]))
                    {
                        given_name = extract_string_value(std::move(args[3]),
                            this_->name_, this_->codename_);
                    }

                    // using balanced symmetric tiles
                    std::string tiling_type = "sym";
                    if (valid(args[4]))
                    {
                        tiling_type = extract_string_value(
                            std::move(args[4]), this_->name_, this_->codename_);
                        if ((tiling_type != "sym" && tiling_type != "row") &&
                            tiling_type != "column")
                        {
                            HPX_THROW_EXCEPTION(hpx::bad_parameter,
                                "dist_constant::eval",
                                this_->generate_error_message(
                                    "invalid tling_type. the tiling_type cane "
                                    "one of these: `sym`, `row` or `column`"));
                        }
                    }

                    double mean = 0.0;
                    if (valid(args[5]))
                    {
                        mean = extract_scalar_numeric_value(
                            std::move(args[5]), this_->name_, this_->codename_);
                    }

                    double std = 1.0;
                    if (valid(args[6]))
                    {
                        std = extract_scalar_numeric_value(
                            std::move(args[6]), this_->name_, this_->codename_);
                    }

                    switch (numdims)
                    {
                    case 1:
                        return this_->dist_random1d(dims[0], tile_idx, numtiles,
                            std::move(given_name), mean, std, this_->name_,
                            this_->codename_);

                    case 2:
                        return this_->dist_random2d(dims, tile_idx, numtiles,
                            std::move(given_name), tiling_type, mean, std,
                            this_->name_, this_->codename_);

                    default:
                        HPX_THROW_EXCEPTION(hpx::bad_parameter,
                            "dist_random::eval",
                            this_->generate_error_message(
                                "the given shape is of an unsupported "
                                "dimensionality"));
                    }
                }),
            execution_tree::primitives::detail::map_operands(operands,
                execution_tree::functional::value_operand{}, args, name_,
                codename_, std::move(ctx)));
    }


}}}

