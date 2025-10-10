#pragma once

#include "cell.h"

#include <vector>

#include <boost/tokenizer.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

namespace qi      = boost::spirit::qi;
namespace phoenix = boost::phoenix;

typedef boost::spirit::line_pos_iterator<std::string::const_iterator> pos_iterator_t;

typedef std::vector<cell> row;
typedef std::vector<cell> cells;

BOOST_FUSION_ADAPT_STRUCT(
    cell,
    (Type, type)
    (Color, color)
)

template<typename It>
struct annotation_f {
    typedef void result_type;

    annotation_f(It first) : first(first) {}
    It const first;

    template<typename Val, typename First, typename Last>
    void operator()(Val& v, First f, Last l) const {
        do_annotate(v, f, l, first);
    }

  private:
    void static do_annotate(cell& c, It f, It l, It first) {
        c.row    = get_line(f) - 1;
        c.column = get_column(first, f) / 2;
    }
    static void do_annotate(...) { std::cerr << "(not having LocationInfo)\n"; }
};

template <typename It>
    struct MinigridParser : qi::grammar<It, row()>
{
  MinigridParser(It first) : MinigridParser::base_type(row_), annotate(first)
  {
    using namespace qi;
    type_.add
      ("W", Type::Wall)
      (" ", Type::Floor)
      ("D", Type::Door)
      ("L", Type::LockedDoor)
      ("K", Type::Key)
      ("A", Type::Ball)
      ("B", Type::Box)
      ("G", Type::Goal)
      ("V", Type::Lava)
      ("n", Type::SlipperyNorth)
      ("e", Type::SlipperyEast)
      ("s", Type::SlipperySouth)
      ("w", Type::SlipperyWest)
      ("a", Type::SlipperyNorthWest)
      ("b", Type::SlipperyNorthEast)
      ("c", Type::SlipperySouthWest)
      ("d", Type::SlipperySouthEast)
      ("X", Type::Agent)
      ("Z", Type::Adversary);
    color_.add
      ("R", Color::Red)
      ("G", Color::Green)
      ("B", Color::Blue)
      ("P", Color::Purple)
      ("Y", Color::Yellow)
      ("W", Color::White)
      (" ", Color::None);

    cell_ = type_ > color_;

    row_ = (cell_ % -qi::char_("\n"));

    auto set_location_info = annotate(_val, _1, _3);
    on_success(cell_, set_location_info);

    BOOST_SPIRIT_DEBUG_NODE(type_);
    BOOST_SPIRIT_DEBUG_NODE(color_);
    BOOST_SPIRIT_DEBUG_NODE(cell_);
  }

  private:
    phoenix::function<annotation_f<It>> annotate;

    qi::symbols<char, Type>  type_;
    qi::symbols<char, Color> color_;

    qi::rule<It, cell()> cell_;
    qi::rule<It, row()>  row_;
};

typedef boost::tokenizer< boost::escaped_list_separator<char> , std::string::const_iterator, std::string> Tokenizer;
//std::ostream& operator<<(std::ostream& os, const row& r);
