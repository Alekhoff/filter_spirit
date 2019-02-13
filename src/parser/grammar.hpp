/**
 * @file grammar type definitions
 *
 * @details This file is intended for main parser API.
 * This is the file that all parser source files should include.
 * Use BOOST_SPIRIT_DECLARE here.
 */
#pragma once
#include "parser/ast.hpp"
#include "parser/config.hpp"
#include "print/generic.hpp"
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>
#include <string>

// workaround for https://github.com/boostorg/spirit/issues/461
// or maybe the missing implementation of the most obvious substitution
template <typename T>
struct boost::spirit::x3::traits::is_substitute<T, T> : boost::mpl::true_ {};

namespace fs::parser
{

// Inherit rule ID type from these type if you want Spirit to call on_success/on_error
// this allows the same things as expr[func] in grammar definitions but does not bloat it.
// This type can be used for any rule that has x3::position_tagged attribute type - it just
// fills it with source code information (eg for later semantic analysis)
struct annotate_on_success
{
	template <typename Iterator, typename Context, typename... Types>
	void on_success(
		const Iterator& first,
		const Iterator& last,
		x3::variant<Types...>& ast,
		const Context& context)
	{
		ast.apply_visitor(x3::make_lambda_visitor<void>([&](auto& node)
		{
			this->on_success(first, last, node, context);
		}));
	}

	template <typename T, typename Iterator, typename Context>
	void on_success(
		const Iterator& first,
		const Iterator& last,
		T& ast,
		const Context& context)
	{
		position_cache_type& positions = x3::get<position_cache_tag>(context).get();
		positions.annotate(ast, first, last);
	}
};

struct error_on_error
{
	template <typename Iterator, typename Exception, typename Context>
	x3::error_handler_result on_error(
		Iterator& /* first */,
		const Iterator& /* last */,
		const Exception& ex,
		const Context& context)
	{
		const position_cache_type& positions = x3::get<position_cache_tag>(context).get();
		const iterator_type error_first = ex.where();
		const auto error_last = ++iterator_type(error_first);
		print::print_line_number_with_indication_and_texts(
			x3::get<error_stream_tag>(context).get(),
			range_type(positions.first(), positions.last()),
			range_type(error_first, error_last),
			"parse error: expected '",
			ex.which(),
			"' here");
		return x3::error_handler_result::fail;
	}
};


// rule IDs
// use multiple inheritance to add more handlers
// rules which do not have any handlers can use forward declared types
struct identifier_impl_class;
struct identifier_class                      : error_on_error, annotate_on_success {};

struct version_literal_class                 : error_on_error, annotate_on_success {};
struct version_requirement_statement_class   : error_on_error, annotate_on_success {};

struct config_param_class                    : error_on_error, annotate_on_success {};
struct config_class                          : error_on_error, annotate_on_success {};

struct boolean_class                         : error_on_error, annotate_on_success {};
struct integer_class                         : error_on_error, annotate_on_success {};
struct opacity_class                         : error_on_error, annotate_on_success {};
struct rarity_literal_class                  : error_on_error, annotate_on_success {};
struct shape_literal_class                   : error_on_error, annotate_on_success {};
struct suit_literal_class                    : error_on_error, annotate_on_success {};
struct color_literal_class                   : error_on_error, annotate_on_success {};
struct group_literal_class                   : error_on_error, annotate_on_success {};
struct group_literal_impl_class              : error_on_error, annotate_on_success {};
struct string_literal_class                  : error_on_error, annotate_on_success {};

struct object_type_expression_class          : error_on_error, annotate_on_success {};
struct array_type_expression_class           : error_on_error, annotate_on_success {};
struct type_expression_class                 : error_on_error, annotate_on_success {};

struct literal_expression_class              : error_on_error, annotate_on_success {};
struct value_expression_class                : error_on_error, annotate_on_success {};
struct array_expression_class                : error_on_error, annotate_on_success {};

struct comparison_operator_expression_class  : error_on_error, annotate_on_success {};

struct level_expression_class                : error_on_error, annotate_on_success {};
struct item_level_condition_class            : error_on_error, annotate_on_success {};
struct drop_level_condition_class            : error_on_error, annotate_on_success {};
struct condition_expression_class            : error_on_error, annotate_on_success {};

struct visibility_action_class               : error_on_error, annotate_on_success {};
struct color_expression_class                : error_on_error, annotate_on_success {};
struct border_color_action_class             : error_on_error, annotate_on_success {};
struct text_color_action_class               : error_on_error, annotate_on_success {};
struct background_color_action_class         : error_on_error, annotate_on_success {};
struct action_expression_class               : error_on_error, annotate_on_success {};

struct condition_list_class                  : error_on_error, annotate_on_success {};
struct action_list_class                     : error_on_error, annotate_on_success {};

struct constant_definition_class             : error_on_error, annotate_on_success {};
struct constant_definition_list_class        : error_on_error, annotate_on_success {};
struct rule_block_class                      : error_on_error, annotate_on_success {};
struct rule_block_list_class                 : error_on_error, annotate_on_success {};
struct filter_specification_class            : error_on_error, annotate_on_success {};

struct grammar_class                         : error_on_error, annotate_on_success {};

// ---- lowest-level tokens ----

// whitespace_type should be defined here but it has been moved to parser/config.hpp for
// dependency reasons. See config.hpp for details.
BOOST_SPIRIT_DECLARE(whitespace_type)

// all comments are ignored
using comment_type = x3::rule<struct comment_class /*, intentionally nothing */>;
BOOST_SPIRIT_DECLARE(comment_type)

// identifier has an extra intermediate rule because Spirit for (?) it's container detection reasons
// can not match identifier grammar with a struct that contains only std::string (compiles only with std::string directly)
// to workaround, we just add 1 more step with the same grammar
// https://stackoverflow.com/questions/18166958
using identifier_impl_type = x3::rule<identifier_impl_class, std::string>;
BOOST_SPIRIT_DECLARE(identifier_impl_type)
using identifier_type = x3::rule<identifier_class, ast::identifier>;
BOOST_SPIRIT_DECLARE(identifier_type)

// ---- version requirement ----

using version_literal_type = x3::rule<version_literal_class, ast::version_literal>;
BOOST_SPIRIT_DECLARE(version_literal_type)

using version_requirement_statement_type = x3::rule<version_requirement_statement_class, ast::version_literal>;
BOOST_SPIRIT_DECLARE(version_requirement_statement_type)

// ---- config ----

using config_param_type = x3::rule<config_param_class, ast::config_param>;
BOOST_SPIRIT_DECLARE(config_param_type)

using config_type = x3::rule<config_class, ast::config>;
BOOST_SPIRIT_DECLARE(config_type)

// core tokens

using boolean_type = x3::rule<boolean_class, ast::boolean_literal>;
BOOST_SPIRIT_DECLARE(boolean_type)

using integer_type = x3::rule<integer_class, ast::integer_literal>;
BOOST_SPIRIT_DECLARE(integer_type)

using opacity_type = x3::rule<opacity_class, ast::opacity_literal>;
BOOST_SPIRIT_DECLARE(opacity_type)


using rarity_literal_type = x3::rule<rarity_literal_class, ast::rarity_literal>;
BOOST_SPIRIT_DECLARE(rarity_literal_type)

using shape_literal_type = x3::rule<shape_literal_class, ast::shape_literal>;
BOOST_SPIRIT_DECLARE(shape_literal_type)

using suit_literal_type = x3::rule<suit_literal_class, ast::suit_literal>;
BOOST_SPIRIT_DECLARE(suit_literal_type)

using color_literal_type = x3::rule<color_literal_class, ast::color_literal>;
BOOST_SPIRIT_DECLARE(color_literal_type)

// same issue as with integer_literal
using group_literal_impl_type = x3::rule<group_literal_impl_class, ast::group_literal>;
BOOST_SPIRIT_DECLARE(group_literal_impl_type)
using group_literal_type = x3::rule<group_literal_class, ast::group_literal>;
BOOST_SPIRIT_DECLARE(group_literal_type)

using string_literal_type = x3::rule<string_literal_class, ast::string_literal>;
BOOST_SPIRIT_DECLARE(string_literal_type)

// ----

using object_type_expression_type = x3::rule<object_type_expression_class, ast::object_type_expression>;
BOOST_SPIRIT_DECLARE(object_type_expression_type)

using array_type_expression_type = x3::rule<array_type_expression_class, ast::array_type_expression>;
BOOST_SPIRIT_DECLARE(array_type_expression_type)

using type_expression_type = x3::rule<type_expression_class, ast::type_expression>;
BOOST_SPIRIT_DECLARE(type_expression_type)

// ----

using literal_expression_type = x3::rule<literal_expression_class, ast::literal_expression>;
BOOST_SPIRIT_DECLARE(literal_expression_type)

using value_expression_type = x3::rule<value_expression_class, ast::value_expression>;
BOOST_SPIRIT_DECLARE(value_expression_type)

using array_expression_type = x3::rule<array_expression_class, ast::array_expression>;
BOOST_SPIRIT_DECLARE(array_expression_type)

// ----

using comparison_operator_expression_type = x3::rule<comparison_operator_expression_class, ast::comparison_operator_expression>;
BOOST_SPIRIT_DECLARE(comparison_operator_expression_type)

// ----

using level_expression_type = x3::rule<level_expression_class, ast::level_expression>;
BOOST_SPIRIT_DECLARE(level_expression_type)

using item_level_condition_type = x3::rule<item_level_condition_class, ast::item_level_condition>;
BOOST_SPIRIT_DECLARE(item_level_condition_type)

using drop_level_condition_type = x3::rule<drop_level_condition_class, ast::drop_level_condition>;
BOOST_SPIRIT_DECLARE(drop_level_condition_type)

using condition_expression_type = x3::rule<condition_expression_class, ast::condition_expression>;
BOOST_SPIRIT_DECLARE(condition_expression_type)

// ----

using visibility_action_type = x3::rule<visibility_action_class, ast::visibility_action>;
BOOST_SPIRIT_DECLARE(visibility_action_type)

using color_expression_type = x3::rule<color_expression_class, ast::color_expression>;
BOOST_SPIRIT_DECLARE(color_expression_type)

using border_color_action_type = x3::rule<border_color_action_class, ast::border_color_action>;
BOOST_SPIRIT_DECLARE(border_color_action_type)

using text_color_action_type = x3::rule<text_color_action_class, ast::text_color_action>;
BOOST_SPIRIT_DECLARE(text_color_action_type)

using background_color_action_type = x3::rule<background_color_action_class, ast::background_color_action>;
BOOST_SPIRIT_DECLARE(background_color_action_type)

using action_expression_type = x3::rule<action_expression_class, ast::action_expression>;
BOOST_SPIRIT_DECLARE(action_expression_type)

// ----

using condition_list_type = x3::rule<condition_list_class, ast::condition_list>;
BOOST_SPIRIT_DECLARE(condition_list_type)

using action_list_type = x3::rule<action_list_class, ast::action_list>;
BOOST_SPIRIT_DECLARE(action_list_type)

// ----

using constant_definition_type = x3::rule<constant_definition_class, ast::constant_definition>;
BOOST_SPIRIT_DECLARE(constant_definition_type)

// ----

using constant_definition_list_type = x3::rule<constant_definition_list_class, ast::constant_definition_list>;
BOOST_SPIRIT_DECLARE(constant_definition_list_type)

using rule_block_type = x3::rule<rule_block_class, ast::rule_block>;
BOOST_SPIRIT_DECLARE(rule_block_type)

using rule_block_list_type = x3::rule<rule_block_list_class, ast::rule_block_list>;
BOOST_SPIRIT_DECLARE(rule_block_list_type)

using filter_specification_type = x3::rule<filter_specification_class, ast::filter_specification>;
BOOST_SPIRIT_DECLARE(filter_specification_type)

// the entire language grammar
using grammar_type = x3::rule<grammar_class, ast::ast_type>;
BOOST_SPIRIT_DECLARE(grammar_type)

}

namespace fs
{

parser::grammar_type grammar();
parser::skipper_type skipper();

}
