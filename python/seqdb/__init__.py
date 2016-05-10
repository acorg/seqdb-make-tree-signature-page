from seqdb_backend import *
from .timeit import timeit
from .fasta import export_from_seqdb
from .make_tree import run_raxml_garli

# ----------------------------------------------------------------------

def open(path_to_seqdb):
    seqdb = Seqdb()
    seqdb.load(filename=str(path_to_seqdb))
    return seqdb

# ----------------------------------------------------------------------
