//
// Created by adamd on 2/10/24.
//
// #include <ncurses.h>
#include <iterator>
#include <locale>
#include <nlohmann/json_fwd.hpp>
#include <string>
// #include <utility>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <codecvt>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/error.hpp>

#include <nlohmann/json.hpp>

enum { max_length = 1024 };

namespace {
  const std::filesystem::path cred_path = "zuliprc";
  const std::string prefix = "https://";
}
std::string base64_encode(const std::string& s) {
    std::string result;
    auto p = reinterpret_cast<const unsigned char*>(s.data());
    size_t len = s.size();
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (size_t i = 0; i < len; i += 3) {
        result.push_back(lookup[p[i] >> 2]);
        result.push_back(lookup[((p[i] & 0x3) << 4) | (i + 1 < len ? (p[i + 1] >> 4) : 0)]);
        result.push_back((i + 1 < len ? lookup[((p[i + 1] & 0xf) << 2) | (i + 2 < len ? (p[i + 2] >> 6) : 0)] : '='));
        result.push_back((i + 2 < len ? lookup[p[i + 2] & 0x3f] : '='));
    }
    return result;
}


auto
parseRC(std::ifstream& file) -> std::map<std::string, std::string>
{
  auto parsed = std::map<std::string, std::string>();

  std::string line;
  while (std::getline(file, line)) {
    // Ignore lines starting with '#' (comments) or empty lines
    if (!line.empty() && line[0] != '#') {
      // Parse key-value pairs
      std::istringstream iss(line);
      std::string key, value;
      if (std::getline(iss, key, '=') && std::getline(iss, value)) {
        // Trim leading and trailing whitespaces from key and value
        size_t firstNonSpace = key.find_first_not_of(" \t");
        size_t lastNonSpace = key.find_last_not_of(" \t");
        key = key.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);

        firstNonSpace = value.find_first_not_of(" \t");
        lastNonSpace = value.find_last_not_of(" \t");
        value = value.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);

        // Store key-value pair in the map
        parsed[key] = value;
      }
    }
  }

  return parsed;
}


namespace zulipclient {
  namespace targets {
    constexpr auto me = "/api/v1/users/me";
  
  }

struct Credentials
{
  std::string email_{ "" };
  std::string creds_{""};
  std::string site_{ "" };

  explicit Credentials(const std::filesystem::path& path)
  {

    std::ifstream file(path);
    auto loaded = parseRC(file);
    email_ = loaded["email"];
    creds_ = base64_encode(loaded["email"] + ":" + loaded["key"]);
    std::cout << creds_ << std::endl;
    site_ = loaded["site"];
    if (site_.find(prefix) == 0) {
      site_ = site_.substr(prefix.length());
    }

  }

  friend auto
  operator<<(std::ostream& os, const Credentials& credentials) -> std::ostream&
  {
    os << credentials.email_ << "\n" << credentials.site_;
    return os;
  }
};

class ZulipClient
{
public:
  explicit ZulipClient(Credentials& credentials): credentials_(std::move(credentials)){
    std::cout << credentials_ << std::endl;
  }
  
  auto
  getMe() -> nlohmann::json {
    auto request = nlohmann::json{};
    return request;
  }
    
private:
  const Credentials credentials_;
};
}

int
main(int argc, char* argv[])
{

  try {
    auto creds = zulipclient::Credentials(cred_path);
    auto client = zulipclient::ZulipClient(creds);
    std::cout << client.getMe() << std::endl;
    std::string host = "chat.sanezoo.com";
    std::string port = "443";
    std::string target = "/api/v1/users/me";
    std::string username = "adam.dvorsky@sanezoo.com";
    std::string token = "ogVu2XD9e8WoL97wCrFtA23mNTl6e9SR";

    // Construct the authorization header
    std::string credentials = base64_encode(username + ":" + token);
    std::string auth_header = "Basic " + credentials;

    // The io_context is required for all I/O
    boost::asio::io_context io_context;

    // Create SSL context
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::sslv23_client);

    // These objects perform our I/O
    boost::asio::ip::tcp::resolver resolver{io_context};
            boost::beast::ssl_stream<boost::beast::tcp_stream> stream(io_context, ssl_ctx);

    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Set an SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        throw boost::system::system_error{ec};
    }

    // Make the connection on the IP address we get from a lookup
    boost::beast::get_lowest_layer(stream).connect(results);

    // Perform the SSL handshake
    stream.handshake(boost::asio::ssl::stream_base::client);

    // Set up an HTTP GET request message
    boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, target, 11};
    req.set(boost::beast::http::field::host, host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(boost::beast::http::field::authorization, auth_header);

    // Send the HTTP request to the remote host
    boost::beast::http::write(stream, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    boost::beast::http::response<boost::beast::http::dynamic_body> res;

    // Receive the HTTP response
    boost::beast::http::read(stream, buffer, res);

    // Write the message to standard out
    std::cout << res << std::endl;

    // Gracefully close the SSL stream
    boost::beast::error_code ec;
    stream.shutdown(ec);
    // if(ec == boost::asio::error::eof) {
        // Rationale:
        // http://www.boost.org/doc/libs/1_66_0/libs/beast/doc/html/beast/using_https.html
        // ec = {};
    // }
    if(ec){
      std::cout << "Throwing error: " << ec << std::endl;
      throw boost::system::system_error{ec};
    }

  } catch(std::exception const& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return EXIT_FAILURE;
  }

  return 0;
}
