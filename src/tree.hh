#pragma once

#include <string>
#include <vector>

#include "json-struct.hh"
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
    inline operator bool() const { return !empty_left() && !left_right_same(); } // if transition is good for display
    friend inline std::ostream& operator<<(std::ostream& out, const AA_Transition& a) { return out << a.display_name(); }

    char left;
    char right;
    size_t pos;
    const Node* for_left;       // node used to set left part, for debugging transition labels

    friend inline auto json_fields(AA_Transition& a, bool for_output)
        {
            if (for_output && !a)
                throw json::no_value();
            return std::make_tuple("t", json::field(&a, &AA_Transition::display_name));
        }

}; // class AA_Transition

class AA_Transitions : public std::vector<AA_Transition>
{
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
              // return std::any_of(begin(), end(), std::mem_fn(&AA_Transition::operator bool));
            return std::any_of(begin(), end(), [](const auto& a) -> bool { return a; });
        }

    inline std::vector<std::pair<std::string, const Node*>> make_labels(bool show_empty_left = false) const
        {
            std::vector<std::pair<std::string, const Node*>> labels;
            for (const auto& aa_transition: *this) {
                if (show_empty_left || !aa_transition.empty_left()) {
                    labels.push_back(std::make_pair(aa_transition.display_name(), aa_transition.for_left));
                }
            }
            return labels;
        }

}; // class AA_Transitions

// ----------------------------------------------------------------------

class Node
{
 public:
    typedef std::vector<Node> Subtree;
    enum class LadderizeMethod { MaxEdgeLength, NumberOfLeaves };

    inline Node() : edge_length(0), line_no(0), number_strains(1), vertical_gap_before(0) {}
    inline Node(std::string aName, double aEdgeLength, const Date& aDate = Date()) : edge_length(aEdgeLength), name(aName), date(aDate), line_no(0), number_strains(1), vertical_gap_before(0) {}
    // inline Node(Node&&) = default;
    // inline Node(const Node&) = default;
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
    int months_from(const Date& aStart) const; // returns negative if date of the node is earlier than aStart

    std::string display_name() const;

      // aa transitions
    AA_Transitions aa_transitions;
    mutable double cumulative_edge_length;
    void remove_aa_transition(size_t aPos, char aRight, bool aDescentUponRemoval); // recursively
    void compute_cumulative_edge_length(double initial_edge_length, double& max_cumulative_edge_length) const;

      // for ladderizing
    double ladderize_max_edge_length;
    Date ladderize_max_date;
    std::string ladderize_max_name_alphabetically;

      // for hz line sections
    double edge_length_to_next;
    mutable size_t vertical_gap_before; // if this node is the beginning of the hz line section, we may need to add gap when enumerating lines

      // for matching hi names for signature page
    std::vector<std::string> hi_names;

    inline Node* find_path_to_first_leaf(std::vector<std::pair<size_t, Node*>>& path)
        {
            if (is_leaf()) {
                return this;
            }
            else {
                path.push_back(std::make_pair(0, this));
                return subtree.front().find_path_to_first_leaf(path);
            }
        }

 protected:
    void ladderize(LadderizeMethod aLadderizeMethod);
    bool find_name_r(std::string aName, std::vector<const Node*>& aPath) const;

    friend inline auto json_fields(Node& a)
        {
            return std::make_tuple(
                "aa_transitions", json::field(&a.aa_transitions, json::output_only_if_true),
                  // "?", json::comment("aa_transitions is for information only, ignored on reading and re-calculated"),
                "aa", json::field(&a.aa, json::output_if_not_empty),
                "clades", json::field(&a.clades, json::output_if_not_empty),
                "continent", json::field(&a.continent, json::output_if_not_empty),
                "date", json::field(&a.date, &Date::display, &Date::parse, json::output_if_true),
                "edge_length", &a.edge_length,
                "id", &a.branch_id,
                "name", json::field(&a.name, json::output_if_not_empty),
                "number_strains", &a.number_strains,
                "subtree", json::field(&a.subtree, json::output_if_not_empty)
                                   );
        }

}; // class Node

// ----------------------------------------------------------------------

class Tree : public Node
{
 public:
    inline Tree() : Node(), mMaxCumulativeEdgeLength(-1) {}

    void print(std::ostream& out) const;
    void print_edges(std::ostream& out) const;
    void fix_labels();
    std::pair<Date, Date> min_max_date() const;
    std::map<Date, size_t> sequences_per_month() const;
    std::pair<double, double> min_max_edge() const;
    std::pair<const Node*, const Node*> top_bottom_nodes_of_subtree(std::string branch_id) const;
    std::string virus_type() const { return mVirusType; }
    std::string lineage() const { return mLineage; }

    std::string json(int indent) const;
    static Tree* from_json(std::string data);

    void match_seqdb(const Seqdb& aSeqdb);
    void clade_setup();         // updates mSettings.clades with clade data from tree

    std::vector<std::string> names() const;
    std::vector<std::string> names_between(std::string first, std::string last, std::string isolated_after = std::string()) const;
    std::vector<const Node*> leaves() const;

      // aa transitions
    void make_aa_transitions();
    void make_aa_transitions(const std::vector<size_t>& aPositions);
    inline std::vector<const Node*> leaf_nodes_sorted_by_cumulative_edge_length() const
        {
            compute_cumulative_edge_length();
            return leaf_nodes_sorted_by([](const Node* a, const Node* b) -> bool { return a->cumulative_edge_length > b->cumulative_edge_length; });
        }

      // returns list of aa and its number of occurences at each pos found in the sequences of the tree
    std::vector<std::map<char, size_t>> aa_per_pos() const;

    void prepare_for_drawing();

    inline const Settings& settings() const { return mSettings; }
    inline Settings& settings() { return mSettings; }

      // finds leaf node with the passed name and returns path to that node, the first pointer in the path is &Tree, the last pointer in the path is the found node.
    std::vector<const Node*> find_name(std::string aName) const;
    const Node* find_node_by_name(std::string aName) const;
    const Node* find_next_leaf_node(const Node& aNode) const;
    const Node* find_previous_leaf_node(const Node& aNode) const;
    void re_root(const std::vector<const Node*>& aNewRoot);
      // re-roots tree making the parent of the leaf node with the passed name root
    void re_root(std::string aName);

    inline void compute_cumulative_edge_length() const
        {
            if (mMaxCumulativeEdgeLength < 0)
                Node::compute_cumulative_edge_length(0, mMaxCumulativeEdgeLength);
        }

    void preprocess_upon_importing_from_external_format();
    void ladderize(LadderizeMethod aLadderizeMethod);

      // hz line sections
    void make_hz_line_sections(double tolerance);
    inline std::vector<const Node*> leaf_nodes_sorted_by_edge_length_to_next() const // longest first!
        {
            compute_cumulative_edge_length();
            return leaf_nodes_sorted_by([](const Node* a, const Node* b) -> bool { return b->edge_length_to_next < a->edge_length_to_next; });
        }


    void add_vaccine(std::string aId, std::string aLabel);

      // number of lines in the tree
    size_t height() const;
      // biggest cumulative_edge_length
    inline double width() const { compute_cumulative_edge_length(); return mMaxCumulativeEdgeLength; }

 private:
    Settings mSettings;
    std::string mVirusType;     // set in match_seqdb
    std::string mLineage;       // set in match_seqdb
    mutable double mMaxCumulativeEdgeLength;

    size_t longest_aa() const;
    void set_branch_id();
    void set_line_no();
    void set_top_bottom();
    void init_hz_line_sections(bool reset = false);

    inline std::pair<Node*, std::vector<std::pair<size_t, Node*>>> find_path_to_first_leaf()
        {
            std::vector<std::pair<size_t, Node*>> path;
            Node* node = Node::find_path_to_first_leaf(path);
            return std::make_pair(node, path);
        }

      // returns next leaf node and common root for the passed node and the next node.
      // returns (nullptr, nullptr) if the last leaf node was passed
    std::pair<Node*,Node*> find_path_to_next_leaf(std::vector<std::pair<size_t, Node*>>& aPath);

    std::vector<const Node*> leaf_nodes_sorted_by(const std::function<bool(const Node*,const Node*)>& cmp) const;

      // changes between "phylogenetic-tree-v1" and "phylogenetic-tree-v2"
      // - settings.clades.per_clade[]: begin and end are names (of nodes) instead of line numbers, lines recomputed right before drawing
    static constexpr const char* TREE_JSON_DUMP_VERSION = "phylogenetic-tree-v2"; // program cannot read phylogenetic-tree-v1
    std::string mJsonDumpVersion = TREE_JSON_DUMP_VERSION;

    friend inline auto json_fields(Tree& a)
        {
            return std::make_tuple(
                "_", json::comment("-*- js-indent-level: 1 -*-"),
                "  version", &a.mJsonDumpVersion,
                "settings", &a.mSettings,
                "tree", static_cast<Node*>(&a)
                                   );
        }

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

inline Node& find_first_leaf(Node& aNode)
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
