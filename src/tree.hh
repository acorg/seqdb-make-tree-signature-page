#pragma once

#include <string>
#include <vector>

#include "json-write.hh"
#include "json-read.hh"
#include "date.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Seqdb;
class Node;

// ----------------------------------------------------------------------

class AA_Transition
{
 public:
    static constexpr const char Empty = ' ';
    inline AA_Transition() : left(Empty), right(Empty), pos(9999), for_left(nullptr) {}
    inline AA_Transition(size_t aPos, char aRight) : left(Empty), right(aRight), pos(aPos), for_left(nullptr) {}
    inline std::string display_name() const { return std::string(1, left) + std::to_string(pos + 1) + std::string(1, right); }
    inline bool empty_left() const { return left == Empty; }
    inline bool left_right_same() const { return left == right; }
    friend inline std::ostream& operator<<(std::ostream& out, const AA_Transition& a) { return out << a.display_name(); }

    char left;
    char right;
    size_t pos;
    const Node* for_left;       // node used to set left part, for debugging transition labels

}; // class AA_Transition

class AA_Transitions : public std::vector<AA_Transition>
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(AA_Transitions& a) : mAA_Transitions(a) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                  // AA_Transitions ignored! they will be re-calculated.
                using namespace jsonr;
                auto item = object(object_string_ignore_value("t") | object_string_ignore_value("n"));
                return (array_begin > ~(item & *(comma > item) ) > array_end)(i1, i2);
            }

          private:
            AA_Transitions& mAA_Transitions;
        };

 public:
    inline void add(size_t aPos, char aRight) { emplace_back(aPos, aRight); }

      // returns if anything was removed
    inline bool remove(size_t aPos)
        {
            auto start = std::remove_if(begin(), end(), [=](const auto& e) { return e.pos == aPos; });
            bool anything_to_remove = start != end();
            if (anything_to_remove)
                erase(start, end());
            return anything_to_remove;
        }

      // returns if anything was removed
    inline bool remove(size_t aPos, char aRight)
        {
            auto start = std::remove_if(begin(), end(), [=](const auto& e) { return e.pos == aPos && e.right == aRight; });
            bool anything_to_remove = start != end();
            if (anything_to_remove) {
                erase(start, end());
            }
            return anything_to_remove;
        }

    inline const AA_Transition* find(size_t aPos) const
        {
            const auto found = std::find_if(begin(), end(), [=](const auto& e) { return e.pos == aPos; });
            return found == end() ? nullptr : &*found;
        }

    inline operator bool() const
        {
            return std::any_of(begin(), end(), [](const auto& a) -> bool { return !a.empty_left() && !a.left_right_same(); });
        }

      // serialize
    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;
    inline auto json_parser() { return json_parser_t(*this); }

}; // class AA_Transitions

// ----------------------------------------------------------------------

class Node
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(Node& aNode) : mNode(aNode) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return object(
                    object_value("subtree", mNode.subtree)
                  | object_value("id", mNode.branch_id)
                  | object_value("clades", mNode.clades)
                  | object_value("continent", mNode.continent)
                  | object_value("edge_length", mNode.edge_length)
                  | object_value("name", mNode.name)
                  | object_value("number_strains", mNode.number_strains)
                  | (skey("aa_transitions") > mNode.aa_transitions.json_parser()) // must be before "aa"!
                  | object_value("aa", mNode.aa)
                  | object_string_value("date", mNode.date)
                  | object_string_ignore_value("?")
                    )(i1, i2);
            }

          private:
            Node& mNode;
        };

 public:
    typedef std::vector<Node> Subtree;

    inline Node() : edge_length(0), line_no(0), number_strains(1) {}
    // inline Node(Node&&) = default;
    // inline Node(const Node&) = default;
    inline Node(std::string aName, double aEdgeLength, const Date& aDate = Date()) : edge_length(aEdgeLength), name(aName), date(aDate), line_no(0), number_strains(1) {}
    // inline Node& operator=(Node&&) = default; // needed for swap needed for sort

    double edge_length;              // indent of node or subtree
    std::string name;                // node name or branch annotation

      // leaf part
    Date date;
    size_t line_no;             // line at which the name is drawn
    std::string aa;             // aligned AA sequence for coloring by subst
                                // for subtree: for each pos: space - children have different aa's at this pos (X not counted), letter - all children have the same aa at this pos (X not counted)

      // for coloring
    std::string continent;
    std::vector<std::string> clades;

      // subtree part
    Subtree subtree;
    double top, bottom;         // subtree boundaries
    size_t number_strains;
    std::string branch_id;

    inline bool is_leaf() const { return subtree.empty() && !name.empty(); }
    inline double middle() const { return is_leaf() ? static_cast<double>(line_no) : ((top + bottom) / 2.0); }
    std::pair<double, size_t> width_height() const;
    int months_from(const Date& aStart) const; // returns negative if date of the node is earlier than aStart

    std::string display_name() const;

      // aa transitions
    AA_Transitions aa_transitions;
    mutable double cumulative_edge_length;
    void remove_aa_transition(size_t aPos, char aRight, bool aDescentUponRemoval); // recursively
    void compute_cumulative_edge_length(double initial_edge_length = 0) const;

      // for ladderizing
    double ladderize_max_edge_length;
    Date ladderize_max_date;
    std::string ladderize_max_name_alphabetically;

      // serialize
    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;
    inline auto json_parser() { return json_parser_t(*this); }

 protected:
    void ladderize();
    bool find_name_r(std::string aName, std::vector<const Node*>& aPath) const;

}; // class Node

// ----------------------------------------------------------------------

class Tree : public Node
{
 public:
    void analyse();
    void print(std::ostream& out) const;
    void print_edges(std::ostream& out) const;
    void fix_labels();
    std::pair<Date, Date> min_max_date() const;
    std::pair<double, double> min_max_edge() const;
    std::pair<const Node*, const Node*> top_bottom_nodes_of_subtree(std::string branch_id) const;
    std::string virus_type() const { return mVirusType; }
    std::string lineage() const { return mLineage; }

    using Node::json;
    std::string json(size_t indent) const;

    static Tree from_json(std::string data);

    void match_seqdb(const Seqdb& aSeqdb);
    void clade_setup();         // updates mSettings.clades with clade data from tree

    std::vector<std::string> names() const;

      // aa transitions
    void make_aa_transitions();
    void make_aa_transitions(const std::vector<size_t>& aPositions);
    std::vector<const Node*> leaf_nodes_sorted_by_cumulative_edge_length() const;

      // returns list of aa and its number of occurences at each pos found in the sequences of the tree
    std::vector<std::map<char, size_t>> aa_per_pos() const;

    void prepare_for_drawing();

    inline const Settings& settings() const { return mSettings; }
    inline Settings& settings() { return mSettings; }

      // finds leaf node with the passed name and returns path to that node, the first pointer in the path is &Tree, the last pointer in the path is the found node.
    std::vector<const Node*> find_name(std::string aName) const;
    void re_root(const std::vector<const Node*>& aNewRoot);
      // re-roots tree making the parent of the leaf node with the passed name root
    void re_root(std::string aName);

    void preprocess_upon_importing_from_external_format();
    void ladderize();

 private:
    Settings mSettings;
    std::string mVirusType;     // set in match_seqdb
    std::string mLineage;       // set in match_seqdb

    size_t longest_aa() const;
    void set_branch_id();

}; // class Tree

// ----------------------------------------------------------------------

template <typename N, typename F1> inline void iterate_leaf(N& aNode, F1 f_name)
{
    if (aNode.is_leaf()) {
        f_name(aNode);
    }
    else {
        for (auto& node: aNode.subtree) {
            iterate_leaf(node, f_name);
        }
    }
}

template <typename N, typename F1, typename F3> inline void iterate_leaf_post(N& aNode, F1 f_name, F3 f_subtree_post)
{
    if (aNode.is_leaf()) {
        f_name(aNode);
    }
    else {
        for (auto& node: aNode.subtree) {
            iterate_leaf_post(node, f_name, f_subtree_post);
        }
        f_subtree_post(aNode);
    }
}

template <typename N, typename F1, typename F2> inline void iterate_leaf_pre(N& aNode, F1 f_name, F2 f_subtree_pre)
{
    if (aNode.is_leaf()) {
        f_name(aNode);
    }
    else {
        f_subtree_pre(aNode);
        for (auto& node: aNode.subtree) {
            iterate_leaf_pre(node, f_name, f_subtree_pre);
        }
    }
}

// Stop descending the tree if f_subtree_pre returned false
template <typename N, typename F1, typename F2> inline void iterate_leaf_pre_stop(N& aNode, F1 f_name, F2 f_subtree_pre)
{
    if (aNode.is_leaf()) {
        f_name(aNode);
    }
    else {
        if (f_subtree_pre(aNode)) {
            for (auto& node: aNode.subtree) {
                iterate_leaf_pre_stop(node, f_name, f_subtree_pre);
            }
        }
    }
}

template <typename N, typename F3> inline void iterate_pre(N& aNode, F3 f_subtree_pre)
{
    if (!aNode.is_leaf()) {
        f_subtree_pre(aNode);
        for (auto& node: aNode.subtree) {
            iterate_pre(node, f_subtree_pre);
        }
    }
}

template <typename N, typename F3> inline void iterate_post(N& aNode, F3 f_subtree_post)
{
    if (!aNode.is_leaf()) {
        for (auto& node: aNode.subtree) {
            iterate_post(node, f_subtree_post);
        }
        f_subtree_post(aNode);
    }
}

template <typename N, typename F1, typename F2, typename F3> inline void iterate_leaf_pre_post(N& aNode, F1 f_name, F2 f_subtree_pre, F3 f_subtree_post)
{
    if (aNode.is_leaf()) {
        f_name(aNode);
    }
    else {
        f_subtree_pre(aNode);
        for (auto node = aNode.subtree.begin(); node != aNode.subtree.end(); ++node) {
            iterate_leaf_pre_post(*node, f_name, f_subtree_pre, f_subtree_post);
        }
        f_subtree_post(aNode);
    }
}

// ----------------------------------------------------------------------

template <typename P> inline const Node* find_node(const Node& aNode, P predicate)
{
    const Node* r = nullptr;
    if (predicate(aNode)) {
        r = &aNode;
    }
    else if (! aNode.is_leaf()) {
        for (auto node = aNode.subtree.begin(); r == nullptr && node != aNode.subtree.end(); ++node) {
            r = find_node(*node, predicate);
        }
    }
    return r;
}

// ----------------------------------------------------------------------

inline const Node& find_first_leaf(const Node& aNode)
{
    return aNode.is_leaf() ? aNode : find_first_leaf(aNode.subtree.front());
}

inline const Node& find_last_leaf(const Node& aNode)
{
    return aNode.is_leaf() ? aNode : find_last_leaf(aNode.subtree.back());
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
