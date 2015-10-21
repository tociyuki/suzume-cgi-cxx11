#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <ostream>

namespace wjson {

enum variation {
    VALUE_NULL,
    VALUE_BOOLEAN,
    VALUE_FIXNUM,
    VALUE_FLONUM,
    VALUE_DATETIME,
    VALUE_STRING,
    VALUE_ARRAY,
    VALUE_TABLE,
};

class value_type;
class setter_type;

typedef std::vector<value_type> array_value_type;
typedef std::map<std::wstring,value_type> table_value_type;

struct setter_segment_type {
    variation mtag;
    std::size_t midx;
    std::wstring mkey;
};

class setter_type {
public:
    setter_type (value_type& value, std::size_t idx);
    setter_type (value_type& value, std::wstring const& k);
    setter_type& operator[] (value_type const& k);
    setter_type& operator[] (std::size_t idx);
    setter_type& operator[] (std::wstring const& k);
    setter_type& operator= (value_type const& x);
    setter_type& operator= (value_type&& x);
    setter_type& operator= (std::wstring const& x);
    setter_type& operator= (std::wstring&& x);

    variation tag () const;
    bool const& boolean () const;
    int64_t const& fixnum () const;
    double const& flonum () const;
    std::wstring const& datetime () const;
    std::wstring const& string () const;
    array_value_type const& array () const;
    table_value_type const& table () const;
    value_type* lookup () const;

private:
    value_type* force ();

    value_type& mvalue;
    std::vector<setter_segment_type> mpath;
};

class value_type {
public:
    value_type ();
    value_type (value_type const& x);
    value_type (value_type&& x);
    ~value_type ();
    value_type& operator= (value_type const& x);
    value_type& operator= (value_type&& x);

    value_type& assign_null ();
    value_type& assign_boolean (bool const x);
    value_type& assign_fixnum (int64_t const x);
    value_type& assign_flonum (double const x);
    value_type& assign_datetime (std::wstring const& x);
    value_type& assign_datetime (std::wstring&& x);
    value_type& assign_string (std::wstring const& x);
    value_type& assign_string (std::wstring&& x);
    value_type& assign_array (array_value_type const& x);
    value_type& assign_array (array_value_type&& x);
    value_type& assign_table (table_value_type const& x);
    value_type& assign_table (table_value_type&& x);

    setter_type operator[] (value_type const& k);
    setter_type operator[] (std::size_t idx);
    setter_type operator[] (std::wstring const& k);

    value_type const& get (value_type const& k) const;
    value_type& get (value_type const& k);
    value_type const& get (std::size_t const idx) const;
    value_type& get (std::size_t const idx);
    value_type const& get (std::wstring const& key) const;
    value_type& get (std::wstring const& key);
    value_type& set (value_type const& k, value_type const& x);
    value_type& set (value_type const& k, value_type&& x);
    value_type& set (std::size_t const idx, value_type const& x);
    value_type& set (std::size_t const idx, value_type&& x);
    value_type& push_back (value_type const& x);
    value_type& push_back (value_type&& x);
    value_type& set (std::wstring const& key, value_type const& x);
    value_type& set (std::wstring const& key, value_type&& x);

    void swap (value_type& x);
    variation tag () const;
    std::size_t size () const;
    bool const& boolean () const;
    bool& boolean ();
    int64_t const& fixnum () const;
    int64_t& fixnum ();
    double const& flonum () const;
    double& flonum ();
    std::wstring const& datetime () const;
    std::wstring& datetime ();
    std::wstring const& string () const;
    std::wstring& string ();
    array_value_type const& array () const;
    array_value_type& array ();
    table_value_type const& table () const;
    table_value_type& table ();

private:
    variation mtag;
    union {
        bool mboolean;
        int64_t mfixnum;
        double mflonum;
        std::wstring mstring;
        array_value_type marray;
        table_value_type mtable;
    };
    void copy_data (value_type const& x);
    void move_data (value_type&& x);
    void destroy ();
};

value_type null ();
value_type boolean (bool const x);
value_type fixnum (int64_t const x);
value_type flonum (double const x);
value_type datetime (std::wstring const& x);
value_type datetime (std::wstring&& x);
value_type string (std::wstring const& x);
value_type string (std::wstring&& x);
value_type string ();
value_type array ();
value_type table ();

bool exists (setter_type const& setter);

}//namespace wjson

std::wostream& operator<< (std::wostream& out, wjson::setter_type const& setter);

