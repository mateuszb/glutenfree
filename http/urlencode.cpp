#include <http/urlencode.hpp>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

using namespace http;
using namespace std;

string http::urlencode(const string& str)
{
  ostringstream oss;
  oss.fill('0');
  oss << hex;

  for (auto c : str) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      oss << c;
    } else {
      oss << uppercase << '%' << setw(2) << (unsigned int)c << nouppercase;
    }
  }
  return oss.str();
}

string http::urldecode(const string& str)
{
  ostringstream oss;

  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    if (*it == '%') {
      ++it;
      auto sub = str.substr(it - str.cbegin(), 2);
      ++it;
      oss << char(stoi(sub, nullptr, 16));      
    } else if (*it == '+') {
      oss << ' ';
    } else {
      oss << *it;
    }
  }
    return oss.str();
}
