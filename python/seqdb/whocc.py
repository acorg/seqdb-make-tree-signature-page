# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import operator
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)
from . import acmacs, normalize, fasta as fasta_m, fasta_old
from seqdb_backend import *
from .hidb import HiDb
from .update import SeqdbUpdater

# ----------------------------------------------------------------------

# seqdb_path: ./seqdb.json.xz
# acmacs_url: https://localhost:1168
# hidb_dir: ~/WHO
# sequence_store_dir: ~/ac/tables-store/sequences

def update(seqdb_path :Path, acmacs_url, hidb_dir :Path, sequence_store_dir :Path, input_files=None, load_existing_seqdb=False, save_seqdb=True):
    acmacs.api(acmacs_url)
    db = Seqdb()
    hidb = HiDb(hidb_dir)
    files = collect_files(db, input_files, sequence_store_dir)
    db_updater = SeqdbUpdater(db, filename=seqdb_path, load=load_existing_seqdb, hidb=hidb)
    read_file_one_by_one_update_db(db_updater, files)
    db_updater.match_hidb()
    db_updater.add_clades()               # note clades must be updated after matching with hidb, because matching provides infor about B lineage
    module_logger.info(db.report())
    if save_seqdb:
        db_updater.save(indent=1)
    return db

# ----------------------------------------------------------------------

def collect_files(db, input_files, source_dir :Path):
    if input_files:
        r = [make_file_entry(db, Path(fn)) for fn in input_files]
    else:
        r = [make_file_entry(db, fn) for fn in source_dir.glob("*.fas.bz2") if "aminoacid.fas.bz2" not in str(fn)]
    r.sort(key=operator.itemgetter("date"))
    module_logger.debug('{} fasta files found for {} {}'.format(len(r), sorted(set(e["lab"] for e in r)), sorted(set(e["virus_type"] for e in r))))
    return r

# ----------------------------------------------------------------------

def read_file_one_by_one_update_db(db_updater, files):
    for f_no, file_entry in enumerate(files, start=1):
        module_logger.info('{} {}'.format(f_no, file_entry["f"]))
        data = read_file(file_entry)
        module_logger.info('{} entries to update seqdb with'.format(len(data)))
        # pprint.pprint(data)
        db_updater.add(data)

# ----------------------------------------------------------------------

def make_file_entry(db, fn :Path):
    try:
        lab, subtype, date, *rest = fn.stem.split("-")
    except ValueError:
        # lab, subtype, date = "UNKNOWN", "B", "19000101"
        raise
    return {"lab": lab.upper(), "virus_type": normalize.virus_type(subtype), "date": date, "f": fn}

# ----------------------------------------------------------------------

def read_file(file_entry):
    csv_filename = Path(str(file_entry["f"]).replace(".fas", ".csv"))
    if csv_filename.exists():
        data = fasta_old.read_fasta_with_csv(fasta_file=file_entry["f"], csv_file=csv_filename, **file_entry)
    else:
        data = fasta_m.read_fasta_with_name_parsing(fasta_file=file_entry["f"], **file_entry)
    return data

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
