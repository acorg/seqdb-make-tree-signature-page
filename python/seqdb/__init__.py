from seqdb_backend import *
from .timeit import timeit
from .fasta import export_from_seqdb
from .make_tree import RaxmlBestGarli, RaxmlSurvivedBestGarli, FasttreeRaxmlSurvivedBestGarli, RaxmlAllGarli, run_raxml_best_garli, run_raxml_survived_best_garli, run_raxml_all_garli, run_fasttree_raxml_survived_best_garli, run_in_background
from .update import SeqdbUpdater

# ----------------------------------------------------------------------

def open(path_to_seqdb):
    seqdb = Seqdb()
    seqdb.load(filename=str(path_to_seqdb))
    return seqdb

# ----------------------------------------------------------------------
