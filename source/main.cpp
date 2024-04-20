//
// Created by adamd on 2/10/24.
//
// #include <ncurses.h>
#include <boost/asio/registered_buffer.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <iterator>
#include <locale>
#include <openssl/ssl.h>
#include <string>
#include <utility>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <curl/curl.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <codecvt>

namespace ssl = boost::asio::ssl;
typedef ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
// typedef ssl::stream<boost::beast::tcp_stream> ssl_socket;

enum { max_length = 1024 };

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

struct Credentials
{
  std::string email_{ "" };
  std::string key_{ "" };
  std::string site_{ "" };

  inline auto
  load(const std::filesystem::path& path) -> void
  {

    std::ifstream file(path);
    auto loaded = parseRC(file);
    email_ = loaded["email"];
    key_ = loaded["key"];
    site_ = loaded["site"];

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
  explicit
  ZulipClient(const std::filesystem::path& path)
  {
    credentials_.load(path);
    std::cout << credentials_ << std::endl;
  }


  auto
  getMesseges() -> void
  {
    // const std::string server = credentials_.site_;
    const std::string server = "chat.sanezoo.com";
    // const std::string path = "/api/v1/users/me";
    // const std::string path = "/";
    const std::string path = "/api/v1/users/me";
    const std::string bot_email_address = credentials_.email_;
    const std::string bot_api_key = credentials_.key_;
    std::string credentials = bot_email_address + ":" + bot_api_key;

    boost::asio::io_context io_context;
    // ssl::context ctx{ssl::context::tlsv12_client};
    ssl::context ctx{ssl::context::sslv23_client};
    // ctx.set_verify_mode(ssl::verify_peer);

    // Create an SSL socket
    ssl_socket socket(io_context, ctx);

    // Resolve the host
    boost::asio::ip::tcp::resolver resolver(io_context);
    std::cout << "Connecting to server:" << std::endl;
    auto resolved = resolver.resolve(server, "https");
    boost::asio::connect(socket.lowest_layer(), resolved);
    std::cout << "Connected to server:" << std::endl;

    // Perform SSL handshake
    std::cout << "Performing handshake" << std::endl;
    socket.handshake(ssl_socket::client);
    std::cout << "Handshake succesfull" << std::endl;

    char* dest_char;
    dest_char = new char[std::size(credentials)];
    std::wstring_convert<std::codecvt_utf8<char>> converter;
    // boost::beast::detail::base64::encode(dest_char, &credentials,std::size(credentials));
    boost::beast::detail::base64::encode(dest_char, &credentials,std::size(credentials));
    std::stringstream request_stream;
    request_stream << "GET " << path << " HTTP/1.1\r\n"
                   << "Host: " << server << "\r\n"
                  //  << "Authorization: Basic " << converter.to_bytes(*credentials.c_str())  << "\r\n"
                   << "Authorization: Basic " << std::string(dest_char)  << "\r\n"
                   << "Accept: */*\r\n"
                   << "Connection: close\r\n\r\n";

    // Send the request
    // std::cout << converter.to_bytes(*credentials.c_str()) << std::endl;
    auto tmp_utf =  converter.to_bytes(*credentials.c_str());
    for (size_t i = 0; i < std::size(tmp_utf); i++) {
      std::cout << tmp_utf[i];
    }
    std::cout << "Sending request: \n" << request_stream.str() << std::endl;
    boost::asio::write(socket, boost::asio::buffer(request_stream.str()));

    // Read the response
    std::cout << "Reading response" << std::endl;
    // boost::asio::streambuf response;
    std::string response;
    boost::system::error_code ec;
    do {
      char buf[max_length];
      std::cout << "Reading some" << std::endl;
      size_t bytes_transferred = socket.read_some(boost::asio::buffer(buf), ec);
      if (!ec){
        response.append(buf, buf + bytes_transferred);
        std::cout << response << std::endl;
      }
    } while (!ec);
    // while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), ec))
      // ;
    if (ec != boost::asio::error::eof)
      socket.shutdown();

    std::cout << &response << std::endl;
  
    delete [] dest_char;

  }
  
  auto getUsers() -> void {
    
    //namespace beast = boost::beast; // from <boost/beast.hpp>
    //namespace http = beast::http;   // from <boost/beast/http.hpp>
    //namespace net = boost::asio;    // from <boost/asio.hpp>
    //namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
    //using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
    //try
    //  {
    //      // Check command line arguments.
    //      if(argc != 4 && argc != 5)
    //      {
    //          std::cerr <<
    //              "Usage: http-client-sync-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
    //              "Example:\n" <<
    //              "    http-client-sync-ssl www.example.com 443 /\n" <<
    //              "    http-client-sync-ssl www.example.com 443 / 1.0\n";
    //          return EXIT_FAILURE;
    //      }
    //      auto const host = "chat.sanezoo.com";
    //      auto const port = 443;
    //      auto const target = argv[3];
    //      int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    //      // The io_context is required for all I/O
    //      net::io_context ioc;

    //      // The SSL context is required, and holds certificates
    //      ssl::context ctx(ssl::context::tlsv12_client);

    //      // This holds the root certificate used for verification
    //      load_root_certificates(ctx);

    //      // Verify the remote server's certificate
    //      ctx.set_verify_mode(ssl::verify_peer);

    //          // These objects perform our I/O
    //      tcp::resolver resolver(ioc);
    //      beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

    //      // Set SNI Hostname (many hosts need this to handshake successfully)
    //      if(! SSL_set_tlsext_host_name(stream.native_handle(), host))
    //      {
    //          beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
    //          throw beast::system_error{ec};
    //      }

    //      // Look up the domain name
    //      auto const results = resolver.resolve(host, port);

    //      // Make the connection on the IP address we get from a lookup
    //      beast::get_lowest_layer(stream).connect(results);

    //      // Perform the SSL handshake
    //      stream.handshake(ssl::stream_base::client);

    //      // Set up an HTTP GET request message
    //      http::request<http::string_body> req{http::verb::get, target, version};
    //      req.set(http::field::host, host);
    //      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    //      // Send the HTTP request to the remote host
    //      http::write(stream, req);

    //      // This buffer is used for reading and must be persisted
    //      beast::flat_buffer buffer;

    //      // Declare a container to hold the response
    //      http::response<http::dynamic_body> res;

    //      // Receive the HTTP response
    //      http::read(stream, buffer, res);

    //      // Write the message to standard out
    //      std::cout << res << std::endl;

    //      // Gracefully close the stream
    //      beast::error_code ec;
    //      stream.shutdown(ec);
    //      if(ec == net::error::eof)
    //      {
    //          // Rationale:
    //          // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
    //          ec = {};
    //      }
    //      if(ec)
    //          throw beast::system_error{ec};

    //      // If we get here then the connection is closed gracefully
    //  }
    //  catch(std::exception const& e)
    //  {
    //      std::cerr << "Error: " << e.what() << std::endl;
    //      return EXIT_FAILURE;
    //  }                               //
  }


private:
  Credentials credentials_;
  // ssl_socket socket_;
  char request_[max_length];
  char reply_[max_length];

};

int
main(int argc, char* argv[])
{
  auto client = ZulipClient("/home/adamd/Projects/personal/zulip-tui/zuliprc");
  client.getMesseges();
  // client.getUsers();

  // initscr();			/* Start curses mode 		  */
  // printw("Hello World !!!");	/* Print Hello World		  */
  // refresh();			/* Print it on to the real screen */
  // getch();			/* Wait for user input */
  // endwin();			/* End curses mode		  */

  return 0;
}
