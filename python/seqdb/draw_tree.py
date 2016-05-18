# -*- Python -*-
# license
# license.

# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import seqdb

# ----------------------------------------------------------------------

def draw_tree(tree_file, path_to_seqdb, output_file, title, pdf_width=600, pdf_height=850):
    tree = seqdb.import_tree(str(tree_file))
    if path_to_seqdb:
        seq_db = seqdb.Seqdb()
        seq_db.load(filename=str(path_to_seqdb))
        tree.match_seqdb(seq_db)
        tree.ladderize()        # must be before clade_setup()
        tree.clade_setup()
    else:
        tree.ladderize()
    tree.make_aa_transitions()

    # tree.settings().draw_tree.grid_step = args.grid_step
    # tree.settings().draw_tree.aa_transition.show_node_for_left_line = args.aa_transition_left_line
    # tree.settings().draw_tree.aa_transition.show_empty_left = args.aa_transition_empty_left
    # tree.settings().draw_tree.aa_transition.number_strains_threshold = args.aa_transition_number_strains_threshold

    module_logger.info('Drawing tree into {}'.format(output_file))
    surface = seqdb.Surface(str(output_file), pdf_width, pdf_height)
    signature_page = seqdb.SignaturePage()
    (signature_page
     .select_parts(seqdb.Show.Title | seqdb.Show.Tree | seqdb.Show.Legend | seqdb.Show.TimeSeries | seqdb.Show.Clades)
     .title(seqdb.Text(seqdb.Location(0, 10), title.format(virus_type=format_virus_type(tree)), 0, 20))
     .color_by_continent(True)
     .prepare(tree, surface)
     .draw(tree, surface))

# ----------------------------------------------------------------------

def format_virus_type(tree):
    virus_type = tree.virus_type()
    lineage = tree.lineage()
    if virus_type:
        title = virus_type
        if lineage:
            if lineage in ["VICTORIA", "YAMAGATA"]:
                lineage = lineage[:3].capitalize()
            title += "/" + lineage
    else:
        title = ""
    return title

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
