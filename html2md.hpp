// Copyright (c) Kay Stenschke
// Licensed under the MIT License - https://opensource.org/licenses/MIT

#ifndef HTML2MD_HPP_
#define HTML2MD_HPP_

#include <functional>
#include <regex>  // NOLINT [build/c++11]
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <map>

namespace html2md {

// Main class: HTML to Markdown converter
class Converter {
 public:
  ~Converter() = default;

  static std::string Convert(std::string *html) {
    auto *instance = new Converter(html);

    auto md = instance->PrepareHtml(html)
                      ->Convert2Md(*html)
                      ->GetMd_();

    instance->CleanUpMarkdown(&md);

    delete instance;

    return md;
  }

  Converter * PrepareHtml(std::string *html) {
    ReplaceAll(html, "\t", " ");
    ReplaceAll(html, "&amp;", "&");
    ReplaceAll(html, "&nbsp;", " ");
    ReplaceAll(html, "&rarr;", "→");

    std::regex exp("<!--(.*?)-->");
    *html = regex_replace(*html, exp, "");

    return this;
  }

  void CleanUpMarkdown(std::string *md) {
    TidyAllLines(md);

    ReplaceAll(md, " , ", ", ");

    ReplaceAll(md, "\n.\n", ".\n");
    ReplaceAll(md, "\n↵\n", " ↵\n");
    ReplaceAll(md, "\n*\n", "\n");
    ReplaceAll(md, "\n. ", ".\n");

    ReplaceAll(md, " [ ", " [");
    ReplaceAll(md, "\n[ ", "\n[");
  }

  const std::string &GetMd_() const {
    return md_;
  }

  Converter* AppendToMd(char ch) {
    if (IsInIgnoredTag()) return this;

    md_ += ch;

    if (ch == '\n')
      chars_in_curr_line_ = 0;
    else
      ++chars_in_curr_line_;

    return this;
  }

  Converter* AppendToMd(const char *str) {
    if (IsInIgnoredTag()) return this;

    md_ += str;

    auto str_len = strlen(str);

    for (int i = 0; i < str_len; ++i) {
      if (str[i] == '\n')
        chars_in_curr_line_ = 0;
      else
        ++chars_in_curr_line_;
    }

    return this;
  }

  // Append ' ' if:
  // - md does not end w/ '**'
  // - md does not end w/ '\n'
  Converter* AppendBlank() {
    UpdatePrevChFromMd();

    if (prev_ch_in_md_ == '\n'
        || (prev_ch_in_md_ == '*' && prev_prev_ch_in_md_ == '*')) return this;

    return AppendToMd(' ');
  }

 private:
  static constexpr const char *kAttributeHref = "href";

  static constexpr const char *kTagAnchor = "a";
  static constexpr const char *kTagBold = "b";
  static constexpr const char *kTagBreak = "br";
  static constexpr const char *kTagDiv = "div";
  static constexpr const char *kTagHead = "head";
  static constexpr const char *kTagHeader1 = "h1";
  static constexpr const char *kTagHeader2 = "h2";
  static constexpr const char *kTagHeader3 = "h3";
  static constexpr const char *kTagHeader4 = "h4";
  static constexpr const char *kTagLink = "link";
  static constexpr const char *kTagListItem = "li";
  static constexpr const char *kTagMeta = "meta";
  static constexpr const char *kTagNav = "nav";
  static constexpr const char *kTagNoScript = "noscript";
  static constexpr const char *kTagOption = "option";
  static constexpr const char *kTagOrderedList = "ol";
  static constexpr const char *kTagParagraph = "p";
  static constexpr const char *kTagPre = "pre";
  static constexpr const char *kTagScript = "script";
  static constexpr const char *kTagSpan = "span";
  static constexpr const char *kTagStrong = "strong";
  static constexpr const char *kTagStyle = "style";
  static constexpr const char *kTagTemplate = "template";
  static constexpr const char *kTagTitle = "title";
  static constexpr const char *kTagUnorderedList = "ul";

  u_int16_t index_ch_in_html_ = 0;

  std::vector<std::string> dom_tags_;

  bool is_in_tag_ = false;
  bool is_closing_tag_ = false;

  bool is_in_attribute_value_ = false;

  // relevant for <li> only, false = is in unordered list
  bool is_in_ordered_list_ = true;
  int index_li;

  char prev_ch_in_md_, prev_prev_ch_in_md_;
  char prev_ch_in_html_ = 'x';

  std::string html_;

  u_int16_t offset_lt_;
  std::string current_tag_;
  std::string prev_tag_;

  std::string current_attribute_;
  std::string current_attribute_value_;

  std::string current_href_;

  u_int16_t chars_in_curr_line_ = 0;
  u_int16_t char_index_in_tag_content = 0;

  std::string md_;
  size_t md_len_;

  // Tag: base class for tag types
  struct Tag {
    virtual void OnHasLeftOpeningTag(Converter* converter) = 0;
    virtual void OnHasLeftClosingTag(Converter* converter) = 0;
  };

  // Tag types

  // tags that are not printed (nav, script, noscript, ...)
  struct TagIgnored : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
    }
  };

  struct TagAnchor : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->IsInIgnoredTag()) return;

      converter->current_href_ =
          converter->RTrim(&converter->md_, true)
                   ->AppendBlank()
                   ->AppendToMd("[")
                   ->ExtractAttributeFromTagLeftOf(kAttributeHref);
    }

    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->IsInIgnoredTag()) return;

      if (converter->prev_ch_in_md_ == ' ') {
        converter->ShortenMarkdown();
      }

      if (converter->prev_ch_in_md_ == '[') {
        converter->ShortenMarkdown();
      } else {
        converter->AppendToMd("](")
                 ->AppendToMd(converter->current_href_.c_str())
                 ->AppendToMd(") ");
      }
    }
  };

  struct TagBold : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_in_md_ != ' ') converter->AppendToMd(' ');

      converter->AppendToMd("**");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->prev_ch_in_md_ == ' ') converter->ShortenMarkdown();

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
      if (converter->prev_ch_in_md_ != '\n') converter->AppendToMd('\n');

      if (converter->prev_prev_ch_in_md_ != '\n') converter->AppendToMd('\n');
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
      if (converter->prev_ch_in_md_ != '\n') converter->AppendToMd("\n");

      converter->AppendToMd("* ");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->md_len_ > 0) converter->AppendToMd("  \n");
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
      if (converter->prev_ch_in_md_ != '\n') converter->AppendToMd("\n");

      if (converter->prev_prev_ch_in_md_ != '\n') converter->AppendToMd("\n");
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

  struct TagPre : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
      if (converter->prev_ch_in_md_ != '\n') converter->AppendToMd("\n");

      if (converter->prev_prev_ch_in_md_ != '\n') converter->AppendToMd("\n");

      converter->AppendToMd("````\n");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      converter->AppendToMd("\n````\n\n");
    }
  };

  struct TagSpan : Tag {
    void OnHasLeftOpeningTag(Converter* converter) override {
    }
    void OnHasLeftClosingTag(Converter* converter) override {
      if (converter->prev_ch_in_md_ != ' '
          && converter->char_index_in_tag_content > 0)
        converter->AppendToMd(' ');
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
      if (converter->prev_ch_in_md_ != '\n') converter->AppendToMd("\n");

      if (converter->prev_prev_ch_in_md_ != '\n') converter->AppendToMd("\n");
    }
    void OnHasLeftClosingTag(Converter* converter) override {
    }
  };

  std::map<std::string, Tag*> tags_;

  explicit Converter(std::string *html) {
    html_ = *html;

    // non-printing tags
    auto *tagIgnored = new TagIgnored();
    tags_[kTagHead] = tagIgnored;
    tags_[kTagMeta] = tagIgnored;
    tags_[kTagNav] = tagIgnored;
    tags_[kTagNoScript] = tagIgnored;
    tags_[kTagScript] = tagIgnored;
    tags_[kTagStyle] = tagIgnored;
    tags_[kTagTemplate] = tagIgnored;

    // printing tags
    tags_[kTagAnchor] = new TagAnchor();
    tags_[kTagBreak] = new TagBreak();
    tags_[kTagDiv] = new TagDiv();
    tags_[kTagHeader1] = new TagHeader1();
    tags_[kTagHeader2] = new TagHeader2();
    tags_[kTagHeader3] = new TagHeader3();
    tags_[kTagHeader4] = new TagHeader4();
    tags_[kTagListItem] = new TagListItem();
    tags_[kTagOption] = new TagOption();
    tags_[kTagOrderedList] = new TagOrderedList();
    tags_[kTagPre] = new TagPre();
    tags_[kTagParagraph] = new TagParagraph();
    tags_[kTagSpan] = new TagSpan();
    tags_[kTagTitle] = new TagTitle();
    tags_[kTagUnorderedList] = new TagUnorderedList();

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
  Converter * RTrim(std::string *s, bool trim_only_blank = false) {
    (*s).erase(
        std::find_if(
            (*s).rbegin(),
            (*s).rend(),
            trim_only_blank
              ? std::not1(std::ptr_fun<int, int>(std::isblank))
              : std::not1(std::ptr_fun<int, int>(std::isspace))
        )
            .base(),
        (*s).end());

    return this;
  }

  // Trim from both ends (in place)
  Converter * Trim(std::string *s) {
    LTrim(s);
    RTrim(s);

    return this;
  }

  // 1. trim all lines
  // 2. reduce consecutive newlines to maximum 3
  void TidyAllLines(std::string *str) {
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

  std::string ExtractAttributeFromTagLeftOf(std::string attr) {
    char ch = ' ';

    // Extract the whole tag from current offset, e.g. from '>', backwards
    auto tag = html_.substr(offset_lt_, index_ch_in_html_ - offset_lt_);

    // locate given attribute
    auto offset_attr = tag.find(attr);

    if (offset_attr == std::string::npos) return "";

    // locate attribute-value pair's '='
    auto offset_equals = tag.find('=', offset_attr);

    if (offset_equals == std::string::npos) return "";

    // locate value's surrounding quotes
    auto offset_double_quote = tag.find('"', offset_equals);
    auto offset_single_quote = tag.find('\'', offset_equals);

    bool has_double_quote = offset_double_quote != std::string::npos;
    bool has_single_quote = offset_single_quote != std::string::npos;

    if (!has_double_quote && !has_single_quote) return "";

    char wrapping_quote;

    u_int64_t offset_opening_quote;
    u_int64_t offset_closing_quote;

    if (has_double_quote) {
      if (!has_single_quote) {
        wrapping_quote = '"';
        offset_opening_quote = offset_double_quote;
      } else {
        if (offset_double_quote < offset_single_quote) {
          wrapping_quote = '"';
          offset_opening_quote = offset_double_quote;
        } else {
          wrapping_quote = '\'';
          offset_opening_quote = offset_single_quote;
        }
      }
    } else {
      // has only single quote
      wrapping_quote = '\'';
      offset_opening_quote = offset_single_quote;
    }

    if (offset_opening_quote == std::string::npos) return "";

    offset_closing_quote = tag.find(wrapping_quote, offset_opening_quote + 1);

    if (offset_closing_quote == std::string::npos) return "";

    return tag.substr(
        offset_opening_quote + 1,
        offset_closing_quote - 1 - offset_opening_quote);
  }

  void TurnLineIntoHeader1() {
    md_ += "\n" + Repeat("=", chars_in_curr_line_) + "\n\n";

    chars_in_curr_line_ = 0;
  }

  void TurnLineIntoHeader2() {
    md_ += "\n" + Repeat("-", chars_in_curr_line_) + "\n\n";

    chars_in_curr_line_ = 0;
  }

  // Main character iteration of parser
  Converter *Convert2Md(const std::string html) {
    for (char ch : html) {
      if (!is_in_tag_
          && ch == '<') {
        OnHasEnteredTag();

        continue;
      }

      UpdateMdLen();

      if ((is_in_tag_ && ParseCharInTag(ch))
          || (!is_in_tag_ && ParseCharInTagContent(ch)))
        continue;

//        if (md_len_ > 4000) break;

      ++index_ch_in_html_;
    }

    return this;
  }

  void UpdateMdLen() { md_len_ = md_.length(); }

  // Current char: '<'
  void OnHasEnteredTag() {
    offset_lt_ = index_ch_in_html_;
    is_in_tag_ = true;
    prev_tag_ = current_tag_;
    current_tag_ = "";

    if (!md_.empty()) {
      prev_ch_in_md_ = md_[md_.length() - 1];

      if (prev_ch_in_md_ != ' ' && prev_ch_in_md_ != '\n')
        md_ += ' ';
    }
  }

  Converter * UpdatePrevChFromMd() {
    if (!md_.empty()) {
      prev_ch_in_md_ = md_[md_.length() - 1];

      if (md_.length() > 1)
        prev_prev_ch_in_md_ = md_[md_.length() - 2];
    }

    return this;
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
      } else if (prev_ch_in_md_ == '=') {
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

    UpdateMdLen();

    current_tag_ = Explode(current_tag_, ' ')[0];

    prev_ch_in_md_ = md_[md_len_ - 1];

    char_index_in_tag_content = 0;

    Tag* tag = tags_[current_tag_];

    if (tag != nullptr) {
      if (is_closing_tag_) {
        is_closing_tag_ = false;

        tag->OnHasLeftClosingTag(this);

        if (!dom_tags_.empty()) dom_tags_.pop_back();
      } else {
        dom_tags_.push_back(current_tag_);

        tag->OnHasLeftOpeningTag(this);
      }
    }

    return true;
  }

  Converter* ShortenMarkdown(int chars = 1) {
    UpdateMdLen();

    md_ = md_.substr(0, md_len_ - chars);

    return this->UpdatePrevChFromMd();
  }

  bool ParseCharInTagContent(char ch) {
    if (ch == '\n') return true;

    if (IsInIgnoredTag()
        || current_tag_ == kTagLink) {
      prev_ch_in_html_ = ch;

      return true;
    }

    prev_ch_in_md_ = md_len_ > 0
                     ? md_[md_len_ - 1]
                     : '.';

    prev_prev_ch_in_md_ = md_len_ > 1
                          ? md_[md_len_ - 2]
                          : '.';

    // prevent more than one consecutive spaces
    if (ch == ' ') {
      if (md_len_ == 0
          || char_index_in_tag_content == 0
          || prev_ch_in_md_ == ' '
          || prev_ch_in_md_ == '\n')
        return true;
    }

    if (ch == '.' && prev_ch_in_md_ == ' ') {
      ShortenMarkdown();
      --chars_in_curr_line_;
    }

    md_ += ch;

    ++chars_in_curr_line_;
    ++char_index_in_tag_content;

    if (chars_in_curr_line_ > 80 && ch == ' ') {
      md_ += "\n";
      chars_in_curr_line_ = 0;
    }

    return false;
  }

  bool IsIgnoredTag(const std::string &tag) {
    return (kTagTemplate == tag
        || kTagStyle == tag
        || kTagScript == tag
        || kTagNoScript == tag
        || kTagNav == tag);

    // meta: not ignored to tolerate if closing is omitted
  }

  bool IsInIgnoredTag() {
    auto len = dom_tags_.size();

    for (int i = 0; i < len; ++i) {
      std::string tag = dom_tags_[i];

      if (kTagPre == tag
          || kTagTitle == tag) return false;

      if (IsIgnoredTag(tag)) return true;
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
