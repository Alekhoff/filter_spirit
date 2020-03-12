#include <fs/network/curl/easy.hpp>
#include <fs/network/download.hpp>
#include <fs/version.hpp>

#include <utility>

namespace
{

namespace net = fs::network;

std::size_t write_callback(char* data, std::size_t /* size */, std::size_t nmemb, void* userdata)
{
	auto& result = *reinterpret_cast<net::http_result*>(userdata);

	try {
		result.data.append(data, nmemb);
		return nmemb;
	}
	catch (const std::exception& e) {
		result.is_error = true;
		result.data = e.what();
	}
	catch (...) {
		result.is_error = true;
		result.data = "(unknown error)";
	}

	return 0; // error: accepted 0 bytes
}

int xfer_info_callback(
	void* userdata,
	curl_off_t dltotal,
	curl_off_t dlnow,
	curl_off_t /* ultotal */,
	curl_off_t /* ulnow */)
{
	auto& info = *reinterpret_cast<net::async_download_info*>(userdata);

	info.xfer_info.store(
		net::download_xfer_info{
			static_cast<std::size_t>(dltotal),
			static_cast<std::size_t>(dlnow)
		},
		std::memory_order::memory_order_release);

	return 0; // no error
}

void save_error(net::http_result& res, std::error_code ec)
{
	res.is_error = true;
	res.data = ec.message();
}

void save_error_to_all(std::vector<net::http_result>& results, std::error_code ec)
{
	for (auto& r : results)
		save_error(r, ec);
}

[[nodiscard]] bool
setup_download(
	net::curl::easy_handle& easy,
	net::network_settings settings,
	std::vector<net::http_result>& results)
{
	if (const auto ec = easy.write_callback(write_callback); ec) {
		save_error_to_all(results, ec);
		return false;
	}

	if (const auto ec = easy.follow_redirects(true); ec) {
		save_error_to_all(results, ec);
		return false;
	}

	if (const auto ec = easy.user_agent(
		(std::string("Filter Spirit/") + fs::version::version_string()).c_str());
		ec)
	{
		save_error_to_all(results, ec);
		return false;
	}

	if (const auto ec = easy.timeout_ms(settings.timeout_milliseconds); ec) {
		save_error_to_all(results, ec);
		return false;
	}

	if (const auto ec = easy.ssl_verify_peer(settings.ssl_verify_peer); ec) {
		save_error_to_all(results, ec);
		return false;
	}

	if (const auto ec = easy.ssl_verify_host(settings.ssl_verify_host); ec) {
		save_error_to_all(results, ec);
		return false;
	}

	if (settings.ca_info_path != nullptr) {
		if (const auto ec = easy.ca_info(settings.ca_info_path); ec) {
			save_error_to_all(results, ec);
			return false;
		}
	}

	if (settings.proxy != nullptr) {
		if (const auto ec = easy.proxy(settings.proxy); ec) {
			save_error_to_all(results, ec);
			return false;
		}
	}

	return true;
}

net::download_result
async_download_impl(
	std::vector<std::string> urls,
	net::network_settings settings,
	net::async_download_info& info)
{
	std::vector<net::http_result> results(urls.size());
	net::curl::easy_handle easy;

	if (!setup_download(easy, settings, results))
		return net::download_result{std::move(results)};

	if (auto ec = easy.xferinfo_callback(xfer_info_callback); ec) {
		save_error_to_all(results, ec);
		return net::download_result{std::move(results)};
	}

	if (auto ec = easy.xferinfo_callback_data(&info); ec) {
		save_error_to_all(results, ec);
		return net::download_result{std::move(results)};
	}

	for (std::size_t i = 0; i < urls.size(); ++i) {
		info.requests_complete.store(i, std::memory_order::memory_order_release);

		if (const auto ec = easy.write_callback_data(&results[i]); ec) {
			save_error(results[i], ec);
			continue;
		}

		if (const auto ec = easy.url(urls[i].c_str()); ec) {
			save_error(results[i], ec);
			continue;
		}

		if (const auto ec = easy.perform(); ec) {
			save_error(results[i], ec);
			continue;
		}
	}

	info.requests_complete.store(urls.size(), std::memory_order::memory_order_release);
	return net::download_result{std::move(results)};
}

} // namespace

namespace fs::network {

download_result
download(
	const std::vector<std::string>& urls,
	network_settings settings)
{
	std::vector<http_result> results(urls.size());
	curl::easy_handle easy;

	if (!setup_download(easy, settings, results))
		return download_result{std::move(results)};

	for (std::size_t i = 0; i < urls.size(); ++i) {
		if (const auto ec = easy.write_callback_data(&results[i]); ec) {
			save_error(results[i], ec);
			continue;
		}

		if (const auto ec = easy.url(urls[i].c_str()); ec) {
			save_error(results[i], ec);
			continue;
		}

		if (const auto ec = easy.perform(); ec) {
			save_error(results[i], ec);
			continue;
		}
	}

	return download_result{std::move(results)};
}

std::future<download_result>
async_dowload(
	std::vector<std::string> urls,
	network_settings settings,
	async_download_info& info)
{
	return std::async(
		std::launch::async,
		async_download_impl,
		std::move(urls),
		settings,
		std::ref(info));
}

}