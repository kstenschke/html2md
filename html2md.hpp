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

    auto md = instance
        ->Convert2Md(html)
        ->GetMd_();

    delete instance;

    return md;
  }

  const std::string &GetMd_() const {
    return md_;
  }

  private:
    static constexpr const char *kTagAnchor = "a";
    static constexpr const char *kTagBold = "b";
    static constexpr const char *kTagHeader1 = "h1";
    static constexpr const char *kTagHeader2 = "h2";
    static constexpr const char *kTagLink = "link";
    static constexpr const char *kTagMeta = "meta";
    static constexpr const char *kTagNoScript = "noscript";
    static constexpr const char *kTagScript = "script";
    static constexpr const char *kTagSpan = "span";
    static constexpr const char *kTagStrong = "strong";
    static constexpr const char *kTagTitle = "title";

    bool is_in_tag_ = false;
    bool is_closing_tag_ = false;
    bool is_in_child_of_noscript_tag_ = false;
    bool is_in_attribute_value_ = false;

    char prev_ch_, prev_prev_ch_;

    std::string current_tag_;
    std::string current_attribute_;
    std::string current_attribute_value_;

    u_int16_t chars_in_curr_line = 0;

    std::string md_;
    size_t  md_len_;

    Converter() = default;

    int ReplaceAll(std::string *haystack,
                   const std::string &needle,
                   const std::string &replacement) {
      // Get first occurrence
      size_t pos = (*haystack).find(needle);

      int amount_replaced = 0;

      // Repeat until end is reached
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
    std::vector<std::string> Explode(std::string const &str, char delimiter) {
      std::vector<std::string> result;
      std::istringstream iss(str);

      for (std::string token; std::getline(iss, token, delimiter);)
        result.push_back(std::move(token));

      return result;
    }

    // Repeat given amount of given string
    std::string Repeat(const std::string &str, u_int16_t amount) {
      std::string out;

      for (u_int16_t i = 0; i < amount; i++) {
        out += str;
      }

      return out;
    }

    void TurnLineIntoHeader1(std::string *md, u_int16_t *chars_in_line) {
      *md += "\n" + Repeat("=", *chars_in_line) + "\n\n";

      *chars_in_line = 0;
    }

    void TurnLineIntoHeader2(std::string *md, u_int16_t *chars_in_line) {
      *md += "\n" + Repeat("-", *chars_in_line) + "\n\n";

      *chars_in_line = 0;
    }

    Converter* Convert2Md(std::string html) {
      ReplaceAll(&html, "\t", " ");

      for (char ch : html) {
        if (!is_in_tag_ && ch == '<') {
          OnHasEnteredTag();

          continue;
        }

        md_len_ = md_.length();

        if (is_in_tag_ && ParseCharInTag(ch)) continue;

        if (!is_in_tag_ && ParseCharInTagContent(ch)) continue;

        if (md_len_ > 4000) break;
      }

      return this;
    }

    // Current char: '<'
    void OnHasEnteredTag() {
      is_in_tag_ = true;
      current_tag_ = "";

      if (!md_.empty()) {
        prev_ch_ = md_[md_.length() - 1];

        if (prev_ch_ != ' ' && prev_ch_ != '\n')
          md_ += ' ';
      }
    }

    /**
     * Handle next char within <...> tag
     *
     * @param ch current character
     * @return   continue surrounding iteration?
     */
    bool ParseCharInTag(char ch) {
      if (ch == '/' && current_tag_.empty()) {
        is_closing_tag_ = true;

        return true;
      }

      if (ch == '>') return OnHasLeftTag();

      if (ch == '=') return true;

      if (ch == '"') {
        if (is_in_attribute_value_) {
          // is at last char of value of attribute
  //        if (current_attribute_ == "href") {
  //          md_ += "](" + current_attribute_value_ + ")";
  //        }

          is_in_attribute_value_ = false;
          current_attribute_ = "";
          current_attribute_value_ = "";
        } else if (prev_ch_ == '=') {
          is_in_attribute_value_ = true;
        }

        return true;
      }

      if (is_in_attribute_value_) {
        current_attribute_value_ += ch;
      } else {
        current_attribute_ += ch;
      }

      current_tag_ += ch;

      return false;
    }

    // Current char: '>'
    bool OnHasLeftTag() {
      is_in_tag_ = false;

      md_len_ = md_.length();
      current_tag_ = Explode(current_tag_, ' ')[0];
      prev_ch_ = md_[md_len_ - 1];

      if (is_closing_tag_) {
        // '>' = has left closing of tag
        is_closing_tag_ = false;

        if (current_tag_== kTagAnchor) {
  //        md_ += '[';
        } else if (current_tag_== kTagBold || current_tag_== kTagStrong) {
          if (prev_ch_ == ' ') ShortenMarkdown();

          md_ += "** ";
        } else if (current_tag_ == kTagNoScript) {
          is_in_child_of_noscript_tag_ = false;
        } else if (md_len_ > 0) {
          if (current_tag_ == kTagSpan) {
            md_ += "\n";
          } else if (current_tag_ == kTagTitle) {
            TurnLineIntoHeader1(&md_, &chars_in_curr_line);
          } else if (current_tag_ == kTagHeader1) {
            TurnLineIntoHeader2(&md_, &chars_in_curr_line);
          }
        }
      } else {
        // '>' = has left opening of tag

        if (current_tag_== kTagBold || current_tag_== kTagStrong) {
          if (prev_ch_ != ' ') md_ += ' ';

          md_ += "**";
        } else if (current_tag_== kTagNoScript) {
          is_in_child_of_noscript_tag_ = true;
        }
      }

      return true;
    }

    void ShortenMarkdown(int chars = 1) {
      md_ = md_.substr(0, md_len_ - chars);
    }

    bool ParseCharInTagContent(char ch) {
      if (is_in_child_of_noscript_tag_
          || current_tag_ == kTagLink
          || current_tag_ == kTagMeta
          || current_tag_ == kTagScript) return true;

      prev_ch_ = md_len_ > 0 ? md_[md_len_ - 1] : '.';
      prev_prev_ch_ = md_len_ > 1 ? md_[md_len_ - 2] : '.';

      // prevent more than one consecutive spaces
      if (ch == ' ' && (prev_ch_ == ' ' || prev_ch_ == '\n')) return true;

      // prevent more than two consecutive newlines
      if (ch == '\n' && prev_ch_ == '\n' && prev_prev_ch_ == '\n') return true;

      if (ch != '\n') {
        if (ch == '.' && prev_ch_ == ' ') {
          ShortenMarkdown();
          --chars_in_curr_line;
        }

        md_ += ch;
        ++chars_in_curr_line;

        if (chars_in_curr_line > 120) {
          // TODO: find offset of previous ' ' and insert newline there
          md_ += "\n";
          chars_in_curr_line = 0;
        }
      }

      return false;
    }
};  // Converter

std::string Convert(const std::string &html) {
  return html2md::Converter::Convert(html);
}


}  // namespace html2md

#endif  // HTML2MD_HPP_
