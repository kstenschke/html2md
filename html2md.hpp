// Copyright (c) Kay Stenschke
// Licensed under the MIT License - https://opensource.org/licenses/MIT

#ifndef HTML2MD_HPP_
#define HTML2MD_HPP_

#include <string>
#include <sstream>
#include <utility>
#include <vector>

namespace html2md {

class Converter {
 public:
  static std::string Convert(const std::string &html) {
    auto *instance = new Converter();

    return instance->Convert2Md(html);
  }

 private:
  bool is_in_tag_ = false;
  bool is_closing_tag_ = false;
  bool is_in_child_of_noscript_tag_ = false;

  char prev_ch_, prev_prev_ch_;

  std::string current_tag_;

  u_int16_t chars_in_curr_line = 0;

  std::string md_;

  Converter() = default;

  int ReplaceAll(std::string *haystack,
                 const std::string &needle,
                 const std::string &replacement) {
    // Get first occurrence
    size_t pos = (*haystack).find(needle);

    int amount_replaced = 0;

    // Repeat till end is reached
    while (pos!=std::string::npos) {
      // Replace this occurrence of sub string
      (*haystack).replace(pos, needle.size(), replacement);

      // Get the next occurrence from the current position
      pos = (*haystack).find(needle, pos + replacement.size());

      amount_replaced++;
    }

    return amount_replaced;
  }

  // Split given string by given character delimiter into vector of strings
  std::vector<std::string> Explode(std::string const &str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream iss(str);

    for (std::string token; std::getline(iss, token, delimiter);)
      result.push_back(std::move(token));

    return result;
  }

  // Repeat given string given amount
  std::string Repeat(const std::string &str, u_int16_t amount) {
    std::string out;

    for (u_int16_t i = 0; i < amount; i++) {
      out += str;
    }

    return out;
  }

  void TurnLineIntoHeader1(std::string *md, u_int16_t *chars_in_line) {
    *md += "\n" + Repeat("=", *chars_in_line) + "\n";

    *chars_in_line = 0;
  }

  void TurnLineIntoHeader2(std::string *md, u_int16_t *chars_in_line) {
    *md += "\n" + Repeat("-", *chars_in_line) + "\n";

    *chars_in_line = 0;
  }

  std::string Convert2Md(std::string html) {
    ReplaceAll(&html, "\t", " ");

    for (char ch : html) {
      if (!is_in_tag_ && ch=='<') {
        is_in_tag_ = true;
        current_tag_ = "";

        if (!md_.empty()) {
          prev_ch_ = md_[md_.length() - 1];

          if (prev_ch_!=' ' && prev_ch_!='\n')
            md_ += ' ';
        }

        continue;
      }

      auto md_len = md_.length();

      if (is_in_tag_) {
        if (ch=='/' && current_tag_.empty()) {
          is_closing_tag_ = true;

          continue;
        }

        if (ch=='>') {
          is_in_tag_ = false;

          current_tag_ = Explode(current_tag_, ' ')[0];
          prev_ch_ = md_[md_.length() - 1];

          if (is_closing_tag_) {
            // '>' = has left closing of tag
            is_closing_tag_ = false;

            if (current_tag_=="b" || current_tag_=="strong") {
              if (prev_ch_==' ') md_ = md_.substr(0, md_.length() - 1);

              md_ += "** ";
            } else if (current_tag_=="noscript") {
              is_in_child_of_noscript_tag_ = false;
            } else if (md_len > 0) {
              if (current_tag_=="span") {
                md_ += "\n";
              } else if (current_tag_=="title") {
                TurnLineIntoHeader1(&md_, &chars_in_curr_line);
              } else if (current_tag_=="h1") {
                TurnLineIntoHeader2(&md_, &chars_in_curr_line);
              }
            }
          } else {
            // '>' = has left opening of tag

            if (current_tag_=="b" || current_tag_=="strong") {
              if (prev_ch_!=' ') md_ += ' ';

              md_ += "**";
            } else if (current_tag_=="noscript") {
              is_in_child_of_noscript_tag_ = true;
            }
          }

          continue;
        }

        current_tag_ += ch;
      }

      if (!is_in_tag_) {
        if (is_in_child_of_noscript_tag_
            || current_tag_=="link"
            || current_tag_=="meta"
            || current_tag_=="script")
          continue;

        prev_ch_ = md_len > 0 ? md_[md_len - 1] : '.';
        prev_prev_ch_ = md_len > 1 ? md_[md_len - 2] : '.';

        if (ch==' ' && (prev_ch_==' ' || prev_ch_=='\n'))
          continue;  // prevent more than two consecutive spaces

        if (ch=='\n' && prev_ch_=='\n' && prev_prev_ch_=='\n')
          continue;  // prevent more than two consecutive newlines

        md_ += ch;

        if (ch!='\n') ++chars_in_curr_line;
      }

      if (md_len > 400) break;
    }

    return md_;
  }
};  // Converter

std::string Convert(const std::string &html) {
  return html2md::Converter::Convert(html);
}

}  // namespace html2md

#endif  // HTML2MD_HPP_
