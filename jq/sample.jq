# -*- Jq -*-
# https://stedolan.github.io/jq/manual/
#----------------------------------------------------------------------

# Not a module! just sample script

import "tree" as TREE;

.
# | TREE::disable_clade("gly")
# | TREE::disable_clade("no-gly")
# | TREE::disable_clade("3C3b")
# | TREE::set_slot_for_all_clades(0)
  | TREE::hz_line_section_0_color("#FFFF00")
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/NEVADA/24/2015__SIAT3", "color": "#4040FF"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/BANGLADESH/3400/2015__MDCK1/SIAT2", "color": "#808080"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/SENDAI-H/N279/2015 MDCK3/SIAT1", "color": "#FF4040"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/MICHIGAN/38/2015 MK1/SIAT1 (2015-08-09)", "color": "#A500FF"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/WEST VIRGINIA/23/2015 SIAT2 (2015-11-23)", "color": "#A5FF00"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/ALASKA/146/2015__OR", "color": "#FFA500"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/WISCONSIN/16/2015__SIAT1", "color": "#00A5FF"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/DELAWARE/35/2015__SIAT3", "color": "#40FF80"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/INDIANA/14/2015 SIAT3 (2015-05-04)", "color": "#40FF80"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/VICTORIA/505/2015 SIAT2", "color": "#40FF80"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/HAWAII/27/2015 SIAT3", "color": "#40FF80"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/CAMBODIA/Z0709312/2015__SIAT2", "color": "#40FF80"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/MIE/26/2015 MDCK1/SIAT2", "color": "#40FF80"})
# | TREE::add_hz_line_section({"first_name": "A(H3N2)/BOLIVIA/426/2015__OR", "color": "#40FF80"})
  | TREE::add_hz_line_section({"first_name": "A(H3N2)/TAIWAN/1085/2015 MDCK3/SIAT1", "color": "#40FF80"})
