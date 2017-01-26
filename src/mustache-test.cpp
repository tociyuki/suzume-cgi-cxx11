#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <cstdio>
#include "mustache.hpp"
#include "taptests.hpp"

// mustache - C++ HTML template

void test_escape_html (test::simple& ts);
void test_synopsis (test::simple& ts);
void test_variables (test::simple& ts);
void test_striter (test::simple& ts);
void test_custom (test::simple& ts);
void test_sections (test::simple& ts);
void test_nonempty_list (test::simple& ts);
void test_non_false_values (test::simple& ts);
void test_inverted_sections (test::simple& ts);
void test_comments (test::simple& ts);

int
main (int argc, char* argv[])
{
    test::simple ts;
    test_escape_html (ts);
    test_synopsis (ts);
    test_variables (ts);
    test_striter (ts);
    test_custom (ts);
    test_sections (ts);
    test_nonempty_list (ts);
    test_non_false_values (ts);
    test_inverted_sections (ts);
    test_comments (ts);
    return ts.done_testing ();
}

void
is_escape_html (test::simple& ts, int escape_level, std::string const& input, std::string const& expected, std::string const& name)
{
    std::string got;
    mustache::page_base::append_html (escape_level, input.cbegin (), input.cend (), got);
    ts.ok (got == expected, name);
}

void
test_escape_html (test::simple& ts)
{
    is_escape_html (ts, 0, "&<>\"&amp;&#39;&#x30;",
        "&<>\"&amp;&#39;&#x30;", "no escape");
    is_escape_html (ts, 1, "&<>\"&amp;&#39;&#x30;",
        "&amp;&lt;&gt;&quot;&amp;&#39;&#x30;", "escape html");
    is_escape_html (ts, 2, "&<>\"&amp;&#39;&#x30;",
        "&amp;&lt;&gt;&quot;&amp;amp;&amp;#39;&amp;#x30;", "escape htmlall");
}

void
trim_bang (std::string& s)
{
    if (s.empty ())
        return;
    if ('\n' == s[0]) {
        s.erase (0, 1);
    }
    std::string::size_type j = s.find_last_not_of (" ");
    if (j != s.npos && j + 1 < s.size ())
        s.erase (j + 1);
}

void
test_synopsis (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { NAME, VALUE, TAXED_VALUE, IN_CA };

        std::string m_name;
        long m_value;

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("name",        NAME,        mustache::STRING);
            layout.bind ("value",       VALUE,       mustache::INTEGER);
            layout.bind ("taxed_value", TAXED_VALUE, mustache::DOUBLE);
            layout.bind ("in_ca",       IN_CA,       mustache::IF);
        }

        void valueof (int symbol, std::string& v)
        {
            if (NAME == symbol) v = "Chris";
        }

        void valueof (int symbol, long& v)
        {
            if (VALUE == symbol) v = m_value;
        }

        void valueof (int symbol, double& v)
        {
            if (TAXED_VALUE == symbol) {
                v = m_value;
                v = v - v * 0.4;
            }
        }

        void valueof (int symbol, bool& v)
        {
            if (IN_CA == symbol) v = true;
        }
    };

    std::string src (R"EOS(
Hello {{name}}
You have just won {{value}} dollars!
{{#in_ca}}
Well, {{taxed_value}} dollars, after taxes.
{{/in_ca}}
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
Hello Chris
You have just won 10000 dollars!
Well, 6000.0 dollars, after taxes.
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    page_type page;
    page.bind (layout);
    page.m_name = "Chris";
    page.m_value = 10000;
    ts.ok (layout.assemble (src), "SYNOPSIS assemble");
    std::string got;
    layout.expand (page, got);
    ts.ok (got == expected, "SYNOPSIS expand");
}

void
test_variables (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { NAME, COMPANY, COMPANY_RAW };

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("name",        NAME,        mustache::STRING);
            layout.bind ("company",     COMPANY,     mustache::STRING);
        }

        void valueof (int symbol, std::string& v)
        {
            switch (symbol) {
            case NAME:
                v = "Chris";
                break;
            case COMPANY:
                v = "<b>GitHub</b>";
                break;
            }
        }
    };

    std::string src (R"EOS(
* {{name}}
* {{age}}
* {{company}}
* {{{company}}}
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
* Chris
* 
* &lt;b&gt;GitHub&lt;/b&gt;
* <b>GitHub</b>
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "Variables assemble");
    std::string got;
    layout.expand (page, got);
    ts.ok (got == expected, "Variables expand");
}

void
test_striter (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { TEXT, TEXT_RAW };
        std::string text;

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("text",     TEXT,     mustache::STRITER);
        }

        void valueof (int symbol, std::string::const_iterator& vfirst,
                                  std::string::const_iterator& vlast)
        {
            if (TEXT == symbol) {
                vfirst = text.cbegin ();
                vlast = text.cend ();
            }
        }
    };

    std::string src (R"EOS(
<p>{{text}}</p>
<p>{{{text}}}</p>
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
<p>It is something to be &lt;b&gt;wrong&lt;/b&gt;, it will.</p>
<p>It is something to be <b>wrong</b>, it will.</p>
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    std::string got;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "striter assemble");
    page.text = "It is something to be <b>wrong</b>, it will.";
    layout.expand (page, got);
    ts.ok (got == expected, "striter expand");
}

void
test_custom (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { TEXT };
        std::string text;

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("text", TEXT, mustache::CUSTOM);
        }

        void expand (mustache::layout_type const& layout, std::size_t ip, mustache::span_type const& op, std::string& output)
        {
            if (TEXT == op.symbol) output.append (text);
        }
    };

    std::string src (R"EOS(
<p>{{text}}</p>
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
<p>It is something to be <b>wrong</b>, it will.</p>
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    std::string got;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "custom assemble");
    page.text = "It is something to be <b>wrong</b>, it will.";
    layout.expand (page, got);
    ts.ok (got == expected, "custom expand");
}

void
test_sections (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { PERSON = 1 };

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("person", PERSON, mustache::IF);
        }

        void valueof (int symbol, bool& v)
        {
            if (PERSON == symbol) v = false;
        }
    };

    std::string src (R"EOS(
Shown.
{{#person}}
  Never shown!
{{/person}}
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
Shown.
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "Sections assemble");
    std::string got;
    layout.expand (page, got);
    ts.ok (got == expected, "Sections expand");
}

void
test_nonempty_list (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { REPO, NAME };

        std::vector<std::string> repo;
        int repo_idx;

        page_type ()
        {
            repo = {"resque", "hub", "rip"};
            repo_idx = 0;
        }

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("repo", REPO, mustache::FOR);
            layout.bind ("name", NAME, mustache::STRING);
        }

        void iter (int symbol)
        {
            if (REPO == symbol) repo_idx = 0;
        }

        void next (int symbol)
        {
            if (REPO == symbol) ++repo_idx;
        }

        void valueof (int symbol, bool& v)
        {
            if (REPO == symbol) v = (repo_idx < repo.size ());
        }

        void valueof (int symbol, std::string& v)
        {
            if (NAME == symbol) v = repo[repo_idx];
        }
    };

    std::string src (R"EOS(
{{#repo}}
  <b>{{name}}</b>
{{/repo}}
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
  <b>resque</b>
  <b>hub</b>
  <b>rip</b>
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "Non-Empty Lists assemble");
    std::string got;
    layout.expand (page, got);
    ts.ok (got == expected, "Non-Empty Lists expand");
}

void
test_non_false_values (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { PERSONQ, NAME };

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("person?", PERSONQ, mustache::IF);
            layout.bind ("name",    NAME,    mustache::STRING);
        }

        void valueof (int symbol, bool& v)
        {
            if (PERSONQ == symbol) v = true;
        }

        void valueof (int symbol, std::string& v)
        {
            if (NAME == symbol) v = "Jon";
        }
    };

    std::string src (R"EOS(
{{#person?}}
  Hi {{name}}!
{{/person?}}
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
  Hi Jon!
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "Non-False Values assemble");
    std::string got;
    layout.expand (page, got);
    ts.ok (got == expected, "Non-False Values expand");
}

void
test_inverted_sections (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
        enum { REPO, NO_REPO, NAME };

        void bind (mustache::layout_type& layout)
        {
            layout.bind ("repo",    REPO,    mustache::IF);
            layout.bind ("name",    NAME,    mustache::STRING);
        }

        void valueof (int symbol, bool& v)
        {
            if (REPO == symbol)         v = false;
            else if (NO_REPO == symbol) v = true;
        }

        void valueof (int symbol, std::string& v)
        {
            if (NAME == symbol) v = "Jon";
        }
    };

    std::string src (R"EOS(
{{#repo}}
  <b>{{name}}</b>
{{/repo}}
{{^repo}}
  No repos :(
{{/repo}}
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
  No repos :(
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    page_type page;
    page.bind (layout);
    ts.ok (layout.assemble (src), "Inverted Sctions assemble");
    std::string got;
    layout.expand (page, got);
    ts.ok (got == expected, "Inverted Sctions expand");
}

void
test_comments (test::simple& ts)
{
    class page_type : public mustache::page_base {
    public:
    };

    std::string src (R"EOS(
<h1>Today{{! ignore {{me}} }}.</h1>
    )EOS");
    trim_bang (src);

    std::string expected (R"EOS(
<h1>Today.</h1>
    )EOS");
    trim_bang (expected);

    mustache::layout_type layout;
    std::string got;
    page_type page;
    ts.ok (layout.assemble (src), "Comments assemble");
    layout.expand (page, got);
    ts.ok (got == expected, "Comments expand");
}
