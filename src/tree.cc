#include <fstream>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <map>
#include <list>
#include <stack>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <numeric>

#include "tree.hh"
#include "seqdb.hh"
#include "xz.hh"
#include "float.hh"
#include "string.hh"
#include "stream.hh"
#include "draw-clades.hh"

// ----------------------------------------------------------------------

std::pair<double, size_t> Node::width_height() const
{
    size_t height = 0;
    double width = 0;
    if (is_leaf()) {
        ++height;
    }
    else {
        for (auto node = subtree.begin(); node != subtree.end(); ++node) {
            auto const wh = node->width_height();
            if (wh.first > width)
                width = wh.first;
            height += wh.second;
        }
    }
    return std::make_pair(width + edge_length, height);

} // Node::width_height

// ----------------------------------------------------------------------

int Node::months_from(const Date& aStart) const
{
    return date.empty() ? -1 : months_between_dates(aStart, date);

} // Node::month_no

// ----------------------------------------------------------------------

std::string Node::display_name() const
{
    if (is_leaf()) {
        auto fields = string::split(name, "__", string::Split::RemoveEmpty);
        auto r = fields[0];
        if (fields.size() > 1) {
            r.append(" " + fields[1]);
            if (fields.size() > 2) {
                r.append(" (distinct seqeuence " + fields[2] + ")");
            }
        }
        if (!date.empty()) {
            r.append(" ");
            r.append(date);
        }
        return r;
    }
    else {
        throw std::runtime_error("Node is not a name node");
    }

} // Node::display_name

// ----------------------------------------------------------------------

void Node::ladderize()
{
    auto set_max_edge = [](Node& aNode) {
        aNode.ladderize_max_edge_length = aNode.edge_length;
        aNode.ladderize_max_date = aNode.date;
        aNode.ladderize_max_name_alphabetically = aNode.name;
    };

    auto compute_max_edge = [](Node& aNode) {
        auto const max_subtree_edge_node = std::max_element(aNode.subtree.begin(), aNode.subtree.end(), [](auto const& a, auto const& b) { return a.ladderize_max_edge_length < b.ladderize_max_edge_length; });
        aNode.ladderize_max_edge_length = aNode.edge_length + max_subtree_edge_node->ladderize_max_edge_length;
        aNode.ladderize_max_date = std::max_element(aNode.subtree.begin(), aNode.subtree.end(), [](auto const& a, auto const& b) { return a.ladderize_max_date < b.ladderize_max_date; })->ladderize_max_date;
        aNode.ladderize_max_name_alphabetically = std::max_element(aNode.subtree.begin(), aNode.subtree.end(), [](auto const& a, auto const& b) { return a.ladderize_max_name_alphabetically < b.ladderize_max_name_alphabetically; })->ladderize_max_name_alphabetically;
    };

      // set max_edge_length field for every node
    iterate_leaf_post(*this, set_max_edge, compute_max_edge);

    auto reorder_subtree_cmp = [](const Node& a, const Node& b) -> bool {
        bool r = false;
        if (float_equal(a.ladderize_max_edge_length, b.ladderize_max_edge_length)) {
            if (a.ladderize_max_date == b.ladderize_max_date) {
                r = a.ladderize_max_name_alphabetically < b.ladderize_max_name_alphabetically;
            }
            else {
                r = a.ladderize_max_date < b.ladderize_max_date;
            }
        }
        else {
            r = a.ladderize_max_edge_length < b.ladderize_max_edge_length;
        }
        return r;
    };

      // re-order subtree based on max_edge_length of its nodes
    auto reorder_subtree = [&reorder_subtree_cmp](Node& aNode) {
        std::sort(aNode.subtree.begin(), aNode.subtree.end(), reorder_subtree_cmp);
    };
    iterate_post(*this, reorder_subtree);

} // Node::ladderize

// ----------------------------------------------------------------------

void Tree::ladderize()
{
    Node::ladderize();
    set_branch_id();
    set_line_no();
    set_top_bottom();
    init_hz_line_sections();

} // Tree::ladderize

// ----------------------------------------------------------------------

void Tree::preprocess_upon_importing_from_external_format()
{
    auto set_number_strains = [](Node& aNode) {
        aNode.number_strains = 0;
        for (const auto& subnode: aNode.subtree) {
            aNode.number_strains += subnode.number_strains;
        }
    };
    iterate_post(*this, set_number_strains);

    set_branch_id();
    set_line_no();
    set_top_bottom();

} // Tree::preprocess_upon_importing_from_external_format

// ----------------------------------------------------------------------

void Tree::set_branch_id()
{
    auto set_branch_id = [](Node& aNode) {
        std::string prefix = aNode.branch_id;
        if (!prefix.empty())
            prefix += ":";
        for (size_t i = 0; i < aNode.subtree.size(); ++i) {
            aNode.subtree[i].branch_id = prefix + std::to_string(i + 1);
        }
    };
    iterate_pre(*this, set_branch_id);

} // Tree::set_branch_id

// ----------------------------------------------------------------------

void Tree::init_hz_line_sections(bool reset)
{
    auto& hz_line_sections = settings().draw_tree.hz_line_sections;
    if (reset)
        hz_line_sections.clear();
    if (hz_line_sections.empty()) {
        const auto first = find_first_leaf(*this);
          // const auto last = find_last_leaf(*this);
        hz_line_sections.emplace_back(first.name, first.line_no, 0);
    }

} // Tree::init_hz_line_sections

// ----------------------------------------------------------------------

void Tree::set_line_no()
{
    size_t current_line = 0;
    auto set_line_no = [&current_line](Node& aNode) {
        aNode.line_no = current_line;
        ++current_line;
    };
    iterate_leaf(*this, set_line_no);

} // Tree::set_line_no

// ----------------------------------------------------------------------

void Tree::set_top_bottom()
{
    auto set_top_bottom = [](Node& aNode) {
        aNode.top = aNode.subtree.begin()->middle();
        aNode.bottom = aNode.subtree.rbegin()->middle();
    };
    iterate_post(*this, set_top_bottom);

} // Tree::set_top_bottom

// ----------------------------------------------------------------------

std::pair<Date, Date> Tree::min_max_date() const
{
    Date min_date, max_date;
    auto min_max_date = [&min_date, &max_date](const Node& aNode) -> void {
        if (!aNode.date.empty()) {
            if (min_date.empty() || aNode.date < min_date)
                min_date = aNode.date;
            if (max_date.empty() || max_date < aNode.date)
                max_date = aNode.date;
        }
    };
    iterate_leaf(*this, min_max_date);
    return std::make_pair(min_date, max_date);

} // Tree::min_max_date

// ----------------------------------------------------------------------

std::pair<double, double> Tree::min_max_edge() const
{
    double min_edge = 1e99, max_edge = 0.0;
    auto min_max_edge = [&min_edge, &max_edge](const Node& aNode) -> void {
        if (aNode.edge_length > 0.0) {
            if (aNode.edge_length < min_edge)
                min_edge = aNode.edge_length;
            if (max_edge < aNode.edge_length)
                max_edge = aNode.edge_length;
        }
    };
    iterate_leaf_pre(*this, min_max_edge, min_max_edge);
    return std::make_pair(min_edge, max_edge);

} // Tree::min_max_edge

// ----------------------------------------------------------------------

std::pair<const Node*, const Node*> Tree::top_bottom_nodes_of_subtree(std::string branch_id) const
{
    const Node* root = find_node(*this, [branch_id](const Node& aNode) -> bool { return aNode.branch_id == branch_id; });
    return std::make_pair(root != nullptr ? &find_first_leaf(*root) : nullptr, root != nullptr ? &find_last_leaf(*root) : nullptr);

} // Tree::top_bottom_nodes_of_subtree

// ----------------------------------------------------------------------

void Tree::print(std::ostream& out) const
{
    size_t indent = 0;
    auto p_name = [&out, &indent](const Node& aNode) {
        out << std::string(indent, ' ') << /* aNode.line_no << " -- " << */ aNode.display_name();
        if (aNode.edge_length >= 0)
            out << ':' << aNode.edge_length;
        out << std::endl;
    };
    auto p_subtree_pre = [&out, &indent](const Node& /*aNode*/) {
        out << std::string(indent, ' ') << '(' << /* ' ' << aNode.top << ' ' << aNode.bottom << */ std::endl;
        indent += 2;
    };
    auto p_subtree_post = [&out, &indent](const Node& aNode) {
        indent -= 2;
        out << std::string(indent, ' ') << ')';
        if (aNode.edge_length >= 0)
            out << ':' << aNode.edge_length;
        out << std::endl;
    };
    iterate_leaf_pre_post(*this, p_name, p_subtree_pre, p_subtree_post);

} // Tree::print

// ----------------------------------------------------------------------

void Tree::print_edges(std::ostream& out) const
{
    std::map<double, size_t> edges; // edge length to number of occurences
    auto collect_edges = [&edges](const Node& aNode) -> void {
        auto iter_inserted = edges.insert(std::make_pair(aNode.edge_length, 1));
        if (!iter_inserted.second)
            ++iter_inserted.first->second;
    };
    iterate_leaf_pre(*this, collect_edges, collect_edges);
    typedef std::pair<double, size_t> E;
    std::list<E> edges_l(edges.begin(), edges.end());
    edges_l.sort([](const E& a, const E& b) { return a.first < b.first; });
    for (auto e: edges_l) {
        out << e.first << " " << e.second << std::endl;
    }

} // Tree::print_edges

// ----------------------------------------------------------------------

void Tree::fix_labels()
{
    const std::list<std::pair<std::string, size_t>> to_remove {
        {"/HUMAN/", 6},
        {"(H3N2)/", 6},
        {"(H1N1)/", 6},
    };

    auto fix_human = [&to_remove](Node& aNode) -> void {
        for (auto e: to_remove) {
            auto const pos = aNode.name.find(e.first);
            if (pos != std::string::npos)
                aNode.name.erase(pos, e.second);
              // replace __ with a space to handle seq_id
            auto const pos__ = aNode.name.find("__");
            if (pos__ != std::string::npos)
                aNode.name.replace(pos__, 2, " ");
        }
    };
    iterate_leaf(*this, fix_human);

} // Tree::fix_labels

// ----------------------------------------------------------------------

void Tree::prepare_for_drawing()
{
      // set line_no for each name node
    size_t current_line = 0;
    auto set_line_no = [&](Node& aNode) {
        aNode.line_no = current_line;
        ++current_line;
    };
      // set top and bottom for each subtree node
    auto set_top_bottom = [](Node& aNode) {
        aNode.top = aNode.subtree.begin()->middle();
        aNode.bottom = aNode.subtree.rbegin()->middle();
    };
    iterate_leaf_post(*this, set_line_no, set_top_bottom);

} // Tree::prepare_for_drawing

// ----------------------------------------------------------------------

void Tree::match_seqdb(const Seqdb& aSeqdb)
{
    std::map<std::string, size_t> virus_types, lineages;
    auto match_name = [&](Node& aNode) {
        const auto entry_seq = aSeqdb.find_by_seq_id(aNode.name);
        if (entry_seq) {
            aNode.clades = entry_seq.seq().clades();
            aNode.date = entry_seq.entry().date();
            aNode.continent = entry_seq.entry().continent();
            aNode.aa = entry_seq.seq().amino_acids(true);
            ++virus_types[entry_seq.entry().virus_type()];
            ++lineages[entry_seq.entry().lineage()];
        }
    };
    iterate_leaf(*this, match_name);
    auto cmp = [](const auto& a, const auto& b) -> bool { return a.second < b.second; };
    mVirusType = std::max_element(virus_types.begin(), virus_types.end(), cmp)->first;
    mLineage = std::max_element(lineages.begin(), lineages.end(), cmp)->first;

} // Tree::match_seqdb

// ----------------------------------------------------------------------

void Tree::clade_setup()
{
    Clades clades;
    clades.prepare(*this, mSettings.clades);
    mSettings.clades.extract(clades);

} // Tree::clade_setup

// ----------------------------------------------------------------------

std::vector<std::string> Tree::names() const
{
    std::vector<std::string> names;
    auto get_name = [&](const Node& aNode) {
        names.push_back(aNode.name);
    };
    iterate_leaf(*this, get_name);
    return names;

} // Tree::names

// ----------------------------------------------------------------------

std::vector<std::string> Tree::names_between(std::string first, std::string last) const
{
    std::vector<std::string> names;
    bool collect = false;
    auto get_name = [&](const Node& aNode) {
        if (!collect && aNode.name == first)
            collect = true;
        if (collect) {
            if (aNode.name == last)
                collect = false;
            else
                names.push_back(aNode.name);
        }
    };
    iterate_leaf(*this, get_name);
    return names;

} // Tree::names_between

// ----------------------------------------------------------------------

void Tree::make_aa_transitions(const std::vector<size_t>& aPositions)
{
    std::vector<const Node*> leaf_nodes = leaf_nodes_sorted_by_cumulative_edge_length();

      // ?reset aa_transition for all nodes?

    auto make_aa_at = [&](Node& aNode) {
        aNode.aa.resize(aPositions.back() + 1, AA_Transition::Empty);
        for (size_t pos: aPositions) {
            aNode.aa[pos] = 'X';
            for (const auto& child: aNode.subtree) {
                if (child.aa[pos] != 'X') {
                    if (aNode.aa[pos] == 'X')
                        aNode.aa[pos] = child.aa[pos];
                    else if (aNode.aa[pos] != child.aa[pos])
                        aNode.aa[pos] = AA_Transition::Empty;
                    if (aNode.aa[pos] == AA_Transition::Empty)
                        break;
                }
            }
              // If this node has AA_Transition::Empty and a child node has letter, then set aa_transition for the child (unless child is a leaf)
            if (aNode.aa[pos] == AA_Transition::Empty) {
                std::map<char, size_t> aa_count;
                for (auto& child: aNode.subtree) {
                      // if (!child.is_leaf())
                    if (child.aa.size() > pos) { // exclude leaf nodes without aa (i.e. not matched agains seqdb, perhaps due to buggy matching)
                        if (child.aa[pos] != AA_Transition::Empty && child.aa[pos] != 'X') {
                            child.aa_transitions.add(pos, child.aa[pos]);
                            ++aa_count[child.aa[pos]];
                              // std::cout << "aa_transition " << child.aa_transitions << " " << child.name << std::endl;
                        }
                        else {
                            const auto found = child.aa_transitions.find(pos);
                            if (found)
                                ++aa_count[found->right];
                        }
                    }
                }
                if (!aa_count.empty()) {
                    const auto max_aa_count = std::max_element(aa_count.begin(), aa_count.end(), [](const auto& e1, const auto& e2) { return e1.second < e2.second; });
                      // std::cout << "aa_count " << aa_count << "   max: " << max_aa_count->first << ':' << max_aa_count->second << std::endl;
                    if (max_aa_count->second > 1) {
                        aNode.remove_aa_transition(pos, max_aa_count->first, false);
                        aNode.aa_transitions.add(pos, max_aa_count->first);
                    }
                }
            }
        }
          // std::cout << "aa  " << aNode.aa << std::endl;
    };
    iterate_post(*this, make_aa_at);

      // add left part to aa transitions (Derek's algorithm)
    auto add_left_part = [&](Node& aNode) {
        if (!aNode.aa_transitions.empty()) {
            const auto node_left_edge = aNode.cumulative_edge_length - aNode.edge_length;
            const auto lb = std::lower_bound(leaf_nodes.begin(), leaf_nodes.end(), node_left_edge, [](const Node* a, double b) { return a->cumulative_edge_length < b; });
            const Node* node_for_left = lb == leaf_nodes.begin() ? nullptr : *(lb - 1);
            for (auto& transition: aNode.aa_transitions) {
                if (node_for_left) {
                    transition.left = node_for_left->aa[transition.pos];
                    transition.for_left = node_for_left;
                      // std::cout << transition << ' ' << node_left_edge << "   " << node_for_left->display_name() << " " << node_for_left->cumulative_edge_length << std::endl;
                }
                // else {
                //     std::cout << transition << ' ' << node_left_edge << "   no left node" << std::endl;
                // }
            }
        }

          // remove transitions having left and right parts the same
        aNode.aa_transitions.erase(std::remove_if(aNode.aa_transitions.begin(), aNode.aa_transitions.end(), [](auto& e) { return e.left_right_same(); }), aNode.aa_transitions.end());

          // add transition labels information to settings
        if (aNode.aa_transitions) {
            settings().draw_tree.aa_transition.add(aNode.branch_id, aNode.aa_transitions.make_labels());
        }
    };
    iterate_leaf_pre(*this, add_left_part, add_left_part);

      // cleanup
    auto erase_aa_at = [&](Node& aNode) {
        aNode.aa.clear();
    };
    iterate_post(*this, erase_aa_at);

} // Tree::make_aa_transitions

// ----------------------------------------------------------------------

void Tree::make_aa_transitions()
{
    const auto num_positions = longest_aa();
    if (num_positions) {
        std::vector<size_t> all_positions(num_positions);
        std::iota(all_positions.begin(), all_positions.end(), 0);
        make_aa_transitions(all_positions);
    }
    else {
        std::cerr << "WARNING: cannot make AA transition labels: no AA sequences present (match with seqdb?)" << std::endl;
    }

} // Tree::make_aa_transitions

// ----------------------------------------------------------------------

std::vector<std::map<char, size_t>> Tree::aa_per_pos() const
{
    std::vector<std::map<char, size_t>> aa_at_pos;
    auto update = [&](const Node& aNode) {
        if (aa_at_pos.size() < aNode.aa.size()) {
            aa_at_pos.resize(aNode.aa.size());
              // std::cout << aNode.aa.size() << " " << aNode.name << std::endl;
        }
        for (size_t pos = 0; pos < aNode.aa.size(); ++pos) {
            if (aNode.aa[pos] != 'X')
                ++aa_at_pos[pos][aNode.aa[pos]];
        }
    };
    iterate_leaf(*this, update);
    return aa_at_pos;

} // Tree::aa_per_pos

// ----------------------------------------------------------------------

void Node::compute_cumulative_edge_length(double initial_edge_length, double& max_cumulative_edge_length) const
{
    cumulative_edge_length = initial_edge_length + edge_length;
    if (!is_leaf()) {
        for (auto& node: subtree) {
            node.compute_cumulative_edge_length(cumulative_edge_length, max_cumulative_edge_length);
        }
    }
    else if (cumulative_edge_length > max_cumulative_edge_length) {
        max_cumulative_edge_length = cumulative_edge_length;
    }

} // Node::compute_cumulative_edge_length

// ----------------------------------------------------------------------

std::vector<const Node*> Tree::leaf_nodes_sorted_by(const std::function<bool(const Node*,const Node*)>& cmp) const
{
    std::vector<const Node*> leaf_nodes;
    iterate_leaf(*this, [&](const Node& aNode) -> void { leaf_nodes.push_back(&aNode); });
    std::sort(leaf_nodes.begin(), leaf_nodes.end(), cmp);
    return leaf_nodes;

} // Tree::leaf_nodes_sorted_by

// ----------------------------------------------------------------------

void Node::remove_aa_transition(size_t aPos, char aRight, bool aDescentUponRemoval) // recursively
{
    auto remove_aa_transition = [=](Node& aNode) -> bool {
        const bool present_any = aNode.aa_transitions.find(aPos);
        aNode.aa_transitions.remove(aPos, aRight);
        return !present_any;
    };
    if (aDescentUponRemoval)
        iterate_leaf_pre(*this, remove_aa_transition, remove_aa_transition);
    else
        iterate_leaf_pre_stop(*this, remove_aa_transition, remove_aa_transition);

} // Node::remove_aa_transition

// ----------------------------------------------------------------------

size_t Tree::longest_aa() const
{
    size_t longest_aa = 0;
    auto find_longest_aa = [&](const Node& aNode) {
        longest_aa = std::max(longest_aa, aNode.aa.size());
    };
    iterate_leaf(*this, find_longest_aa);
    return longest_aa;

} // Tree::longest_aa

// ----------------------------------------------------------------------

std::vector<const Node*> Tree::find_name(std::string aName) const
{
    std::vector<const Node*> path;
    bool found = find_name_r(aName, path);
    if (!found)
        throw std::runtime_error(aName + " not found in the tree");
    return path;

} // Tree::find_name

// ----------------------------------------------------------------------

bool Node::find_name_r(std::string aName, std::vector<const Node*>& aPath) const
{
    bool found = false;
    if (is_leaf()) {
        if (name == aName) {
            aPath.push_back(this);
            found = true;
        }
    }
    else {
        aPath.push_back(this);
        for (const Node& child: subtree) {
            found = child.find_name_r(aName, aPath);
            if (found)
                break;
        }
        if (!found)
            aPath.pop_back();
    }
    return found;

} // Node::find_name_r

// ----------------------------------------------------------------------

void Tree::re_root(std::string aName)
{
    std::vector<const Node*> path = find_name(aName);
    path.pop_back();
    re_root(path);

} // Tree::re_root

// ----------------------------------------------------------------------

void Tree::re_root(const std::vector<const Node*>& aNewRoot)
{
    if (aNewRoot.front() != this)
        throw std::invalid_argument("Invalid path passed to Tree::re_root");

    std::vector<Node> nodes;
    for (size_t item_no = 0; item_no < (aNewRoot.size() - 1); ++item_no) {
        const Node& source = *aNewRoot[item_no];
        nodes.push_back(Node());
        std::copy_if(source.subtree.begin(), source.subtree.end(), std::back_inserter(nodes.back().subtree), [&](const Node& to_copy) -> bool { return &to_copy != aNewRoot[item_no + 1]; });
        nodes.back().edge_length = aNewRoot[item_no + 1]->edge_length;
    }

    std::vector<Node> new_subtree(aNewRoot.back()->subtree.begin(), aNewRoot.back()->subtree.end());
    Subtree* append_to = &new_subtree;
    for (auto child = nodes.rbegin(); child != nodes.rend(); ++child) {
        append_to->push_back(*child);
        append_to = &append_to->back().subtree;
    }
    subtree = new_subtree;
    edge_length = 0;

} // Tree::re_root

// ----------------------------------------------------------------------

std::pair<Node*,Node*> Tree::find_path_to_next_leaf(std::vector<std::pair<size_t, Node*>>& aPath)
{
    auto& back = aPath.back();
    if (++back.first < back.second->subtree.size()) {
        return std::make_pair(back.second->subtree[back.first].find_path_to_first_leaf(aPath), back.second);
    }
    else if (aPath.size() > 1) {
        aPath.pop_back();
        return find_path_to_next_leaf(aPath);
    }
    else {
        return std::make_pair(nullptr, nullptr);         //  end of tree
    }

} // Tree::find_path_to_next_leaf

// ----------------------------------------------------------------------

void Tree::make_hz_line_sections(double tolerance)
{
    compute_cumulative_edge_length();
    std::cout << "max_cumulative_edge_length " << mMaxCumulativeEdgeLength << std::endl;

      // compute edge_length_to_next
    for (auto node_path = find_path_to_first_leaf(); node_path.first != nullptr; ) {
        auto next_root = find_path_to_next_leaf(node_path.second);
        if (next_root.first != nullptr) {
            node_path.first->edge_length_to_next = node_path.first->cumulative_edge_length - next_root.second->cumulative_edge_length + next_root.first->cumulative_edge_length - next_root.second->cumulative_edge_length;
              // std::cout << node_path.first->name << " " << node_path.first->edge_length_to_next << std::endl;
        }
        node_path.first = next_root.first;
    }

    const auto by_edge_length_to_next = leaf_nodes_sorted_by_edge_length_to_next(); // longest first!
    for (const auto& node: by_edge_length_to_next) {
        std::cout << node->name << " " << node->line_no << " " << node->edge_length_to_next << " " << (node->edge_length_to_next / mMaxCumulativeEdgeLength) << std::endl;
    }

    init_hz_line_sections(true);
    auto& hz_line_sections = settings().draw_tree.hz_line_sections;
    for (const auto& node: by_edge_length_to_next) {
        if ((node->edge_length_to_next / mMaxCumulativeEdgeLength) < tolerance)
            break;
        hz_line_sections.emplace_back(*node, 0xFF0000);
    }
    hz_line_sections.sort();

} // Tree::make_hz_line_sections

// ----------------------------------------------------------------------
// json
// ----------------------------------------------------------------------

constexpr const char* TREE_JSON_DUMP_VERSION = "phylogenetic-tree-v1";

std::string Tree::json(size_t indent) const
{
    std::string target;
    size_t prefix = 0;
    jsonw::json_begin(target, jsonw::NoCommaNoIndent, '{', indent, prefix);
    auto comma = jsonw::json(target, jsonw::NoComma, "  version", TREE_JSON_DUMP_VERSION, indent, prefix);
    comma = jsonw::json(target, comma, "settings", mSettings, indent, prefix);
    jsonw::json(target, comma, "tree", *this, indent, prefix);
    jsonw::json_end(target, '}', indent, prefix);
      // std::cerr << target << std::endl;
    return target;

} // Tree::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma Node::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json_if(target, comma, "aa", aa, indent, prefix);
    comma = jsonw::json_if(target, comma, "clades", clades, indent, prefix);
    comma = jsonw::json_if(target, comma, "continent", continent, indent, prefix);
    comma = jsonw::json_if(target, comma, "date", date, indent, prefix);
    comma = jsonw::json(target, comma, "edge_length", edge_length, indent, prefix);
    comma = jsonw::json_if(target, comma, "id", branch_id, indent, prefix);
    comma = jsonw::json_if(target, comma, "name", name, indent, prefix);
    comma = jsonw::json(target, comma, "number_strains", number_strains, indent, prefix);
    if (aa_transitions) {
        comma = jsonw::json(target, comma, "?", "aa_transitions is for information only, ignored on reading and re-calculated", indent, prefix);
        jsonw::json(target, comma, "aa_transitions", indent, prefix);
        target.append(": ");
        comma = aa_transitions.json(target, jsonw::NoCommaNoIndent, indent, prefix);
    }
    comma = jsonw::json_if(target, comma, "subtree", subtree, indent, prefix);
    return jsonw::json_end(target, '}', indent, prefix);

} // Node::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma AA_Transitions::json(std::string& target, jsonw::IfPrependComma comma, size_t /*indent*/, size_t prefix) const
{
    comma = json_begin(target, comma, '[', 0, prefix);
    for (const auto& aat: *this) {
        if (!aat.empty_left() && !aat.left_right_same()) {
            comma = json_begin(target, comma, '{', 0, prefix);
            comma = jsonw::json(target, comma, "t", aat.display_name(), 0, prefix);
            if (aat.for_left)
                comma = jsonw::json(target, comma, "n", aat.for_left->branch_id, 0, prefix);
            comma = jsonw::json_end(target, '}', 0, prefix);
        }
    }
    return jsonw::json_end(target, ']', 0, prefix);

} // AA_Transitions::json

// ----------------------------------------------------------------------

Tree Tree::from_json(std::string data)
{
    Tree tree;
    auto parse_tree = jsonr::object(jsonr::version(TREE_JSON_DUMP_VERSION) | (jsonr::skey("tree") > tree.json_parser()) | tree.settings().json_parser());
    try {
        parse_tree(std::begin(data), std::end(data));
    }
    catch (jsonr::failure& err) {
        throw jsonr::JsonParsingError(err.message(std::begin(data)));
    }
    catch (axe::failure<char>& err) {
        throw jsonr::JsonParsingError(err.message());
    }
    return tree;

} // Tree::from_json

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
