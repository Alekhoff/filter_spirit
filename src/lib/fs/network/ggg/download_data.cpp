#include <fs/network/ggg/download_data.hpp>
#include <fs/network/url_encode.hpp>
#include <fs/network/download.hpp>
#include <fs/log/logger.hpp>

#include <future>
#include <utility>

namespace
{

constexpr auto leagues_url = "https://api.pathofexile.com/leagues";

}

namespace fs::network::ggg
{

std::future<api_league_data>
async_download_leagues(
	network_settings settings,
	download_info* info,
	const log::monitor& logger)
{
	return std::async(std::launch::async, [](network_settings settings, download_info* info, const log::monitor& logger) {
		std::vector<std::string> urls = { leagues_url };
		download_result result = download("api.pathofexile.com", urls, std::move(settings), info, logger);
		return api_league_data{std::move(result.results[0].data)};
	}, std::move(settings), info, std::ref(logger));
}

}
