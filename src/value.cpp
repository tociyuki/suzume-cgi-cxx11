#include <vector>
#include <map>
#include <string>
#include <utility>
#include <cmath>
#include "value.hpp"

namespace wjson {

value_type::value_type () : mtag (VALUE_NULL)
{
    mboolean = false;
}

value_type::value_type (value_type const& x) : mtag (x.mtag)
{
    copy_data (x);
}

value_type::value_type (value_type&& x) : mtag (x.mtag)
{
    move_data (std::move (x));
}

value_type::~value_type ()
{
    destroy ();
}

value_type&
value_type::operator=(value_type const& x)
{
    if (this != &x) {
        destroy ();
        mtag = x.mtag;
        copy_data (x);
    }
    return *this;
}

value_type&
value_type::operator=(value_type&& x)
{
    if (this != &x) {
        destroy ();
        mtag = x.mtag;
        move_data (std::move (x));
    }
    return *this;
}

value_type&
value_type::assign_null ()
{
    destroy ();
    mtag = VALUE_NULL;
    mboolean = false;
    return *this;
}

value_type&
value_type::assign_boolean (bool const x)
{
    destroy ();
    mtag = VALUE_BOOLEAN;
    mboolean = x;
    return *this;
}

value_type&
value_type::assign_fixnum (int64_t const x)
{
    destroy ();
    mtag = VALUE_FIXNUM;
    mfixnum = x;
    return *this;
}

value_type&
value_type::assign_flonum (double const x)
{
    if (std::isnan (x))
        throw std::out_of_range ("value_type(double): nan invalid.");
    if (std::isinf (x))
        throw std::out_of_range ("value_type(double): inf invalid.");
    destroy ();
    mtag = VALUE_FLONUM;
    mflonum = x;
    return *this;
}

value_type&
value_type::assign_datetime (std::wstring const& x)
{
    destroy ();
    mtag = VALUE_DATETIME;
    new (&mstring) std::wstring (x);
    return *this;    
}

value_type&
value_type::assign_datetime (std::wstring&& x)
{
    destroy ();
    mtag = VALUE_DATETIME;
    new (&mstring) std::wstring (std::move (x));
    return *this;    
}

value_type&
value_type::assign_string (std::wstring const& x)
{
    destroy ();
    mtag = VALUE_STRING;
    new (&mstring) std::wstring (x);
    return *this;    
}

value_type&
value_type::assign_string (std::wstring&& x)
{
    destroy ();
    mtag = VALUE_STRING;
    new (&mstring) std::wstring (std::move (x));
    return *this;    
}

value_type&
value_type::assign_array (array_value_type const& x)
{
    destroy ();
    mtag = VALUE_ARRAY;
    new (&marray) array_value_type (x);
    return *this;    
}

value_type&
value_type::assign_array (array_value_type&& x)
{
    destroy ();
    mtag = VALUE_ARRAY;
    new (&marray) array_value_type (std::move (x));
    return *this;    
}

value_type&
value_type::assign_table (table_value_type const& x)
{
    destroy ();
    mtag = VALUE_TABLE;
    new (&mtable) table_value_type (x);
    return *this;    
}

value_type&
value_type::assign_table (table_value_type&& x)
{
    destroy ();
    mtag = VALUE_TABLE;
    new (&mtable) table_value_type (std::move (x));
    return *this;
}

setter_type
value_type::operator[] (value_type const& k)
{
    if (mtag == VALUE_TABLE && k.mtag == VALUE_STRING)
        return setter_type (*this, k.mstring);
    throw std::out_of_range ("value_type::[](value)const: invalid");
}

setter_type
value_type::operator[] (std::size_t idx)
{
   if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::[](idx)const: not array");
    return setter_type (*this, idx);
}

setter_type
value_type::operator[] (std::wstring const& key)
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("value_type::[](key)const: not table");
    return setter_type (*this, key);
}

value_type const&
value_type::get (value_type const& k) const
{
    if (mtag == VALUE_TABLE && k.mtag == VALUE_STRING)
        return mtable.at (k.mstring);
    throw std::out_of_range ("value_type::get(value)const: invalid");    
}

value_type&
value_type::get (value_type const& k)
{
    if (mtag == VALUE_TABLE && k.mtag == VALUE_STRING)
        return mtable.at (k.mstring);
    throw std::out_of_range ("value_type::get(value)const: invalid");    
}

value_type const&
value_type::get (std::size_t const idx) const
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::get(idx)const: not array");
    return marray.at (idx);
}

value_type&
value_type::get (std::size_t const idx)
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::get(idx): not array");
    return marray.at (idx);
}

value_type const&
value_type::get (std::wstring const& key) const
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("value_type::get(key)const: not table");
    return mtable.at (key);
}

value_type&
value_type::get (std::wstring const& key)
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("value_type::get(key): not table");
    return mtable.at (key);
}

value_type&
value_type::set (value_type const& k, value_type const& x)
{
    if (mtag == VALUE_TABLE && k.mtag == VALUE_STRING)
        return set (k.mstring, x);
    throw std::out_of_range ("value_type::set(value,const&x)const: invalid");    
}

value_type&
value_type::set (value_type const& k, value_type&& x)
{
    if (mtag == VALUE_TABLE && k.mtag == VALUE_STRING)
        return set (k.mstring, std::move (x));
    throw std::out_of_range ("value_type::set(value,&&x)const: invalid");    
}

value_type&
value_type::set (std::size_t const idx, value_type const& x)
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::set(idx,const&x): not array");
    if (idx >= marray.size ())
        marray.resize (idx + 1);
    marray[idx] = x;
    return *this;
}

value_type&
value_type::set (std::size_t const idx, value_type&& x)
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::set(idx,&&x): not array");
    if (idx >= marray.size ())
        marray.resize (idx + 1);
    std::swap (marray[idx], x);
    return *this;
}

value_type&
value_type::push_back (value_type const& x)
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::set(idx,const&x): not array");
    marray.push_back (x);
    return *this;
}

value_type&
value_type::push_back (value_type&& x)
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("value_type::set(idx,&&x): not array");
    marray.push_back (std::move (x));
    return *this;
}

value_type&
value_type::set (std::wstring const& key, value_type const& x)
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("value_type::set(key,const&x): not array");
    mtable[key] = x;
    return *this;
}

value_type&
value_type::set (std::wstring const& key, value_type&& x)
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("value_type::set(key,&&x): not array");
    std::swap (mtable[key], x);
    return *this;
}

void
value_type::swap (value_type& x)
{
    if (this != &x) {
        value_type tmp (std::move (*this));
        mtag = x.mtag;
        move_data (std::move (x));
        x = std::move (tmp);
    }
}

variation
value_type::tag () const
{
    return mtag;
}

std::size_t
value_type::size () const
{
    switch (mtag) {
    case VALUE_DATETIME: return mstring.size ();
    case VALUE_STRING: return mstring.size ();
    case VALUE_ARRAY:  return marray.size ();
    case VALUE_TABLE:  return mtable.size ();
    default: return 0;
    }
}

bool const&
value_type::boolean () const
{
    if (mtag != VALUE_BOOLEAN)
        throw std::out_of_range ("boolean()const: not boolean");
    return mboolean;
}

bool&
value_type::boolean ()
{
    if (mtag != VALUE_BOOLEAN)
        throw std::out_of_range ("boolean(): not boolean");
    return mboolean;
}

int64_t const&
value_type::fixnum () const
{
    if (mtag != VALUE_FIXNUM)
        throw std::out_of_range ("fixnum()const: not fixnum");
    return mfixnum;
}

int64_t&
value_type::fixnum ()
{
    if (mtag != VALUE_FIXNUM)
        throw std::out_of_range ("fixnum(): not fixnum");
    return mfixnum;
}

double const&
value_type::flonum () const
{
    if (mtag != VALUE_FLONUM)
        throw std::out_of_range ("flonum()const: not flonum");
    return mflonum;
}

double&
value_type::flonum ()
{
    if (mtag != VALUE_FLONUM)
        throw std::out_of_range ("flonum(): not flonum");
    return mflonum;
}

std::wstring const&
value_type::datetime () const
{
    if (mtag != VALUE_DATETIME)
        throw std::out_of_range ("datetime()const: not datetime");
    return mstring;
}

std::wstring&
value_type::datetime ()
{
    if (mtag != VALUE_DATETIME)
        throw std::out_of_range ("datetime(): not datetime");
    return mstring;
}

std::wstring const&
value_type::string () const
{
    if (mtag != VALUE_STRING)
        throw std::out_of_range ("string()const: not string");
    return mstring;
}

std::wstring&
value_type::string ()
{
    if (mtag != VALUE_STRING)
        throw std::out_of_range ("string(): not string");
    return mstring;
}

array_value_type const&
value_type::array () const
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("array()const: not array");
    return marray;
}

array_value_type&
value_type::array ()
{
    if (mtag != VALUE_ARRAY)
        throw std::out_of_range ("array(): not array");
    return marray;
}

table_value_type const&
value_type::table () const
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("table()const: not table");
    return mtable;
}

table_value_type&
value_type::table ()
{
    if (mtag != VALUE_TABLE)
        throw std::out_of_range ("table(): not table");
    return mtable;
}

void
value_type::copy_data (value_type const& x)
{
    switch (mtag) {
    case VALUE_NULL:
    case VALUE_BOOLEAN:
        mboolean = x.mboolean;
        break;
    case VALUE_FIXNUM:
        mfixnum = x.mfixnum;
        break;
    case VALUE_FLONUM:
        mflonum = x.mflonum;
        break;
    case VALUE_DATETIME:
    case VALUE_STRING:
        new (&mstring) std::wstring (x.mstring);
        break;
    case VALUE_ARRAY:
        new (&marray) array_value_type (x.marray);
        break;
    case VALUE_TABLE:
        new (&mtable) table_value_type (x.mtable);
        break;
    }
}

void
value_type::move_data (value_type&& x)
{
    switch (mtag) {
    case VALUE_NULL:
    case VALUE_BOOLEAN:
        mboolean = x.mboolean;
        break;
    case VALUE_FIXNUM:
        mfixnum = x.mfixnum;
        break;
    case VALUE_FLONUM:
        mflonum = x.mflonum;
        break;
    case VALUE_DATETIME:
    case VALUE_STRING:
        new (&mstring) std::wstring (std::move (x.mstring));
        break;
    case VALUE_ARRAY:
        new (&marray) array_value_type (std::move (x.marray));
        break;
    case VALUE_TABLE:
        new (&mtable) table_value_type (std::move (x.mtable));
        break;
    }
}

void
value_type::destroy ()
{
    switch (mtag) {
    case VALUE_NULL:
    case VALUE_BOOLEAN:
    case VALUE_FIXNUM:
    case VALUE_FLONUM:
        break;
    case VALUE_DATETIME:
    case VALUE_STRING:
        mstring.~basic_string ();
        break;
    case VALUE_ARRAY:
        marray.~vector ();
        break;
    case VALUE_TABLE:
        mtable.~map ();
        break;
    }
}

value_type
null ()
{
    value_type e;
    return e;
}

value_type
boolean (bool const x)
{
    value_type e;
    e.assign_boolean (x);
    return e;
}

value_type
fixnum (int64_t const x)
{
    value_type e;
    e.assign_fixnum (x);
    return e;
}

value_type
flonum (double const x)
{
    value_type e;
    e.assign_flonum (x);
    return e;
}

value_type
datetime (std::wstring const& x)
{
    value_type e;
    e.assign_datetime (x);
    return std::move (e);
}

value_type
datetime (std::wstring&& x)
{
    value_type e;
    e.assign_datetime (std::move (x));
    return std::move (e);
}

value_type
string (std::wstring const& x)
{
    value_type e;
    e.assign_string (x);
    return std::move (e);
}

value_type
string (std::wstring&& x)
{
    value_type e;
    e.assign_string (std::move (x));
    return std::move (e);
}

value_type
string ()
{
    value_type e;
    e.assign_string (L"");
    return std::move (e);
}

value_type
array ()
{
    value_type e;
    e.assign_array ({});
    return std::move (e);
}

value_type
table ()
{
    value_type e;
    e.assign_table ({});
    return std::move (e);
}

}//namespace wjson
