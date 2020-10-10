// Copyright (c) Kay Stenschke
// Licensed under the MIT License - https://opensource.org/licenses/MIT

#include <string>
#include <vector>
#include <sstream>

namespace html2md {

int ReplaceAll(std::string *haystack,
               const std::string &needle,
               const std::string &replacement) {
  // Get first occurrence
  size_t pos = (*haystack).find(needle);

  int amount_replaced = 0;

  // Repeat till end is reached
  while (pos != std::string::npos) {
    // Replace this occurrence of sub string
    (*haystack).replace(pos, needle.size(), replacement);

    // Get the next occurrence from the current position
    pos = (*haystack).find(needle, pos + replacement.size());

    amount_replaced++;
  }

  return amount_replaced;
}

// Split given string by given character delimiter into vector of strings
std::vector<std::string> Explode(std::string const &str,
                                         char delimiter) {
  std::vector<std::string> result;
  std::istringstream iss(str);

  for (std::string token; std::getline(iss, token, delimiter);)
    result.push_back(std::move(token));

  return result;
}

std::string Html2Text(std::string html) {
  ReplaceAll(&html, "\t", " ");

  std::string md;

  bool is_in_tag = false;
  bool is_closing_tag = false;
  bool is_in_child_of_noscript_tag = false;

  char prev_ch, preprev_ch;

  std::string current_tag;

  for (char ch : html) {
    if (!is_in_tag && ch == '<') {
      is_in_tag = true;
      current_tag = "";

      if (!md.empty()) {
        prev_ch = md[md.length() - 1];

        if (prev_ch != ' ' && prev_ch != '\n')
          md += ' ';
      }

      continue;
    }

    auto md_len = md.length();

    if (is_in_tag) {
      if (ch == '/' && current_tag.empty()) {
        is_closing_tag = true;

        continue;
      }

      if (ch == '>') {
        is_in_tag = false;

        current_tag = Explode(current_tag, ' ')[0];

        if (is_closing_tag) {
          is_closing_tag = false;

          if (current_tag == "noscript") {
            is_in_child_of_noscript_tag = false;
          } else if (md_len > 0) {
            if (current_tag == "span") {
              md += "\n";
            } else if (current_tag == "title") {
              md += "\n\n";
            }
          }
        } else if (current_tag == "noscript") {
          is_in_child_of_noscript_tag = true;
        }

        continue;
      }

      current_tag += ch;
    }

    if (!is_in_tag) {
      if (is_in_child_of_noscript_tag
          || current_tag == "link"
          || current_tag == "meta"
          || current_tag == "script") continue;

      prev_ch = md_len > 0 ? md[md_len - 1] : '.';
      preprev_ch = md_len > 1 ? md[md_len - 2] : '.';

      if (ch == ' ' && (prev_ch == ' ' || prev_ch == '\n'))
        continue;  // prevent more than two consecutive spaces

      if (ch == '\n' && prev_ch == '\n' && preprev_ch == '\n')
        continue;  // prevent more than two consecutive newlines

      md += ch;
    }

//    if (md_len > 400) break;
  }

  return md;
}

}  // namespace html2md
