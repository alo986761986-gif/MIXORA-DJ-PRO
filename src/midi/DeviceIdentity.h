#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>
namespace mixora {
// A WinMM port number can change after reconnect. Persist name+occurrence instead.
// This is a best-effort identity, NOT a USB serial number. Equal-name devices can swap.
inline std::string normalizeMidiName(std::string name) {
 auto first=name.find_first_not_of(" \t\r\n");
 if(first==std::string::npos)return "";
 auto last=name.find_last_not_of(" \t\r\n");
 name=name.substr(first,last-first+1);
 std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
 return name;
}
inline std::vector<std::string> midiIdentityKeys(const std::vector<std::string>& names) {
 std::unordered_map<std::string,unsigned> seen;
 std::vector<std::string> result;result.reserve(names.size());
 for(const auto& name:names) {
  const std::string normalized=normalizeMidiName(name);
  const unsigned occurrence=seen[normalized]++;
  // Length-prefix the name so punctuation cannot make key ambiguous.
  result.push_back(std::to_string(normalized.size())+":"+normalized+":"+std::to_string(occurrence));
 }
 return result;
}
}
