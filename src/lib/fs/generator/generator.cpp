#include <fs/generator/generator.hpp>
#include <fs/lang/filter_block.hpp>
#include <fs/utility/string_helpers.hpp>
#include <fs/version.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <string>
#include <string_view>
#include <sstream>

namespace fs::generator
{

std::string assemble_blocks_to_raw_filter(const std::vector<lang::filter_block>& blocks)
{
	std::stringstream ss;

	for (const lang::filter_block& block : blocks)
		block.generate(ss);

	return ss.str();
}

void prepend_metadata(const lang::item_price_metadata& metadata, std::string& raw_filter)
{
	namespace v = version;
	std::string data_to_prepend =
R"(# autogenerated by Filter Spirit - an advanced item filter generator for Path of Exile
# Write filters in an enhanced language with the ability to query item prices. Refresh whenever you want.
#
# read tutorial, browse documentation, ask questions, report bugs on: github.com/Xeverous/filter_spirit
#
# or contact directly:
#     reddit : /u/Xeverous
#     Discord: Xeverous#2151
#     in game: pathofexile.com/account/view-profile/Xeverous
#
# Generation info:
)";
	data_to_prepend +=
"#     Filter Spirit version     : " + std::to_string(v::major) + "." + std::to_string(v::minor) + "." + std::to_string(v::patch) + "\n"
"#     filter generation date    : " + utility::ptime_to_pretty_string(boost::posix_time::microsec_clock::universal_time()) + "\n"
"#     item price data downloaded: " + utility::ptime_to_pretty_string(metadata.download_date) + "\n"
"#     item price data from      : " + std::string(lang::to_string(metadata.data_source)) + "\n"
"#     item price data for league: " + metadata.league_name + "\n"
"#\n"
"# May the drops be with you.\n"
"\n";

	raw_filter = data_to_prepend + raw_filter;
}

}
