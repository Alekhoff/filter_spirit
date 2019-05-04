#include "fst/common/print_type.hpp"
#include "fst/common/test_fixtures.hpp"
#include "fst/common/string_operations.hpp"

#include "fs/compiler/compiler.hpp"
#include "fs/log/buffered_logger.hpp"
#include "fs/log/utility.hpp"

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <string>
#include <string_view>
#include <functional>

namespace ut = boost::unit_test;
namespace tt = boost::test_tools;

namespace fst
{

BOOST_FIXTURE_TEST_SUITE(compiler_suite, compiler_fixture)

	BOOST_AUTO_TEST_CASE(minimal_input_resolve_constants)
	{
		const fs::parser::parse_success_data parse_data = parse(minimal_input());
		const std::variant<fs::lang::constants_map, fs::compiler::error::error_variant> map_or_error =
			resolve_constants(parse_data.ast.constant_definitions);
		BOOST_TEST_REQUIRE(std::holds_alternative<fs::lang::constants_map>(map_or_error));
		const auto& map = std::get<fs::lang::constants_map>(map_or_error);
		BOOST_TEST(map.empty());
	}

	/*
	 * tests that check for correct compiler errors for common mistakes
	 *
	 * - parse should succeed
	 * - compilation step(s) should fail
	 *   - check the the error variant holds correct error type
	 *   - check that error information points to relevant place in input
	 */
	class compiler_error_fixture : public compiler_fixture
	{
	protected:
		template <typename T>
		T expect_error_when_resolving_constants(const std::vector<fs::parser::ast::constant_definition>& defs)
		{
			const std::variant<fs::lang::constants_map, fs::compiler::error::error_variant> map_or_error =
				resolve_constants(defs);
			BOOST_TEST_REQUIRE(std::holds_alternative<fs::compiler::error::error_variant>(map_or_error));
			const auto& error = std::get<fs::compiler::error::error_variant>(map_or_error);
			BOOST_TEST_REQUIRE(std::holds_alternative<T>(error));
			return std::get<T>(error);
		}
	};

	BOOST_FIXTURE_TEST_SUITE(compiler_error_suite, compiler_error_fixture, * ut::depends_on("compiler_suite/minimal_input_resolve_constants"))
		namespace error = fs::compiler::error;

		BOOST_AUTO_TEST_CASE(name_already_exists)
		{
			const std::string input_str = minimal_input() + R"(
const some_var = 0
const xyz = 1
const some_other_var = 2
const xyz = 3
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::name_already_exists>(parse_data.ast.constant_definitions);

			const std::string_view pattern = "xyz";
			const std::string_view expected_place_of_original_name = search(input, pattern);
			const std::string_view reported_place_of_original_name = parse_data.lookup_data.position_of(error_desc.place_of_original_name);
			BOOST_TEST(compare_ranges(expected_place_of_original_name, reported_place_of_original_name, input));

			const char *const search_first = expected_place_of_original_name.data() + expected_place_of_original_name.size();
			const char *const search_last  = input.data() + input.size();
			const std::string_view expected_place_of_duplicated_name = search(fs::log::make_string_view(search_first, search_last), pattern);
			const std::string_view reported_place_of_duplicated_name = parse_data.lookup_data.position_of(error_desc.place_of_duplicated_name);
			BOOST_TEST(compare_ranges(expected_place_of_duplicated_name, reported_place_of_duplicated_name, input));
		}

		BOOST_AUTO_TEST_CASE(no_such_name)
		{
			const std::string input_str = minimal_input() + R"(
const abc = 0
const xyz = non_existent_obj
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::no_such_name>(parse_data.ast.constant_definitions);

			const std::string_view expected_place_of_name = search(input, "non_existent_obj");
			const std::string_view reported_place_of_name = parse_data.lookup_data.position_of(error_desc.place_of_name);
			BOOST_TEST(compare_ranges(expected_place_of_name, reported_place_of_name, input));
		}

		BOOST_AUTO_TEST_CASE(no_such_function)
		{
			const std::string input_str = minimal_input() + R"(
const abc = non_existent_func(0)
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::no_such_function>(parse_data.ast.constant_definitions);

			const std::string_view expected_place_of_name = search(input, "non_existent_func");
			const std::string_view reported_place_of_name = parse_data.lookup_data.position_of(error_desc.place_of_name);
			BOOST_TEST(compare_ranges(expected_place_of_name, reported_place_of_name, input));
		}

		BOOST_AUTO_TEST_CASE(invalid_amount_of_arguments)
		{
			const std::string input_str = minimal_input() + R"(
const color = Path(11, 22)
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::invalid_amount_of_arguments>(parse_data.ast.constant_definitions);

			const std::string_view expected_place_of_arguments = search(input, "11, 22");
			const std::string_view reported_place_of_arguments = parse_data.lookup_data.position_of(error_desc.place_of_arguments);
			BOOST_TEST(compare_ranges(expected_place_of_arguments, reported_place_of_arguments, input));
		}

		BOOST_AUTO_TEST_CASE(type_mismatch)
		{
			const std::string input_str = minimal_input() + R"(
const color = RGB(11, 22, "33")
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::type_mismatch>(parse_data.ast.constant_definitions);

			BOOST_TEST(error_desc.expected_type == +fs::lang::object_type::integer);
			BOOST_TEST(error_desc.actual_type == +fs::lang::object_type::string);

			const std::string_view expected_place_of_expression = search(input, "\"33\"");
			const std::string_view reported_place_of_expression = parse_data.lookup_data.position_of(error_desc.place_of_expression);
			BOOST_TEST(compare_ranges(expected_place_of_expression, reported_place_of_expression, input));
		}

		BOOST_AUTO_TEST_CASE(empty_socket_group)
		{
			const std::string input_str = minimal_input() + R"(
const group = Group("")
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::empty_socket_group>(parse_data.ast.constant_definitions);

			const std::string_view expected_place_of_expression = search(input, "\"\"");
			const std::string_view reported_place_of_expression = parse_data.lookup_data.position_of(error_desc.place_of_socket_group_string);
			BOOST_TEST(compare_ranges(expected_place_of_expression, reported_place_of_expression, input));
		}

		BOOST_AUTO_TEST_CASE(illegal_characters_in_socket_group)
		{
			const std::string input_str = minimal_input() + R"(
const group = Group("GBAC")
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::illegal_characters_in_socket_group>(parse_data.ast.constant_definitions);

			const std::string_view expected_place_of_expression = search(input, "\"GBAC\"");
			const std::string_view reported_place_of_expression = parse_data.lookup_data.position_of(error_desc.place_of_socket_group_string);
			BOOST_TEST(compare_ranges(expected_place_of_expression, reported_place_of_expression, input));
		}

		BOOST_AUTO_TEST_CASE(invalid_socket_group)
		{
			const std::string input_str = minimal_input() + R"(
const group = Group("RRRRRRR") # 7 characters
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::invalid_socket_group>(parse_data.ast.constant_definitions);

			const std::string_view expected_place_of_expression = search(input, "\"RRRRRRR\"");
			const std::string_view reported_place_of_expression = parse_data.lookup_data.position_of(error_desc.place_of_socket_group_string);
			BOOST_TEST(compare_ranges(expected_place_of_expression, reported_place_of_expression, input));
		}

		BOOST_AUTO_TEST_CASE(invalid_minimap_icon_size)
		{
			const std::string input_str = minimal_input() + R"(
const icon = MinimapIcon(5, green, circle) # size must be in range [0, 2]
)";
			const std::string_view input = input_str;
			const fs::parser::parse_success_data parse_data = parse(input);
			const auto error_desc = expect_error_when_resolving_constants<error::invalid_minimap_icon_size>(parse_data.ast.constant_definitions);

			BOOST_TEST(error_desc.requested_size == 5);

			const std::string_view expected_place_of_expression = search(input, "5");
			const std::string_view reported_place_of_expression = parse_data.lookup_data.position_of(error_desc.place_of_size_argument);
			BOOST_TEST(compare_ranges(expected_place_of_expression, reported_place_of_expression, input));
		}

	BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

}
