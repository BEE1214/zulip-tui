//
// Created by adamd on 2/10/24.
//
// #include <ncurses.h>
#include <boost/asio/registered_buffer.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <iterator>
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

namespace ssl = boost::asio::ssl;
typedef ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

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
    std::cout << "Credentials: " << credentials << std::endl;

    boost::asio::io_context io_context;
    ssl::context ctx{ssl::context::sslv23_client};

    // Create an SSL socket
    ssl_socket socket(io_context, ctx);
    // ssl::stream<boost::asio::ip::tcp::socket> socket(io_context, ctx);

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
    boost::beast::detail::base64::encode(dest_char, &credentials,std::size(credentials));
    std::stringstream request_stream;
    request_stream << "GET " << path << " HTTP/1.1\r\n"
                   << "Host: " << server << "\r\n"
                   << "Authorization: Basic " << std::string(dest_char)  << "\r\n"
                   << "Accept: */*\r\n"
                   << "Connection: close\r\n\r\n";

    // Send the request
    std::cout << "Sending request: " << request_stream.str() << std::endl;
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

  // initscr();			/* Start curses mode 		  */
  // printw("Hello World !!!");	/* Print Hello World		  */
  // refresh();			/* Print it on to the real screen */
  // getch();			/* Wait for user input */
  // endwin();			/* End curses mode		  */

  return 0;
}
