#include "draw-tree.hh"

#include "tree.hh"
#include "antigenic-maps.hh"

// ----------------------------------------------------------------------

DrawTree& DrawTree::prepare(Tree& aTree)
{
    aTree.prepare_for_drawing();
    const auto tre_wh = aTree.width_height();
    mNumberOfLines = tre_wh.second;

    mVaccines.clear();
    for (const auto& vaccine_data: aTree.settings().draw_tree.vaccines) {
        mVaccines.emplace(vaccine_data.id, vaccine_data);
    }

    return *this;

} // DrawTree::prepare

// ----------------------------------------------------------------------

void DrawTree::draw(const Tree& aTree, Surface& aSurface, const Viewport& aViewport, const SettingsDrawTree& aSettings)
{
    mViewport = aViewport;
    const auto tre_wh = aTree.width_height();
    mHorizontalStep = aViewport.size.width / tre_wh.first * 0.9;
    mVerticalStep = aViewport.size.height / (tre_wh.second + 2); // +2 to add space at the top and bottom
    mLineWidth = std::min(aSettings.line_width, mVerticalStep * 0.5);
    set_label_scale(aSurface, aTree, aViewport, aSettings);
    set_horizontal_step(aSurface, aTree, aViewport, aSettings);

    draw_grid(aSurface, aViewport, aSettings);
    draw_node(aTree, aSurface, aViewport.origin, aSettings, aSettings.root_edge);

      // aSurface.line(aViewport.origin, aViewport.bottom_right(), 0xFF00A5, 5, CAIRO_LINE_CAP_ROUND);

} // DrawTree::draw

// ----------------------------------------------------------------------

void DrawTree::draw_node(const Node& aNode, Surface& aSurface, const Location& aOrigin, const SettingsDrawTree& aSettings, double aEdgeLength)
{
    const Viewport viewport(Location(aOrigin.x, aOrigin.y + mVerticalStep * aNode.middle()),
                            Size((aEdgeLength < 0.0 ? aNode.edge_length : aEdgeLength) * mHorizontalStep, 0));

    aSurface.line(viewport.origin, viewport.top_right(), aSettings.line_color, mLineWidth);
    draw_aa_transition(aNode, aSurface, viewport, aSettings.aa_transition);
    if (aNode.is_leaf()) {
        const std::string text = aNode.display_name();
        const auto font_size = mVerticalStep * mLabelScale;
        const auto tsize = aSurface.text_size(text, font_size, aSettings.label_style);
        aSurface.text(viewport.top_right() + Size(aSettings.name_offset, tsize.height * 0.5), text, mColoring->color(aNode), font_size, aSettings.label_style);
    }
    else {
        // if (aShowBranchIds && !aNode.branch_id.empty()) {
        //     show_branch_id(aSurface, aNode.branch_id, aLeft, y);
        // }
        // if (!aNode.name.empty() && aNode.number_strains > aNumberStrainsThreshold) {
        //     show_branch_annotation(aSurface, aNode.branch_id, aNode.name, aLeft, right, y);
        // }
        aSurface.line({viewport.right(), aOrigin.y + mVerticalStep * aNode.top}, {viewport.right(), aOrigin.y + mVerticalStep * aNode.bottom}, aSettings.line_color, mLineWidth);
          // draw_aa_transition(aNode, aSurface, viewport, aSettings.aa_transition);
        for (auto node = aNode.subtree.begin(); node != aNode.subtree.end(); ++node) {
            draw_node(*node, aSurface, Location(viewport.right(), aOrigin.y), aSettings);
        }
    }

} // DrawTree::draw_node

// ----------------------------------------------------------------------

void DrawTree::draw_aa_transition(const Node& aNode, Surface& aSurface, const Viewport& aViewport, const SettingsAATransition& aSettings)
{
    if (!aNode.aa_transitions.empty() && aNode.number_strains >= aSettings.number_strains_threshold) {
        auto labels = aNode.aa_transitions.make_labels(aSettings.show_empty_left);
        auto branch_settings = aSettings.for_branch(aNode.branch_id);
        if (!labels.empty()) {
            const auto longest_label = std::max_element(labels.begin(), labels.end(), [](const auto& a, const auto& b) { return a.first.size() < b.first.size(); });
            const auto longest_label_size = aSurface.text_size(longest_label->first, branch_settings.size, branch_settings.style);
            const Size offset(aViewport.size.width > longest_label_size.width ? (aViewport.size.width - longest_label_size.width) / 2 : (aViewport.size.width - longest_label_size.width),
                              longest_label_size.height * branch_settings.interline);
            Location origin(aViewport.origin + offset);
            for (const auto& label: labels) {
                const auto label_width = aSurface.text_size(label.first, branch_settings.size, branch_settings.style).width;
                const Location label_xy(origin.x + (longest_label_size.width - label_width) / 2, origin.y);
                aSurface.text(label_xy, label.first, branch_settings.color, branch_settings.size, branch_settings.style);
                if (aSettings.show_node_for_left_line && label.second) {
                    aSurface.line(aViewport.origin, // origin - Size(0, longest_label_size.height / 2)
                                  mViewport.origin + Location(mHorizontalStep * label.second->cumulative_edge_length, mVerticalStep * label.second->line_no),
                                  aSettings.node_for_left_line_color, aSettings.node_for_left_line_width);
                }
                origin.y += longest_label_size.height * branch_settings.interline;
            }

            std::cout << "AA transitions: ";
            std::transform(labels.begin(), labels.end(), std::ostream_iterator<std::string>(std::cout, " "), [](const auto& e) -> std::string { return e.first; });
            std::cout << " --> " << aNode.branch_id << std::endl;
        }
    }

} // DrawTree::draw_aa_transition

// ----------------------------------------------------------------------

void DrawTree::draw_grid(Surface& aSurface, const Viewport& aViewport, const SettingsDrawTree& aSettings)
{
    if (aSettings.grid_step) {
        const auto grid_step = aSettings.grid_step * mVerticalStep;
        const auto font_size = grid_step * 0.4;
        const auto style = aSettings.aa_transition.data.style;
        const auto tsize = aSurface.text_size("8888", font_size, style);
        const auto right = aViewport.right() - tsize.width;
        const auto left = aViewport.right() - tsize.width * 2;
        size_t v_id = aSettings.grid_step;
        for (double y = aViewport.origin.y + grid_step; y < aViewport.bottom(); y += grid_step, v_id += aSettings.grid_step) {
            aSurface.line(Location(left, y), Location(right, y), aSettings.grid_color, aSettings.grid_width);
            aSurface.text(Location(right, y), std::to_string(v_id), aSettings.grid_color, font_size, style);
        }
        const auto bottom = aViewport.bottom() + tsize.height * 2;
        const auto top = aViewport.bottom();
        v_id = 0;
        for (double x = aViewport.origin.x; x < right; x += grid_step, ++v_id) {
            aSurface.line(Location(x, top), Location(x, bottom), aSettings.grid_color, aSettings.grid_width);
            std::string label;
            if (v_id < 26) {
                label = std::string(1, 'A' + static_cast<char>(v_id));
            }
            else {
                const auto prefix = 'A' + static_cast<char>(v_id / 26 - 1);
                label = std::string(1, prefix) + std::string(1, 'A' + static_cast<char>(v_id % 26));
            }
            aSurface.text(Location(x, bottom), label, aSettings.grid_color, font_size, style);
        }
    }

} // DrawTree::draw_grid

// ----------------------------------------------------------------------

void DrawTree::set_label_scale(Surface& aSurface, const Tree& aTree, const Viewport& aViewport, const SettingsDrawTree& aSettings)
{
    mLabelScale = 1.0;
    mWidth = tree_width(aSurface, aTree, aSettings);
    for (int i = 0; (mLabelScale * mVerticalStep) > 1.0 && mWidth > aViewport.size.width; ++i) {
        mLabelScale *= 0.95;
        mWidth = tree_width(aSurface, aTree, aSettings, aSettings.root_edge);
          // std::cerr << i << " label scale: " << mLabelScale << "  width:" << mWidth << " right:" << tree_right_margin << std::endl;
    }

} // DrawTree::set_label_scale

// ----------------------------------------------------------------------

void DrawTree::set_horizontal_step(Surface& aSurface, const Tree& aTree, const Viewport& aViewport, const SettingsDrawTree& aSettings)
{
    while (true) {
        const double save_h_step = mHorizontalStep;
        const double save_width = mWidth;
        mHorizontalStep *= 1.05;
        mWidth = tree_width(aSurface, aTree, aSettings, aSettings.root_edge);
        if (mWidth >= aViewport.size.width) {
            mHorizontalStep = save_h_step;
            mWidth = save_width;
            break;
        }
    }

} // DrawTree::set_horizontal_step

// ----------------------------------------------------------------------

double DrawTree::tree_width(Surface& aSurface, const Node& aNode, const SettingsDrawTree& aSettings, double aEdgeLength) const
{
    double r = 0;
    const double right = (aEdgeLength < 0.0 ? aNode.edge_length : aEdgeLength) * mHorizontalStep;
    if (aNode.is_leaf()) {
        auto const font_size = mVerticalStep * mLabelScale;
        r = aSurface.text_size(aNode.display_name(), font_size, aSettings.label_style).width + aSettings.name_offset;
    }
    else {
        for (auto node = aNode.subtree.begin(); node != aNode.subtree.end(); ++node) {
            const double node_r = tree_width(aSurface, *node, aSettings);
            if (node_r > r)
                r = node_r;
        }
    }
    return r + right;

} // DrawTree::tree_width

// ----------------------------------------------------------------------

DrawHzLines& DrawHzLines::prepare(Tree& aTree, HzLineSections& aSections)
{
      // find line_no for each section first and last
    for (size_t section_no = 0; section_no < aSections.size(); ++section_no) {
        auto& section = aSections[section_no];
        const auto first = aTree.find_name(section.first_name);
        if (first.empty())
            throw std::runtime_error("Cannot process hz-line-section: \"" + section.first_name + "\" not found in the tree");
        section.first_line = first.back()->line_no;
        if (section_no == 0 && section.first_line != 0)
            throw std::runtime_error("Cannot process hz-line-section: line_no for the first section is not 0");
    }

    std::sort(aSections.begin(), aSections.end(), [](const auto& a, const auto& b) -> bool { return a.first_line < b.first_line; });

    return *this;

} // DrawHzLines::prepare

// ----------------------------------------------------------------------

void DrawHzLines::draw(Surface& aSurface, const Viewport& aTimeSeries, const Viewport& aAntigenicMapsViewport, const DrawTree& aDrawTree, const AntigenicMaps& aAntigenicMaps, const HzLineSections& aSections)
{
    const auto vertical_step = aDrawTree.vertical_step();
    for (size_t section_no = 0; section_no < aSections.size(); ++section_no) {
        const auto& section = aSections[section_no];
        double first_y;
        if (section_no != 0) {
            first_y = aTimeSeries.origin.y + vertical_step * section.first_line - vertical_step * 0.5;
              // draw hz line in the time series area
            aSurface.line({aTimeSeries.origin.x, first_y}, {aTimeSeries.right(), first_y}, aSections.hz_line_color, aSections.hz_line_width);
        }
        else {
            first_y = aTimeSeries.origin.y;
        }

          // draw section vertical colored bar
        double last_y = section_no == (aSections.size() - 1) ? aTimeSeries.bottom() : aTimeSeries.origin.y + vertical_step * aSections[section_no+1].first_line + vertical_step * 0.5;
        aSurface.line({aAntigenicMapsViewport.origin.x, first_y}, {aAntigenicMapsViewport.origin.x, last_y}, section.color, section.line_width);
    }

    if (aSections.sequenced_antigen_line_show) {
          // draw sequenced antigen marks
        const double mark_x1 = aAntigenicMapsViewport.origin.x - aSections[0].line_width;
        const double mark_x2 = mark_x1 - aSections.sequenced_antigen_line_length;
        for (auto line_no: aAntigenicMaps.lines_of_sequenced_antigens_in_chart()) {
            const double y = aTimeSeries.origin.y + vertical_step * line_no;
            aSurface.line({mark_x1, y}, {mark_x2, y}, aSections.sequenced_antigen_line_color, aSections.sequenced_antigen_line_width);
        }
    }

} // DrawHzLines::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
