#include <vector>
#include <map>
#include <string>
#include <utility>
#include <stdexcept>
#include "value.hpp"

namespace wjson {

setter_type::setter_type (value_type& value, std::size_t idx)
    : mvalue (value), mpath ()
{
    mpath.push_back ({VALUE_ARRAY, idx, L""});
}

setter_type::setter_type (value_type& value, std::wstring const& k)
    : mvalue (value), mpath ()
{
    mpath.push_back ({VALUE_TABLE, 0, k});
}

setter_type&
setter_type::operator[] (value_type const& k)
{
    if (k.tag () != VALUE_STRING)
        throw std::out_of_range ("setter::[](value): string only");
    mpath.push_back ({VALUE_TABLE, 0, k.string ()});
    return *this;
}

setter_type&
setter_type::operator[] (std::size_t idx)
{
    mpath.push_back ({VALUE_ARRAY, idx, L""});
    return *this;
}

setter_type&
setter_type::operator[] (std::wstring const& k)
{
    mpath.push_back ({VALUE_TABLE, 0, k});
    return *this;
}

setter_type&
setter_type::operator= (value_type const& x)
{
    value_type* node = force ();
    if (VALUE_ARRAY == mpath.back ().mtag)
        node->set (mpath.back ().midx, x);
    else if (VALUE_TABLE == mpath.back ().mtag)
        node->set (mpath.back ().mkey, x);
    return *this;
}

setter_type&
setter_type::operator= (value_type&& x)
{
    value_type* node = force ();
    if (VALUE_ARRAY == mpath.back ().mtag)
        node->set (mpath.back ().midx, std::move (x));
    else if (VALUE_TABLE == mpath.back ().mtag)
        node->set (mpath.back ().mkey, std::move (x));
    return *this;
}

setter_type&
setter_type::operator= (std::wstring const& x)
{
    value_type* node = force ();
    if (VALUE_ARRAY == mpath.back ().mtag)
        node->set (mpath.back ().midx, std::move(::wjson::string (x)));
    else if (VALUE_TABLE == mpath.back ().mtag)
        node->set (mpath.back ().mkey, std::move(::wjson::string (x)));
    return *this;
}

setter_type&
setter_type::operator= (std::wstring&& x)
{
    value_type* node = force ();
    if (VALUE_ARRAY == mpath.back ().mtag)
        node->set (mpath.back ().midx, std::move(::wjson::string (std::move (x))));
    else if (VALUE_TABLE == mpath.back ().mtag)
        node->set (mpath.back ().mkey, std::move(::wjson::string (std::move (x))));
    return *this;
}

variation
setter_type::tag () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        return VALUE_NULL;
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).tag ();
    else
        return node->get (mpath.back ().mkey).tag ();
}

bool const&
setter_type::boolean () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::boolean(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).boolean ();
    else
        return node->get (mpath.back ().mkey).boolean ();
}

int64_t const&
setter_type::fixnum () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::fixnum(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).fixnum ();
    else
        return node->get (mpath.back ().mkey).fixnum ();
}

double const&
setter_type::flonum () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::flonum(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).flonum ();
    else
        return node->get (mpath.back ().mkey).flonum ();
}

std::wstring const&
setter_type::datetime () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::datetime(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).datetime ();
    else
        return node->get (mpath.back ().mkey).datetime ();
}

std::wstring const&
setter_type::string () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::string(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).string ();
    else
        return node->get (mpath.back ().mkey).string ();
}

array_value_type const&
setter_type::array () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::array(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).array ();
    else
        return node->get (mpath.back ().mkey).array ();
}

table_value_type const&
setter_type::table () const
{
    value_type* node = lookup ();
    if (node == nullptr)
        throw std::out_of_range ("const setter_type::table(x): not exists");
    if (VALUE_ARRAY == mpath.back ().mtag)
        return node->get (mpath.back ().midx).table ();
    else
        return node->get (mpath.back ().mkey).table ();
}

value_type*
setter_type::force ()
{
    value_type* node = &mvalue;
    for (std::size_t i = 0; i + 1 < mpath.size (); ++i) {
        if (VALUE_ARRAY == mpath.at (i).mtag && VALUE_ARRAY == node->tag ()) {
            if (mpath.at (i).midx >= node->array ().size ()) {
                if (VALUE_ARRAY == mpath. at(i + 1).mtag)
                    node->set (mpath.at (i).midx, wjson::array ());
                else
                    node->set (mpath.at (i).midx, wjson::table ());
            }
            value_type& e = node->array ().at (mpath.at (i).midx);
            node = &e;
        }
        else if (VALUE_TABLE == mpath.at (i).mtag && VALUE_TABLE == node->tag ()) {
            if (node->table ().count (mpath.at (i).mkey) == 0) {
                if (VALUE_ARRAY == mpath.at (i + 1).mtag)
                    node->set (mpath.at (i).mkey, wjson::array ());
                else
                    node->set (mpath.at (i).mkey, wjson::table ());                
            }
            value_type& e = node->table ().at (mpath.at (i).mkey);
            node = &e;
        }
        else
            throw std::out_of_range ("setter_type::force (): type mismatch");
    }
    return node;
}

value_type*
setter_type::lookup () const
{
    value_type* node = &mvalue;
    for (std::size_t i = 0; i + 1 < mpath.size (); ++i) {
        if (VALUE_ARRAY == mpath.at (i).mtag && VALUE_ARRAY == node->tag ()) {
            if (mpath.at (i).midx >= node->array ().size ())
                return nullptr;
            value_type& e = node->array ().at (mpath.at (i).midx);
            node = &e;
        }
        else if (VALUE_TABLE == mpath.at (i).mtag && VALUE_TABLE == node->tag ()) {
            if (node->table ().count (mpath.at (i).mkey) == 0)
                return nullptr;
            value_type& e = node->table ().at (mpath.at (i).mkey);
            node = &e;
        }
        else
            throw std::out_of_range ("setter_type::force (): type mismatch");
    }
    return node;
}

bool
exists (setter_type const& setter)
{
    return setter.lookup () != nullptr; 
}

}//namespace wjson

std::wostream&
operator<< (std::wostream& out, wjson::setter_type const& setter)
{
    out << setter.string ();
    return out;
}
