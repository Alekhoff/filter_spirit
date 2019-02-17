/**
 * @file AST definitions
 *
 * @brief This file is intended for type definitions that will be used for AST
 */
#pragma once
#include "lang/types.hpp"
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <utility>
#include <string>

/*
 * Warning: std::optional and std::tuple are not yet supported,
 * use Boost counterparts instead.
 *
 * Do not use std::variant, inherit from x3::variant instead.
 *
 * https://github.com/boostorg/spirit/issues/270
 */

namespace fs::parser::ast
{

namespace x3 = boost::spirit::x3;

// ---- whitespace ----

// all whitespace and comments are ignored
// no point in saving them, hence nothing here

// ---- fundamental tokens ----

struct identifier : x3::position_tagged
{
	identifier& operator=(std::string str)
	{
		value = std::move(str);
		return *this;
	}

	const std::string& get_value() const { return value; }

	std::string value;
};

// ---- version requirement ----

struct version_literal : x3::position_tagged
{
	int major = 0;
	int minor = 0;
	int patch = 0;
};

struct version_requirement_statement : x3::position_tagged
{
	version_requirement_statement& operator=(version_literal vl)
	{
		min_required_version = vl;
		return *this;
	}

	const version_literal& get_value() const { return min_required_version; }

	version_literal min_required_version;
};

// ---- config ----

struct config_param : x3::position_tagged
{
	identifier name;
	bool enabled;
	std::vector<config_param> child_params; // yo dawg, I heard you like recursion...
};

struct config : x3::position_tagged
{
	std::vector<config_param> params;
};

// ---- literal types ----

struct integer_literal : x3::position_tagged
{
	integer_literal& operator=(int n)
	{
		value = n;
		return *this;
	}

	int get_value() const { return value; }

	int value;
};

struct string_literal : std::string, x3::position_tagged
{
};

struct boolean_literal : x3::position_tagged
{
	boolean_literal& operator=(bool b)
	{
		value = b;
		return *this;
	}

	bool get_value() const { return value; }

	bool value;
};

struct rarity_literal : x3::position_tagged
{
	rarity_literal& operator=(lang::rarity r)
	{
		value = r;
		return *this;
	}

	lang::rarity get_value() const { return value; }

	lang::rarity value;
};

struct shape_literal : x3::position_tagged
{
	shape_literal& operator=(lang::shape s)
	{
		value = s;
		return *this;
	}

	lang::shape get_value() const { return value; }

	lang::shape value;
};

struct suit_literal : x3::position_tagged
{
	suit_literal& operator=(lang::suit s)
	{
		value = s;
		return *this;
	}

	lang::suit get_value() const { return value; }

	lang::suit value;
};

// ---- expressions ----

struct literal_expression : x3::variant<
		integer_literal,
		string_literal,
		boolean_literal,
		rarity_literal,
		shape_literal,
		suit_literal
	>, x3::position_tagged
{
	using base_type::base_type;
	using base_type::operator=;
};

struct function_call : x3::position_tagged
{
	identifier type_name;
	std::vector<struct value_expression> arguments;
};

struct array_expression : std::vector<struct value_expression>, x3::position_tagged
{
};

struct value_expression : x3::variant<
		literal_expression,
		array_expression,
		function_call,
		identifier
	>, x3::position_tagged
{
	using base_type::base_type;
	using base_type::operator=;
};

// ---- definitions ----

struct constant_definition : x3::position_tagged
{
	identifier name;
	value_expression value;
};

// ---- rules ----

struct comparison_operator_expression : x3::position_tagged
{
	comparison_operator_expression& operator=(lang::comparison_type ct)
	{
		value = ct;
		return *this;
	}

	lang::comparison_type get_value() const { return value; }

	lang::comparison_type value;
};

struct comparison_condition
{
	lang::comparison_condition_property property;
	comparison_operator_expression comparison_type;
	value_expression value;
};

struct string_condition
{
	lang::string_condition_property property;
	value_expression value;
};

struct condition : x3::variant<
		comparison_condition,
		string_condition
	>, x3::position_tagged
{
	using base_type::base_type;
	using base_type::operator=;
};

struct action : x3::position_tagged
{
	lang::action_type action_type;
	value_expression value;
};

// ---- filter structure ----

struct visibility_statement : x3::position_tagged
{
	visibility_statement& operator=(bool b)
	{
		show = b;
		return *this;
	}

	bool get_value() const { return show; }

	bool show;
};

// rule_block defined earlier than statement due to circular dependency
// note: we could use x3::forward_ast but it would have a worse memory layout
struct rule_block : x3::position_tagged
{
	std::vector<condition> conditions;
	std::vector<struct statement> statements;
};

struct statement : x3::variant<
		action,
		visibility_statement,
		rule_block
	>, x3::position_tagged
{
	using base_type::base_type;
	using base_type::operator=;
};

struct filter_structure : x3::position_tagged
{
	version_requirement_statement version_data;
	config config;
	std::vector<constant_definition> constant_definitions;
	std::vector<statement> statements;
};

using ast_type = filter_structure;

}
