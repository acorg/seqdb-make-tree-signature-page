# -*- Jq -*-
# https://stedolan.github.io/jq/manual/
# Lib to be imported by jq scripts for postprocessing tree json
# ----------------------------------------------------------------------

# Import using
# import "tree" as TREE;

def disable_clade(name):
  .settings.clades.per_clade |= map(if .id == name then .show = false else . end);

def set_slot_for_all_clades(slot):
  .settings.clades.per_clade[].slot = slot;

# Usage: add_hz_line_section({"first_name": "A(H3N2)/NEVADA/24/2015__SIAT3", "color": "#4040FF"})
def add_hz_line_section(section):
  .settings.tree.hz_line_sections.hz_line_sections |= . + [section];

def hz_line_section_0_color(color):
  .settings.tree.hz_line_sections.hz_line_sections[0].color = color;
