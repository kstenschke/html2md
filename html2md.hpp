// Copyright (c) Kay Stenschke
// Licensed under the MIT License - https://opensource.org/licenses/MIT

#ifndef HTML2MD_HPP_
#define HTML2MD_HPP_

#include <functional>
#include <regex>  // NOLINT [build/c++11]
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <map>

namespace html2md {

// Main class: HTML to Markdown converter
class Converter {
 public:
  ~Converter() {
  }

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

  void AppendToMd(char ch) {
    md_ += ch;

    if (ch == '\n')
      chars_in_curr_line_ = 0;
    else
      ++chars_in_curr_line_;
  }

  void AppendToMd(const char *str) {
    md_ += str;

    auto str_len = strlen(str);

    for (int i = 0; i < str_len; ++i) {
      if (str[i] == '\n')
        chars_in_curr_line_ = 0;
      else
        ++chars_in_curr_line_;
    }
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
  size_t md_len_;

  // Tag: base class for tag types
  struct Tag {
    virtual void OnHasLeftOpeningTag(Converter* converter) = 0;
    virtual void OnHasLeftClosingTag(Converter* converter) = 0;
  };

  // Tag types
  struct TagAnchor : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->AppendToMd("[");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->prev_ch_ == ' ') converter->ShortenMarkdown();

      converter->AppendToMd("]");
    }
  };

  struct TagBold : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_ != ' ') converter->AppendToMd(' ');

      converter->AppendToMd("**");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->prev_ch_ == ' ') converter->ShortenMarkdown();

      converter->AppendToMd("**");
    }
  };

  struct TagBreak : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->md_len_ > 0) converter->AppendToMd("  \n");
    }
  };

  struct TagDiv : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_ != '\n') converter->AppendToMd('\n');

      if (converter->prev_prev_ch_ != '\n') converter->AppendToMd('\n');
    }
    void OnHasLeftClosingTag(Converter* converter) override {
    }
  };

  struct TagHeader1 : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->md_len_ > 0)  converter->TurnLineIntoHeader2();
    }
  };

  struct TagHeader2 : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->AppendToMd("\n\n\n### ");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->AppendToMd("\n\n");
    }
  };

  struct TagHeader3 : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->AppendToMd("\n\n\n#### ");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->AppendToMd("\n\n");
    }
  };

  struct TagHeader4 : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->AppendToMd("\n\n\n##### ");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->AppendToMd("\n\n");
    }
  };

  struct TagListItem : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_ != '\n') converter->AppendToMd("\n");

      converter->AppendToMd("* ");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->md_len_ > 0) converter->AppendToMd("  \n");
    }
  };

  struct TagNoScript : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->is_in_child_of_noscript_tag_ = true;
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->is_in_child_of_noscript_tag_ = false;
    }
  };

  struct TagOption : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->md_len_ > 0) converter->AppendToMd("  \n");
    }
  };

  struct TagOrderedList : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_ != '\n') converter->AppendToMd("\n");

      if (converter->prev_prev_ch_ != '\n') converter->AppendToMd("\n");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
    }
  };

  struct TagParagraph : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (!converter->md_.empty()) converter->AppendToMd("  \n\n");
    }
  };

  struct TagScript : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->is_in_script_tag_ = true;
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->is_in_script_tag_ = false;
    }
  };

  struct TagSpan : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->prev_ch_ != ' ') converter->AppendToMd(' ');
    }
  };

  struct TagStyle : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->is_in_style_tag_ = true;
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->is_in_style_tag_ = false;
    }
  };

  struct TagTemplate : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      converter->is_in_template_tag_ = true;
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->is_in_template_tag_ = false;
    }
  };

  struct TagTitle : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->TurnLineIntoHeader1();
    }
  };

  struct TagUnorderedList : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_ != '\n') converter->AppendToMd("\n");

      if (converter->prev_prev_ch_ != '\n') converter->AppendToMd("\n");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
    }
  };

  std::map<std::string, Tag*> tags_;

  Converter() {
    tags_[kTagAnchor] = new TagAnchor();
    tags_[kTagBreak] = new TagBreak();
    tags_[kTagDiv] = new TagDiv();
    tags_[kTagHeader1] = new TagHeader1();
    tags_[kTagHeader2] = new TagHeader2();
    tags_[kTagHeader3] = new TagHeader3();
    tags_[kTagHeader4] = new TagHeader4();
    tags_[kTagListItem] = new TagListItem();
    tags_[kTagNoScript] = new TagNoScript();
    tags_[kTagOption] = new TagOption();
    tags_[kTagOrderedList] = new TagOrderedList();
    tags_[kTagParagraph] = new TagParagraph();
    tags_[kTagScript] = new TagScript();
    tags_[kTagSpan] = new TagSpan();
    tags_[kTagTemplate] = new TagTemplate();
    tags_[kTagTitle] = new TagTitle();
    tags_[kTagUnorderedList] = new TagUnorderedList();
    tags_[kTagStyle] = new TagStyle();

    auto *bold = new TagBold();
    tags_[kTagBold] = bold;
    tags_[kTagStrong] = bold;
  }

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
  }

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
  static std::vector<std::string> Explode(std::string const &str,
                                          char delimiter) {
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

  void TurnLineIntoHeader1() {
    md_ += "\n" + Repeat("=", chars_in_curr_line_) + "\n\n";

    chars_in_curr_line_ = 0;
  }

  void TurnLineIntoHeader2() {
    md_ += "\n" + Repeat("-", chars_in_curr_line_) + "\n\n";

    chars_in_curr_line_ = 0;
  }

  Converter *Convert2Md(const std::string html) {
    for (char ch : html) {
      if (!is_in_tag_ && ch == '<') {
        OnHasEnteredTag();

        continue;
      }

      md_len_ = md_.length();

      if ((is_in_tag_ && ParseCharInTag(ch))
          || (!is_in_tag_ && ParseCharInTagContent(ch)))
        continue;

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

    Tag* tag = tags_[current_tag_];

    if (tag != nullptr) {
      if (is_closing_tag_) {
        is_closing_tag_ = false;
        tag->OnHasLeftClosingTag(this);
      } else {
        tag->OnHasLeftOpeningTag(this);
      }
    }

    return true;
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
        || current_tag_ == kTagScript
        || ch == '\n')
      return true;

    prev_ch_ = md_len_ > 0 ? md_[md_len_ - 1] : '.';
    prev_prev_ch_ = md_len_ > 1 ? md_[md_len_ - 2] : '.';

    // prevent more than one consecutive spaces
    if (ch == ' ') {
      if (md_len_ == 0
          || prev_ch_ == ' '
          || prev_ch_ == '\n')
        return true;
    }

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

// Static wrapper for simple invocation
std::string Convert(std::string html) {
  return html2md::Converter::Convert(&html);
}

}  // namespace html2md

#endif  // HTML2MD_HPP_
