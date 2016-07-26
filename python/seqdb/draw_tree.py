# -*- Python -*-
# license
# license.

# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import seqdb as seqdb_m

# ----------------------------------------------------------------------

def draw_tree(tree_file, seqdb, output_file, title, pdf_width=600, pdf_height=850):
    tree = seqdb_m.import_tree(str(tree_file))
    if seqdb:
        if isinstance(seqdb, (str, Path)):
            seq_db = seqdb_m.Seqdb()
            seq_db.load(filename=str(seqdb))
            seqdb = seq_db
        tree.match_seqdb(seqdb)
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
    surface = seqdb_m.Surface(str(output_file), pdf_width, pdf_height)
    signature_page = seqdb_m.SignaturePage()
    (signature_page
     .select_parts(seqdb_m.Show.Title | seqdb_m.Show.Tree | seqdb_m.Show.Legend | seqdb_m.Show.TimeSeries | seqdb_m.Show.Clades)
     .title(seqdb_m.Text(seqdb_m.Location(0, 20), title.format(virus_type=format_virus_type(tree)), 0, 20))
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
