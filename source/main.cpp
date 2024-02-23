//
// Created by adamd on 2/10/24.
//
#include <ncurses.h>
#include <string>
#include <utility>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <curl/curl.h>
#include "../include/httplib.h"

// template<T&> struct Credential
// {
//   explicit
//   Credential(const T& content):content_(content){}
// private:
//   T content_;
// };

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
    // CURL* curl = curl_easy_init();
    // if (!curl) {
    //   std::cerr << "Failed to initialize libcurl." << std::endl;
    //   return;
    // }
    //
    // const char* url = (credentials_.site_ + "/api/v1/users/me").c_str();
    // curl_easy_setopt(curl, CURLOPT_URL, url);
    //
    // curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    //
    // const char* credentials = (credentials_.email_ + ":" + credentials_.key_).
    //   c_str();
    // curl_easy_setopt(curl, CURLOPT_USERPWD, credentials);
    //
    // // curl_easy_setopt(curl,
    // //                  CURLOPT_POSTFIELDS,
    // //                  "anchor=43&narrow=[{\"operand\": \"sender\", \"operator\": \"Adam Dvorsky\"}]");
    //
    // std::string response;
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    //
    // // Perform the HTTP request
    // CURLcode res = curl_easy_perform(curl);
    //
    // // Check for errors
    // if (res != CURLE_OK) {
    //   std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) <<
    //     std::endl;
    // } else {
    //   // Print the response
    //   std::cout << "Response:\n" << response << std::endl;
    // }
    //
    // // Cleanup
    // curl_easy_cleanup(curl);

    httplib::Client client(credentials_.site_);
    std::string credentials = credentials_.email_ + ":" + credentials_.key_;
    std::string auth_header = "Basic " + httplib::detail::base64_encode(credentials);
    httplib::Headers headers = {{"Authorization", auth_header}};
    auto res = client.Get("/api/v1/users/me", headers);

  }

private:
  static size_t
  WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
  {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
  }

  Credentials credentials_;

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