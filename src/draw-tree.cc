#include "draw-tree.hh"

#include "tree.hh"
#include "antigenic-maps.hh"

// ----------------------------------------------------------------------

DrawTree& DrawTree::prepare(Tree& aTree, const SettingsDrawTree& aSettings)
{
    add_hz_line_sections_gap(aTree, aSettings.hz_line_sections);
    aTree.prepare_for_drawing();
    mNumberOfLines = aTree.height();

    mNodesToMark.clear();
    for (const auto& mark_node: aTree.settings().draw_tree.mark_nodes) {
        mNodesToMark.emplace(mark_node.id, mark_node);
    }

    return *this;

} // DrawTree::prepare

// ----------------------------------------------------------------------

void DrawTree::add_hz_line_sections_gap(Tree& aTree, const HzLineSections& aSections)
{
    if (aSections.vertical_gap > 0) {
        for (size_t section_no = 1; section_no < aSections.size(); ++section_no) {
            auto& section = aSections[section_no];
            const auto node = aTree.find_node_by_name(section.first_name);
            if (node == nullptr)
                throw std::runtime_error("Cannot process hz-line-section: \"" + section.first_name + "\" not found in the tree");
            node->vertical_gap_before = aSections.vertical_gap;
        }
    }

} // DrawTree::add_hz_line_sections_gap

// ----------------------------------------------------------------------

void DrawTree::calculate_viewports(const Tree& aTree, const Viewport& aViewport)
{
    mViewport = aViewport;
    mHorizontalStep = aViewport.size.width / aTree.width() * 0.9;
    mVerticalStep = aViewport.size.height / (aTree.height() + 2); // +2 to add space at the top and bottom

} // DrawTree::calculate_viewports

// ----------------------------------------------------------------------

void DrawTree::draw(const Tree& aTree, Surface& aSurface, const Viewport& aViewport, const SettingsDrawTree& aSettings)
{
    mLineWidth = std::min(aSettings.line_width, mVerticalStep * 0.5);
    std::cout << "Tree line width: " << mLineWidth << "  Settings: " << aSettings.line_width << "  vertical_step/2: " << mVerticalStep * 0.5 << std::endl;
    set_label_scale(aSurface, aTree, aViewport, aSettings);
    set_horizontal_step(aSurface, aTree, aViewport, aSettings);

    draw_grid(aSurface, aViewport, aSettings);
    draw_node(aTree, aSurface, aViewport.origin, aSettings, aSettings.root_edge);

    mark_nodes(aSurface);
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
        const auto text_origin = viewport.top_right() + Size(aSettings.name_offset, tsize.height / 2);
        aSurface.text(text_origin, text, mColoring->color(aNode), font_size, aSettings.label_style);
        auto mark_node = mNodesToMark.find(aNode.name);
        if (mark_node != mNodesToMark.end())
            mark_node->second.set(text_origin, aNode);
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

void DrawTree::mark_nodes(Surface& aSurface)
{
    for (const auto& node_entry: mNodesToMark) {
        const auto& node = node_entry.second;
        const auto text_origin = node.location + Location(node.mark_data.label_offset_x, node.mark_data.label_offset_y);
        const auto label = node.mark_data.label.empty() ? node.mark_data.id : node.mark_data.label;
        aSurface.text(text_origin, label, node.mark_data.label_color, node.mark_data.label_size, node.mark_data.label_style);

        const auto tsize = aSurface.text_size(label, node.mark_data.label_size, node.mark_data.label_style);
        const auto line_origin = text_origin + Size(tsize.width / 2, node.mark_data.label_offset_y > 0 ? -tsize.height : 0);
        aSurface.line(line_origin, node.location, node.mark_data.line_color, node.mark_data.line_width);
    }

} // DrawTree::mark_nodes

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
        const auto node = aTree.find_node_by_name(section.first_name);
        if (node == nullptr)
            throw std::runtime_error("Cannot process hz-line-section: \"" + section.first_name + "\" not found in the tree");
        section.first_line = node->line_no;
        if (section_no == 0 && section.first_line != 0)
            throw std::runtime_error("Cannot process hz-line-section: line_no for the first section is not 0");
    }

    std::sort(aSections.begin(), aSections.end(), [](const auto& a, const auto& b) -> bool { return a.first_line < b.first_line; });

    return *this;

} // DrawHzLines::prepare

// ----------------------------------------------------------------------

void DrawHzLines::draw(Surface& aSurface, const Viewport& aTimeSeriesViewport, const Viewport& aAntigenicMapsViewport, const DrawTree& aDrawTree, const AntigenicMaps* aAntigenicMaps, const SettingsAntigenicMaps& aAntigenicMapsSettings, const HzLineSections& aSections, SettingsSignaturePage::Layout aLayout) const
{
    if (!aSections.empty()) {
        auto draw_sequenced = &DrawHzLines::draw_sequenced_right;
        auto draw_section_lines = &DrawHzLines::draw_section_lines_right;
        switch (aLayout) {
          case SettingsSignaturePage::TreeTimeseriesCladesMaps:
              draw_sequenced = &DrawHzLines::draw_sequenced_right;
              draw_section_lines = &DrawHzLines::draw_section_lines_right;
              break;
          case SettingsSignaturePage::TreeCladesTimeseriesMaps:
              draw_sequenced = &DrawHzLines::draw_sequenced_left;
              draw_section_lines = &DrawHzLines::draw_section_lines_left;
              break;
        }

        // if (aAntigenicMapsSettings.mode == SettingsAntigenicMaps::BWVpos) {
        //       // make grey background for the antigenic map area
        //     aSurface.rectangle_filled(Location(aTimeSeriesViewport.right() + aAntigenicMapsSettings.border_width, 0), aSurface.canvas_size(), TRANSPARENT, 0, 0xF0F0F0);
        // }

        const double vertical_step = aDrawTree.vertical_step();
        for (size_t section_no = 0; section_no < aSections.size(); ++section_no) {
            const auto& section = aSections[section_no];
            double first_y = aTimeSeriesViewport.origin.y;
            if (section_no != 0) {
                first_y += vertical_step * (section.first_line - 0.5);
                  // draw hz line in the time series area
                const double line_y = aTimeSeriesViewport.origin.y + vertical_step * (section.first_line - (1 + aSections.vertical_gap) * 0.5);
                aSurface.line({aTimeSeriesViewport.origin.x, line_y}, {aTimeSeriesViewport.right(), line_y}, aSections.hz_line_color, aSections.hz_line_width);
            }

            if (aAntigenicMaps != nullptr) {
                const double last_y = section_no == (aSections.size() - 1)
                        ? aTimeSeriesViewport.bottom()
                        : aTimeSeriesViewport.origin.y + vertical_step * (aSections[section_no+1].first_line - aSections.vertical_gap - 0.5);
                switch (aAntigenicMapsSettings.mode) {
                  case SettingsAntigenicMaps::ColoredGrid:
                        // draw section vertical colored bar
                      aSurface.line({aAntigenicMapsViewport.origin.x, first_y}, {aAntigenicMapsViewport.origin.x, last_y}, section.color, section.line_width);
                      break;
                  case SettingsAntigenicMaps::BWVpos: {
                        // draw section direction lines
                      const Viewport map_viewport = aAntigenicMaps->viewport_of(aAntigenicMapsViewport, section_no);
                      if (map_viewport.size.width > 0) { // map viewport with zero size means that map has no tracked antigens and must not be shown according to settings (maps_for_sections_without_antigens)
                          (this->*draw_section_lines)(aSurface, aTimeSeriesViewport, aAntigenicMapsViewport, map_viewport, aAntigenicMapsSettings, first_y, last_y, vertical_step, aSections);
                      }
                  }
                      break;
                  case SettingsAntigenicMaps::NamedGrid:
                        // draw brackets
                      const Viewport map_viewport = aAntigenicMaps->viewport_of(aAntigenicMapsViewport, section_no);
                      if (map_viewport.size.width > 0) { // map viewport with zero size means that map has no tracked antigens and must not be shown according to settings (maps_for_sections_without_antigens)
                          draw_section_brackets(aSurface, aTimeSeriesViewport, aAntigenicMapsSettings, first_y, last_y, vertical_step, aSections);
                      }
                      break;
                }
            }
        }

        if (aAntigenicMaps != nullptr && aSections.sequenced_antigen_line_show) {
              // draw sequenced antigen marks
            (this->*draw_sequenced)(aSurface, aTimeSeriesViewport, *aAntigenicMaps, aAntigenicMapsViewport, aSections, vertical_step);
        }
    }

} // DrawHzLines::draw

// ----------------------------------------------------------------------

void DrawHzLines::draw_section_lines_right(Surface& aSurface, const Viewport& aTimeSeriesViewport, const Viewport& aAntigenicMapsViewport, const Viewport& aMapViewport, const SettingsAntigenicMaps& /*aAntigenicMapsSettings*/, double first_y, double last_y, double /*vertical_step*/, const HzLineSections& aSections) const
{
    const double gap = aAntigenicMapsViewport.size.height * 0.01;
    const double top_y = aMapViewport.center().y - gap;
    const double bottom_y = top_y + gap * 2.0;

    aSurface.line({aTimeSeriesViewport.right(), first_y}, {aAntigenicMapsViewport.origin.x, top_y}, aSections.hz_line_color, aSections.hz_line_width);
    aSurface.line({aTimeSeriesViewport.right(), last_y}, {aAntigenicMapsViewport.origin.x, bottom_y}, aSections.hz_line_color, aSections.hz_line_width);

} // DrawHzLines::draw_section_lines_right

// ----------------------------------------------------------------------

void DrawHzLines::draw_section_lines_left(Surface& aSurface, const Viewport& aTimeSeriesViewport, const Viewport& aAntigenicMapsViewport, const Viewport& aMapViewport, const SettingsAntigenicMaps& aAntigenicMapsSettings, double first_y, double last_y, double vertical_step, const HzLineSections& aSections) const
{
    const double gap = aAntigenicMapsViewport.size.height * 0.01;
    const double map_top_y = aMapViewport.center().y - gap;
    const double map_bottom_y = map_top_y + gap * 2;

    const double brace_vertical_x = aTimeSeriesViewport.right() + aSections.sequenced_antigen_line_length * 3;
    first_y -= vertical_step * 10;
    last_y += vertical_step * 10;
    const double brace_gap_top_y = (first_y + last_y) /2 - gap;
    const double brace_gap_bottom_y = brace_gap_top_y + gap * 2;

    std::vector<Location> vertices;
    vertices.emplace_back(aMapViewport.origin.x, map_top_y);
    vertices.emplace_back(brace_vertical_x, brace_gap_top_y);
    vertices.emplace_back(brace_vertical_x, first_y);
    vertices.emplace_back(aTimeSeriesViewport.right() + aAntigenicMapsSettings.border_width, first_y);
    vertices.emplace_back(aTimeSeriesViewport.right() + aAntigenicMapsSettings.border_width, last_y);
    vertices.emplace_back(brace_vertical_x, last_y);
    vertices.emplace_back(brace_vertical_x, brace_gap_bottom_y);
    vertices.emplace_back(aMapViewport.origin.x, map_bottom_y);

    aSurface.path_outline(vertices.begin(), vertices.begin() + 4, aSections.connecting_pipe_border_color, aSections.connecting_pipe_border_width);
    aSurface.path_outline(vertices.begin() + 4, vertices.end(), aSections.connecting_pipe_border_color, aSections.connecting_pipe_border_width);
    aSurface.path_fill(vertices.begin(), vertices.end(), aSections.connecting_pipe_background_color);

} // DrawHzLines::draw_section_lines_left

// ----------------------------------------------------------------------

void DrawHzLines::draw_section_brackets(Surface& aSurface, const Viewport& aTimeSeriesViewport, const SettingsAntigenicMaps& aAntigenicMapsSettings, double first_y, double last_y, double vertical_step, const HzLineSections& aSections) const
{
    const double brace_vertical_x = aTimeSeriesViewport.right() + aSections.sequenced_antigen_line_length * 3;
    first_y -= vertical_step * 10;
    last_y += vertical_step * 10;

    std::vector<Location> vertices;
    vertices.emplace_back(aTimeSeriesViewport.right() + aAntigenicMapsSettings.border_width, first_y);
    vertices.emplace_back(brace_vertical_x, first_y);
    vertices.emplace_back(brace_vertical_x, last_y);
    vertices.emplace_back(aTimeSeriesViewport.right() + aAntigenicMapsSettings.border_width, last_y);

    aSurface.path_outline(vertices.begin(), vertices.end(), aAntigenicMapsSettings.bracket_border_color, aAntigenicMapsSettings.bracket_border_width);
    aSurface.path_fill(vertices.begin(), vertices.end(), aAntigenicMapsSettings.bracket_background_color);

} // DrawHzLines::draw_section_brackets

// ----------------------------------------------------------------------

void DrawHzLines::draw_sequenced_right(Surface& aSurface, const Viewport& aTimeSeriesViewport, const AntigenicMaps& aAntigenicMaps, const Viewport& aAntigenicMapsViewport, const HzLineSections& aSections, double vertical_step) const
{
    const double mark_x1 = aAntigenicMapsViewport.origin.x - aSections[0].line_width;
    const double mark_x2 = mark_x1 - aSections.sequenced_antigen_line_length;
    for (auto line_no: aAntigenicMaps.lines_of_sequenced_antigens_in_chart()) {
        const double y = aTimeSeriesViewport.origin.y + vertical_step * line_no;
        aSurface.line({mark_x1, y}, {mark_x2, y}, aSections.sequenced_antigen_line_color, aSections.sequenced_antigen_line_width);
    }

} // DrawHzLines::draw_right

// ----------------------------------------------------------------------

void DrawHzLines::draw_sequenced_left(Surface& aSurface, const Viewport& aTimeSeriesViewport, const AntigenicMaps& aAntigenicMaps, const Viewport& /*aAntigenicMapsViewport*/, const HzLineSections& aSections, double vertical_step) const
{
    const double mark_x1 = aTimeSeriesViewport.right() + aSections.sequenced_antigen_line_length;
    const double mark_x2 = mark_x1 + aSections.sequenced_antigen_line_length;
    for (auto line_no: aAntigenicMaps.lines_of_sequenced_antigens_in_chart()) {
        const double y = aTimeSeriesViewport.origin.y + vertical_step * line_no;
        aSurface.line({mark_x1, y}, {mark_x2, y}, aSections.sequenced_antigen_line_color, aSections.sequenced_antigen_line_width);
    }

} // DrawHzLines::draw_left

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
