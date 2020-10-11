// Copyright (c) Kay Stenschke
// Licensed under the MIT License - https://opensource.org/licenses/MIT

#ifndef HTML2MD_HPP_
#define HTML2MD_HPP_

#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <regex>  // NOLINT [build/c++11]

namespace html2md {

class Converter {
 public:
  static std::string Convert(std::string *html) {
    auto *instance = new Converter();

    PrepareHtml(html);

    auto md = instance
        ->Convert2Md(*html)
        ->GetMd_();

    delete instance;

    CleanUpMarkdown(&md);

    return md;
  }

  static void CleanUpMarkdown(std::string *md) {
    TidyAllLines(md);

    ReplaceAll(md, " , ", ", ");
    ReplaceAll(md, "\n.\n", ".\n");
    ReplaceAll(md, "\n↵\n", " ↵\n");
    ReplaceAll(md, "\n*\n", "\n");
    ReplaceAll(md, "\n. ", ".\n");
  }

  static void PrepareHtml(std::string *html) {
    ReplaceAll(html, "\t", " ");
    ReplaceAll(html, "&amp;", "&");
    ReplaceAll(html, "&nbsp;", " ");
    ReplaceAll(html, "&rarr;", "→");

    std::regex exp("<!--(.*?)-->");

    *html = regex_replace(*html, exp, "");
  }

  const std::string &GetMd_() const {
    return md_;
  }

   private:
    static constexpr const char *kTagAnchor = "a";
    static constexpr const char *kTagBold = "b";
    static constexpr const char *kTagBreak = "br";
    static constexpr const char *kTagDiv = "div";
    static constexpr const char *kTagHeader1 = "h1";
    static constexpr const char *kTagHeader2 = "h2";
    static constexpr const char *kTagHeader3 = "h3";
    static constexpr const char *kTagHeader4 = "h4";
    static constexpr const char *kTagLink = "link";
    static constexpr const char *kTagListItem = "li";
    static constexpr const char *kTagMeta = "meta";
    static constexpr const char *kTagNoScript = "noscript";
    static constexpr const char *kTagOption = "option";
    static constexpr const char *kTagOrderedList = "ol";
    static constexpr const char *kTagParagraph = "p";
    static constexpr const char *kTagScript = "script";
    static constexpr const char *kTagSpan = "span";
    static constexpr const char *kTagStrong = "strong";
    static constexpr const char *kTagStyle = "style";
    static constexpr const char *kTagTemplate = "template";
    static constexpr const char *kTagTitle = "title";
    static constexpr const char *kTagUnorderedList = "ul";

    bool is_in_tag_ = false;
    bool is_closing_tag_ = false;
    bool is_in_child_of_noscript_tag_ = false;
    bool is_in_script_tag_ = false;
    bool is_in_style_tag_ = false;
    bool is_in_svg_tag_ = false;
    bool is_in_template_tag_ = false;
    bool is_in_attribute_value_ = false;

    // relevant for <li> only, false = is in unordered list
    bool is_in_ordered_list_ = true;
    int index_li;

    char prev_ch_, prev_prev_ch_;

    std::string current_tag_;
    std::string prev_tag_;
    std::string current_attribute_;
    std::string current_attribute_value_;

    u_int16_t chars_in_curr_line_ = 0;

    std::string md_;
    size_t  md_len_;

    Converter() = default;

    // Trim from start (in place)
    static void LTrim(std::string *s) {
      (*s).erase(
          (*s).begin(),
          std::find_if(
              (*s).begin(),
              (*s).end(),
              std::not1(std::ptr_fun<int, int>(std::isspace))));
    }

    // Trim from end (in place)
    static void RTrim(std::string *s) {
      (*s).erase(
          std::find_if(
              (*s).rbegin(),
              (*s).rend(),
              std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          (*s).end());
    }

    // Trim from both ends (in place)
    static void Trim(std::string *s) {
      LTrim(s);
      RTrim(s);
    }

    // 1. trim all lines
    // 2. reduce consecutive newlines to maximum 3
    static void TidyAllLines(std::string *str) {
      auto lines = Explode(*str, '\n');
      std::string res;

      int amount_newlines = 0;

      for (auto line : lines) {
        Trim(&line);

        if (line.empty()) {
          if (amount_newlines < 2) {
            res += "\n";
            amount_newlines++;
          }
        } else {
          amount_newlines = 0;

          res += line + "\n";
        }
      }

      *str = res.substr(0, res.length() - 1);
    };

    static int ReplaceAll(std::string *haystack,
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
    static std::vector<std::string> Explode(std::string const &str, char delimiter) {
      std::vector<std::string> result;
      std::istringstream iss(str);

      for (std::string token; std::getline(iss, token, delimiter);)
        result.push_back(std::move(token));

      return result;
    }

    // Repeat given amount of given string
    static std::string Repeat(const std::string &str, u_int16_t amount) {
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

    Converter* Convert2Md(const std::string html) {
      for (char ch : html) {
        if (!is_in_tag_ && ch == '<') {
          OnHasEnteredTag();

          continue;
        }

        md_len_ = md_.length();

        if (is_in_tag_ && ParseCharInTag(ch)) continue;

        if (!is_in_tag_ && ParseCharInTagContent(ch)) continue;

//        if (md_len_ > 4000) break;
      }

      return this;
    }

    // Current char: '<'
    void OnHasEnteredTag() {
      is_in_tag_ = true;
      prev_tag_ = current_tag_;
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

      if (is_closing_tag_)
        OnHasLeftClosingTag();
      else
        OnHasLeftOpeningTag();

      return true;
    }

    void OnHasLeftOpeningTag() {
      // '>' = has left opening-tag
      if (current_tag_ == kTagBold || current_tag_ == kTagStrong) {
        if (prev_ch_ != ' ') {
          md_ += ' ';
          ++chars_in_curr_line_;
        }

        md_ += "**";
        ++chars_in_curr_line_;
      } else if (current_tag_ == kTagHeader2) {
        md_ += "\n\n\n### ";
        chars_in_curr_line_ = 4;
      } else if (current_tag_ == kTagHeader3) {
        md_ += "\n\n\n#### ";
        chars_in_curr_line_ = 5;
      } else if (current_tag_ == kTagHeader4) {
        md_ += "\n\n\n##### ";
        chars_in_curr_line_ = 6;
      } else if (current_tag_ == kTagListItem) {
        if (prev_ch_ != '\n') md_ += '\n';

        md_ += "* ";
        chars_in_curr_line_ = 2;
      } else if (current_tag_ == kTagUnorderedList
        || current_tag_ == kTagOrderedList
        || current_tag_ == kTagDiv) {
        if (prev_ch_ != '\n') {
          md_ += '\n';
          chars_in_curr_line_ = 0;
        }

        if (prev_prev_ch_ != '\n') {
          md_ += '\n';
          chars_in_curr_line_ = 0;
        }

      } else if (current_tag_ == kTagNoScript) {
        is_in_child_of_noscript_tag_ = true;
      } else if (current_tag_ == kTagScript) {
        is_in_script_tag_ = true;
      } else if (current_tag_ == kTagStyle) {
        is_in_style_tag_ = true;
      } else if (current_tag_ == kTagTemplate) {
        is_in_template_tag_ = true;
      }
    }

    void OnHasLeftClosingTag() {
      // '>' = has left closing-tag
      is_closing_tag_ = false;

      if (current_tag_ == kTagAnchor) {
//        md_ += '[';
      } else if (current_tag_ == kTagBold || current_tag_ == kTagStrong) {
        if (prev_ch_ == ' ') ShortenMarkdown();

        md_ += "**";
      } else if (current_tag_ == kTagHeader2
          || current_tag_ == kTagHeader3
          || current_tag_ == kTagHeader4) {
        md_ += "\n\n";
        chars_in_curr_line_ = 0;
      } else if (current_tag_ == kTagNoScript) {
        is_in_child_of_noscript_tag_ = false;
      } else if (current_tag_ == kTagScript) {
        is_in_script_tag_ = false;
      } else if (current_tag_ == kTagStyle) {
        is_in_style_tag_ = false;
      } else if (current_tag_ == kTagTemplate) {
        is_in_template_tag_ = false;
      } else if (md_len_ > 0) {
        if (current_tag_ == kTagParagraph) {
          md_ += "  \n\n";
          chars_in_curr_line_ = 0;
        } else if (current_tag_ == kTagBreak
            || current_tag_ == kTagOption
            || current_tag_ == kTagListItem) {
          md_ += "  \n";
          chars_in_curr_line_ = 0;
        } else if (current_tag_ == kTagSpan) {
          if (prev_ch_ != ' ') md_ += " ";
        } else if (current_tag_ == kTagTitle) {
          TurnLineIntoHeader1(&md_, &chars_in_curr_line_);
        } else if (current_tag_ == kTagHeader1) {
          TurnLineIntoHeader2(&md_, &chars_in_curr_line_);
        }
      }
    }

    void ShortenMarkdown(int chars = 1) {
      md_ = md_.substr(0, md_len_ - chars);
    }

    bool ParseCharInTagContent(char ch) {
      if (is_in_child_of_noscript_tag_
          || is_in_script_tag_
          || is_in_style_tag_
          || is_in_template_tag_
          || current_tag_ == kTagLink
          || current_tag_ == kTagMeta
          || current_tag_ == kTagScript) return true;

      prev_ch_ = md_len_ > 0 ? md_[md_len_ - 1] : '.';
      prev_prev_ch_ = md_len_ > 1 ? md_[md_len_ - 2] : '.';

      // prevent more than one consecutive spaces
      if (ch == ' ') {
        if (md_len_ == 0
            || prev_ch_== ' '
            || prev_ch_=='\n') return true;
      }

      if (ch == '\n') return true;

      if (ch == '.' && prev_ch_ == ' ') {
        ShortenMarkdown();
        --chars_in_curr_line_;
      }

      md_ += ch;
      ++chars_in_curr_line_;

      if (chars_in_curr_line_ > 80 && ch == ' ') {
        md_ += "\n";
        chars_in_curr_line_ = 0;
      }

      return false;
    }
};  // Converter

std::string Convert(std::string html) {
  return html2md::Converter::Convert(&html);
}


}  // namespace html2md

#endif  // HTML2MD_HPP_
