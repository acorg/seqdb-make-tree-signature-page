# -*- Python -*-
# license
# license.

"""
Functions for reading and generating fasta files.
"""

import os, re, collections, operator
import logging; module_logger = logging.getLogger(__name__)
from . import open_file, normalize

# ======================================================================

class FastaReaderError (Exception):
    pass

# ======================================================================

def export_from_seqdb(seqdb, filename, output_format, amino_acids, lab, virus_type, lineage, gene, start_date, end_date, base_seq, name_format, aligned, truncate_left, encode_name, wrap, truncate_to_most_common_length, hamming_distance_threshold, hamming_distance_report, sort_by, with_hi_name, name_match):

    def make_entry(e):
        r = {
            "e": e,
            "n": name_format.format(name=e.make_name(), date=e.entry.date(), lab_id=e.seq.lab_id(), passage=e.seq.passage(), lab=e.seq.lab(), gene=e.seq.gene(), seq_id=e.seq_id()),
            "d": e.entry.date(),
            }
        return r

    def get_sequence(e, left_part_size):
        e["s"] = e["e"].seq.amino_acids(aligned=aligned, left_part_size=left_part_size) if amino_acids else e["e"].seq.nucleotides(aligned=aligned, left_part_size=left_part_size)
        return e

    def left_part(e):
        return - (e["e"].seq.amino_acids_shift() if amino_acids else e["e"].seq.nucleotides_shift())

    def exclude_by_hamming_distance(e1, e2, threshold):
        hd = hamming_distance(e1["s"], e2["s"], e1["n"], e2["n"])
        if hd >= threshold:
            module_logger.info('{!r} excluded because hamming distance to {!r} is {} (threshold: {})'.format(e2["n"], e1["n"], hd, threshold))
            r = False
        else:
            r = True
        return r

    # ----------------------------------------------------------------------

    iter = (seqdb.iter_seq()
            .filter_lab(normalize.lab(lab) or "")
            .filter_subtype(normalize.virus_type(virus_type) or "")
            .filter_lineage(normalize.lineage(lineage) or "")
            .filter_aligned(aligned)
            .filter_gene(gene)
            .filter_date_range(normalize.date(start_date), normalize.date(end_date))
            .filter_hi_name(with_hi_name)
            )
    if name_match is not None:
        iter = iter.filter_name_regex(name_match)
    sequences = [make_entry(e) for e in iter]
    left_part_size = 0 if truncate_left else max(left_part(seq) for seq in sequences)
    if left_part_size:
        module_logger.info('Left part size (signal peptide): {}'.format(left_part_size))
    sequences = [get_sequence(seq, left_part_size) for seq in sequences]

    # report empty sequences
    empty = [seq for seq in sequences if not seq["s"]]
    if empty:
        module_logger.warning('The following sequences are empty and are not exported:\n  {}'.format("\n  ".join(seq["n"] for seq in empty)))

    # remove empty sequences
    sequences = [seq for seq in sequences if seq["s"]]
    if not sequences:
        raise ValueError("No sequences found for exporting")

    # avoid repeated names
    sequences.sort(key=operator.itemgetter("n"))
    prev_name = None
    repeat_no = 0
    for seq in sequences:
        if seq["n"] == prev_name:
            repeat_no += 1
            seq["n"] += "__" + str(repeat_no)
        else:
            prev_name = seq["n"]
            repeat_no = 0

    if sort_by:
        if sort_by == "date":
            sequences.sort(key=operator.itemgetter("d"))
        elif args.sort_by == "name":
            pass # already sorted by name -- sequences.sort(key=operator.itemgetter("n"))
        else:
            raise ValueError("Unrecognized sort_by argument")

    if start_date is not None:
        module_logger.info('Start date: ' + start_date)
    if end_date is not None:
        module_logger.info('End date:   ' + end_date)
    module_logger.info('{} sequences to export'.format(len(sequences)))

    # base seq is always the first one in the file, regardless of sorting, to ease specifying the outgroup for GARLI
    if base_seq:
        base_seqs = [get_sequence(make_entry(e), left_part_size) for e in seqdb.iter_seq().filter_name_regex(base_seq)]
        if len(base_seqs) != 1:
            raise ValueError("{} base sequences selected: {}".format(len(base_seqs), " ".join(repr(s.make_name()) for s in base_seqs)))
        module_logger.info('base_seq: {}'.format(base_seqs[0]["n"]))
        base_seq_present = [e_no for e_no, e in enumerate(sequences) if base_seqs[0]["n"] == e["n"]]
        if base_seq_present:
            # base seq is already there, remove it
            del sequences[base_seq_present[0]]
        sequences = base_seqs + sequences

    # convert to most common length BEFORE excluding by hamming distance threshold
    if truncate_to_most_common_length:
        truncate_to_most_common(sequences, fill="X" if amino_acids else "-")

    if hamming_distance_threshold:
        sequences = list(filter(lambda s: exclude_by_hamming_distance(sequences[0], s, hamming_distance_threshold), sequences))
    if len(sequences) < 2:
        raise ValueError("Too few ({}) sequences found for exporting".format(len(sequences)))

    if hamming_distance_report:
        hamming_distances = sorted(([e["n"], hamming_distance(sequences[0]["s"], e["s"], sequences[0]["n"], e["n"])] for e in sequences[1:]), key=operator.itemgetter(1), reverse=True)
    else:
        hamming_distances = None

    exp = exporter(output=str(filename), output_format=output_format, encode_name=encode_name, wrap=wrap)
    module_logger.info('Writing {} sequences'.format(len(sequences)))
    for ss in sequences:
        exp.write(name=ss["n"], sequence=ss["s"])
    return {"base_seq": fasta_encode_name(sequences[0]["n"]) if base_seq else None, "filename": filename, "hamming_distances": hamming_distances, "number_of_sequences": len(sequences)}

# ----------------------------------------------------------------------

def most_common_length(sequences):
    len_stat = collections.Counter(len(e["s"]) for e in sequences)
    return len_stat.most_common(1)[0][0]

# ----------------------------------------------------------------------

def truncate_to_most_common(sequences, fill):
    mclen = most_common_length(sequences)
    module_logger.info('Truncating/extending sequences to the most common length: {}'.format(mclen))
    for entry in sequences:
        ls = len(entry["s"])
        if ls > mclen:
            entry["s"] = entry["s"][:mclen]
        elif ls < mclen:
            entry["s"] += fill * (mclen - ls)
    return mclen

# ----------------------------------------------------------------------

def hamming_distance(s1, s2, n1, n2):
    l = min(len(s1), len(s2))
    hd = sum(1 for pos in range(l) if s1[pos] != s2[pos])
    # module_logger.debug('HD: {} {!r} {!r}'.format(hd, n1, n2))
    return hd

# ----------------------------------------------------------------------

def read_from_file(filename):
    """Yields tuple (name, sequence) for each entry in the file"""
    yield from read_from_string(open_file.open_for_reading_text(filename), filename)

# ----------------------------------------------------------------------

def read_from_string(source, filename):
    """Yields tuple (name, sequence) for each entry in the string"""
    sequence = []
    name = None

    for line_no, line in enumerate(source.splitlines(), start=1):
        if not line or line[0] == ';':               # empty or comment line
            pass
        elif line[0] == '>':
            if name or sequence:
                yield (name, _check_sequence("".join(sequence), name, filename, line_no))
            sequence = []
            name = line[1:].strip()
        else:
            if not name:
                raise FastaReaderError('{filename}:{line_no}: sequence without name'.format(filename=filename, line_no=line_no))
            sequence.append(line.replace("/", "-").upper()) # / found in H1pdm sequences
    if name:
        yield (name, _check_sequence("".join(sequence), name, filename, line_no))

# ----------------------------------------------------------------------

sReSequence = re.compile(r"^[A-Za-z\-~:\*\.]+$")

def _check_sequence(sequence, name, filename, line_no):
    if not sequence:
        raise FastaReaderError('{filename}:{line_no}: {name!r} without sequence'.format(name=name, filename=filename, line_no=line_no))
    if not sReSequence.match(sequence):
        raise FastaReaderError('{filename}:{line_no}: invalid sequence read: {sequence}'.format(sequence=sequence, filename=filename, line_no=line_no))
    return sequence

# ----------------------------------------------------------------------

def read_fasta(fasta_file):
    """Returns list of dict {"name":, "sequence":}"""

    def make_entry(raw_name, sequence):
        return {"sequence": sequence, "name": raw_name}

    r = [make_entry(raw_name, sequence) for raw_name, sequence in read_from_string(open_file.open_for_reading_text(fasta_file).read(), fasta_file)]
    module_logger.debug('{} sequences imported from {}'.format(len(r), fasta_file))
    return r

# ----------------------------------------------------------------------

def read_fasta_with_name_parsing(fasta_file, lab, virus_type, **_):
    """Returns list of dict {"name":, "sequence":, "date":, "lab":}"""
    np = name_parser()

    def make_entry(raw_name, sequence):
        n_entry = np.parse(raw_name, lab=lab)
        if not n_entry:
            raise RuntimeError("Cannot parse name: {!r}".format(raw_name))
        entry = {"sequence": sequence, "lab": lab, "virus_type": virus_type}
        entry.update(n_entry)
        return entry

    r = [make_entry(raw_name, sequence) for raw_name, sequence in read_from_string(open_file.open_for_reading_text(str(fasta_file)).read(), fasta_file)]
    module_logger.debug('{} sequences imported from {}'.format(len(r), fasta_file))
    return r

# ----------------------------------------------------------------------

sNameParser = None

def name_parser():
    global sNameParser
    if not sNameParser:
        sNameParser = NameParser()
    return sNameParser

class NameParser:

    def __init__(self):
        self.parsers = (
            (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]*)\s+\|\s+(?P<lab_id>[^\|]*)?\s+\|\s+(?P<lab>[A-Za-z ]+)\s+\|\s+(?P<virus_type>[AB]\s*/\s*H\d+N\d+)\s+\|\s*(?P<lineage>[A-Za-z0-9]+)?\s*$', re.I), self.gisaid), # name | date | passage | lab_id | lab | virus_type | lineage
            (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]*)\s+\|\s+(?P<lab_id>[^\|]*)?\s+\|\s+(?P<lab>[A-Za-z ]+)\s*$', re.I), self.gisaid), # name | date | passage | lab_id | lab
            (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]+)\s+\|\s+(?P<lab_id>[^\|]+)?\s+\|.*$', re.I), self.gisaid), # name | date | passage | lab_id | something-else
            (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]+)?\s*\|\s*(?P<lab_id>.+)?\s*$', re.I), self.gisaid), # name | date | passage? | lab_id
            (re.compile(r'^(?P<name1>EPI\d+)\s+\|\s+(?P<gene>HA|NA)\s+\|\s+(?P<designation>[^\|]+)\s+\|\s+(?P<name>EPI_[A-Z_0-9]+)\s+\|\s*(?P<passage>[^\s]+)?\s*\|\s*(?P<flu_type>.+)?\s*$', re.I), self.gisaid_melb), # name1 | gene | designation | name | passage | flu_type
            #(re.compile(r'^(?P<type>B|H3|H1)(?P<location>[A-Z]+)(?P<isolation_number>\d+)(?P<year>\d\d)[A-Z]*\s', re.I), self.nimr_glued),
            (re.compile(r'^(?P<name>[^\s]+)\s+PileUp\sof', re.I), self.nimr_20090914),
            (re.compile(r'^(?P<designation>[^|]+)\s+\|\s+(?P<passage>[^\s]+)\s*\|\s+(?P<name>(?:EPI|201)\d+)\s*$', re.I), self.cdc_20100913), # name | passage | fasta_id
            (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?P<passage>[^\s]+)\s+\|\s*(?P<lab_id>.+)?\s*$', re.I), self.gisaid_without_date), # name | passage | lab_id
            (re.compile(r'^(?P<name>[^\s]+)\s\s+(?P<date>[\d/]+)\s\s+(?P<passage>[^\s]+)\s*$', re.I), self.melb_20100823), # name  date  passage
            (re.compile(r'^(?P<name>\d+S\d+)\s+\"Contig\s+\d+\"\s+\(\d+,\d+\)$', re.I), self.melb_20110921),
            (re.compile(r'^(?P<name>[^_]+/\d\d\d\d)[\s_]+[^/]*(?:[\s_]?:(?P<passage>[EC]\d*))[^/]*$', re.I), self.name_passage), # CNIC
            (re.compile(r'^(?P<name>[^_]+/\d\d\d\d)[\s_]+(?:Jan|Feb|Mar|Apr|may|Jun|Jul|Aug|Sep|Oct|Nov|Dec)?[^/]*$', re.I), self.name_only), # CNIC
            (re.compile(r'^(?P<name>[^\s]+_4)$', re.I), self.name_only),   # CDC
            (re.compile(r'^(?P<name>[^_]+)[\s_]+[^/]*$', re.I), self.name_only), # CNIC
            (re.compile(r'^(?P<name>.+/\d\d\d\d)[\s\-]*(?P<passage>[\.\dA-Z]+)?$', re.I), self.name_passage), # NIID
            (re.compile(r'.'), self.simple),
            )

    def parse(self, raw_name, lab=None):
        for rex, func_name in self.parsers:
            m = rex.match(raw_name)
            if m:
                # module_logger.debug('NameParser {}'.format(func_name))
                return {k: v for k, v in func_name(raw_name, m, lab=lab).items() if v}
        return None

    def simple(self, raw_name, m, **_):
        return {'name': raw_name}

    # def nimr_glued(self, raw_name, m=None):
    #     def convert_type(t):
    #         if re.match(r'^H\d\d?$', t):
    #             t = 'A'
    #         return t
    #     return {'name': '/'.join((convert_type(m.group('type')), m.group('location'), m.group('isolation_number'), m.group('year')))}

    def nimr_20090914(self, raw_name, m, **_):
        return {'name': m.group('name').upper()}

    mReCdcId = re.compile(r'^\s*(?P<cdcid>\d{8,10})(?:_\d{6}_v\d(?:_\d)?|_\d|\s+.*)?$')

    def gisaid(self, raw_name, m, with_date=True, lab=None, **_):
        groups = m.groupdict()
        # module_logger.debug('gisaid with_date:{} {!r} --> {}'.format(with_date, raw_name, groups))
        year = (with_date and (groups.get('year1') or groups.get('year2') or groups.get('year3'))) or None
        try:
            lab = self._fix_gisaid_lab(m.group('lab'))   # do NOT use groups here!
        except IndexError:
            pass
        lab_id = groups.get('lab_id')
        # module_logger.debug('{} lab_id {} {}'.format(lab, lab_id, self.mReCdcId.match(lab_id)))
        if lab == "CDC" and lab_id is not None:
            m_cdcid = self.mReCdcId.match(lab_id)
            if m_cdcid:
                lab_id = m_cdcid.group("cdcid")
            else:
                if lab_id:
                    module_logger.warning('Not a cdcid: {}'.format(lab_id))
                lab_id = None
        return {
            'name': groups.get('name'),
            'date': year and '-'.join((year, groups.get('month1') or groups.get('month2') or '01', groups.get('day1') or '01')),
            'passage': groups.get('passage'),
            'lab_id': lab_id,
            'lab': lab,
            'virus_type': self._fix_gisaid_virus_type(groups.get("virus_type")),
            'lineage': groups.get("lineage"),
            }

    def _fix_gisaid_lab(self, lab):
        return (lab
                .replace("Centers for Disease Control and Prevention", "CDC")
                .replace("Crick Worldwide Influenza Centre", "NIMR")
                .replace("National Institute for Medical Research", "NIMR")
                .replace("WHO Collaborating Centre for Reference and Research on Influenza", "MELB")
                .replace("National Institute of Infectious Diseases (NIID)", "NIID")
                .replace("National Institute of Infectious Diseases", "NIID")
                .replace("Erasmus Medical Center", "EMC")
                )

    def _fix_gisaid_virus_type(self, virus_type):
        if virus_type:
            m = re.match(r"^\s*([AB])\s*/\s*(H\d+N\d+)\s*$", virus_type)
            if m:
                if m.group(1) == "B":
                    virus_type = "B"
                else:
                    virus_type = "A(" + m.group(2) + ")"
                pass
            else:
                raise ValueError("Unrecognized gisaid flu virus_type: " + virus_type)
        return virus_type

    def gisaid_without_date(self, raw_name, m, **kw):
        return self.gisaid(raw_name=raw_name, m=m, with_date=False, **kw)

    def gisaid_melb(self, raw_name, m, **_):
        return {'name': m.group('name'), 'gene': m.group('gene')}

    def melb_20100823(self, raw_name, m, **_):
        return {'name': m.group('name').upper(), 'date': m.group('date'), 'passage': m.group('passage')}

    def melb_20110921(self, raw_name, m, **_):
        return {'name': m.group('name').upper()}

    def name_only(self, raw_name, m, **_):
        return {'name': m.group('name').upper()}

    def cdc_20100913(self, raw_name, m, **_):
        return {'name': m.group('name').upper(), 'passage': m.group('passage')}

    def name_passage(self, raw_name, m, **_):
        return {'name': m.group('name').upper(), 'passage': m.group('passage')}

# ----------------------------------------------------------------------

# def generate_one(name, sequence, encode, split=True):
#     return ">{}\n{}\n".format((fasta_encode_name(name) if encode else name).strip(), (sequence_split(sequence) if split else sequence).strip())

# ----------------------------------------------------------------------

def fasta_encode_name(name):
    for char in "% :()!*';@&=+$,?#[]":
        name = name.replace(char, '%{:02X}'.format(ord(char)))
    return name

# ----------------------------------------------------------------------

# def sequence_split(sequence, chunk_len=75, separator="\n"):
#     if chunk_len and chunk_len > 0:
#         r = separator.join(sequence[i:i+chunk_len] for i in range(0, len(sequence), chunk_len))
#     else:
#         r = sequence
#     return r

# ----------------------------------------------------------------------

def exporter(output, output_format, encode_name, wrap):
    if output_format == "fasta":
        r = FastaExporter(output=output, encode_name=encode_name, wrap=wrap)
    elif output_format == "phylip":
        r = PhylipExporter(output=output, encode_name=encode_name, wrap=wrap)
    else:
        raise ValueError("Unrecognized output_format: {}".format(output_format))
    return r

# ----------------------------------------------------------------------

class ExporterBase:

    def __init__(self, output, encode_name, wrap):
        self.f = open_file.open_for_writing_binary(output)
        module_logger.info('Writing {}'.format(output))
        self._close_file = output != "-"
        self.encode_name = encode_name
        self.wrap = wrap

    def __del__(self):
        if self._close_file:
            self.f.close()

    def sequence_split(self, sequence, chunk_len=60, separator="\n"):
        if chunk_len and chunk_len > 0:
            r = separator.join(sequence[i:i+chunk_len] for i in range(0, len(sequence), chunk_len))
        else:
            r = sequence
        return r

    def make_name(self, name):
        if self.encode_name:
            name = fasta_encode_name(name)
        return name.strip()

class FastaExporter (ExporterBase):

    def write(self, name, sequence):
        self.f.write(">{}\n{}\n".format(self.make_name(name), (self.sequence_split(sequence) if self.wrap else sequence).strip()).encode("utf-8"))

class PhylipExporter (ExporterBase):

    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)
        self.names = []
        self.sequences = []

    def __del__(self):
        self.do_write()
        super().__del__()

    def write(self, name, sequence):
        self.names.append(self.make_name(name))
        self.sequences.append(sequence)

    def do_write(self):
        max_s_len = max(len(s) for s in self.sequences)
        max_n_len = max(len(n) for n in self.names)
        self.f.write("{} {}\n".format(len(sequences), max_s_len).encode("utf-8"))
        if self.wrap:
            raise NotImplementedError("phylip with wrapping")   # http://www.molecularevolution.org/resources/fileformats/phylip_dna
        else:
            for no, (n, s) in enumerate(zip(self.names, self.sequences)):
                self.f.write("{:<{}s}  {}{}\n".format(names[no], max_n_len, s, "-" * (max_s_len - len(s))).encode("utf-8"))

# ----------------------------------------------------------------------
# ----------------------------------------------------------------------
# ----------------------------------------------------------------------

def xx():
    xx

# ----------------------------------------------------------------------

# def export(data, output, output_format :str, truncate_to_most_common :bool, name_format, amino_acids :bool, aligned :bool, encode_name :bool, wrap :bool):
#     """
#     output: filename or "-"
#     output_format: "fasta", "phylip"
#     truncate_to_most_common: bool - make all sequence of the same length, use the most common length among all data
#     name_format: "{name} {date} {lab_id} {passage} {lab} {gene} {seq_id}"
#     amino_acids: bool
#     aligned: bool
#     encode_name: bool
#     wrap: bool - wrap lines with sequence
#     """

#     def make_name_with_lab(e):
#         labs = e.labs()
#         if labs:
#             lab = labs[0]
#             lab_ids = e.lab_ids(lab)
#             if lab_ids:
#                 lab_id = lab_ids[0]
#             else:
#                 lab_id = ""
#         else:
#             lab = ""
#             lab_id = ""
#         return re.sub(r"\s-", "-", re.sub(r"\s+", " ", name_format.format(name=e.name(), hi_name=e.name_hi(), passage=e.passage(), seq_id=e.seq_id(), date=e.date() or "NO-DATE", lab_id=lab_id, lab=lab, gene=e.gene())))

#     def make_name(e):
#         return re.sub(r"\s-", "-", re.sub(r"\s+", " ", name_format.format(name=e.name(), hi_name=e.name_hi(), passage=e.passage(), seq_id=e.seq_id(), date=e.date() or "NO-DATE", gene=e.gene())))

#     def make_sequence(e):
#         if amino_acids:
#             if aligned:
#                 sequence = e.aa_aligned()
#             else:
#                 sequence = e.aa()
#         else:
#             if aligned:
#                 sequence = e.nuc_aligned()
#             else:
#                 sequence = e.nuc()
#         return sequence

#     def truncate_extend(s, length):
#         if len(s) > length:
#             s = s[:length]
#         elif len(s) < length:
#             s = "{}{}".format(s, "-" * (length - len(s)))
#         return s

#     def export_fasta(f, data_to_write):
#         for n, s in data_to_write:
#             f.write(generate_one(name=n, sequence=s, encode=encode_name, split=wrap).encode("utf-8"))

#     def export_phylip(f, data_to_write):
#         if encode_name:
#             names = [fasta_encode_name(n) for n, s in data_to_write]
#         else:
#             names = [n for n, s in data_to_write]
#         max_s_len = max(len(s) for n, s in data_to_write)
#         max_n_len = max(len(n) for n in names)
#         f.write("{} {}\n".format(len(data_to_write), max_s_len).encode("utf-8"))
#         if wrap:
#             raise NotImplementedError("phylip with wrapping")   # http://www.molecularevolution.org/resources/fileformats/phylip_dna
#         else:
#             for no, (n, s) in enumerate(data_to_write):
#                 f.write("{:<{}s}  {}{}\n".format(names[no], max_n_len, s, "-" * (max_s_len - len(s))).encode("utf-8"))

#     if "lab" in name_format:
#         name_maker = make_name_with_lab
#     else:
#         name_maker = make_name
#     data_to_write = [(name_maker(e), make_sequence(e)) for e in data]

#     if truncate_to_most_common:
#         len_stat = collections.Counter(len(e[1]) for e in data_to_write)
#         most_common_length = len_stat.most_common(1)[0][0]
#         module_logger.info('Truncating/extending sequences to the most common length: {}'.format(most_common_length))
#         data_to_write = [(n, truncate_extend(s, most_common_length)) for n, s in data_to_write]

#     with open_file.open_for_writing_binary(output) as f:
#         if output_format == "fasta":
#             export_fasta(f, data_to_write)
#         elif output_format == "phylip":
#             export_phylip(f, data_to_write)
#         else:
#             raise ValueError("Unsupported output format: {}".format(output_format))
#     module_logger.info('{} written'.format(output))


# ----------------------------------------------------------------------

# def generate(filename, data, names_order=None, predicate=None, sequence_access=None, split_at=75):
#     module_logger.info('generating fasta into {}'.format(filename))
#     with open_file.open_for_writing_binary(filename) as f:
#         if isinstance(data, dict):
#             _generate_from_dict(f=f, data=data, names_order=names_order or sorted(data), predicate=predicate or (lambda n, s: True), sequence_access=sequence_access or (lambda a: a), split_at=split_at)
#         elif isinstance(data, list) and len(data) and isinstance(data[0], (list, tuple)) and len(data[0]) == 2:
#             _generate_from_list(f, data, split_at=split_at)
#         else:
#             raise ValueError("Cannot write {} to fasta (dict or list of (name, seq) pairs expected".format(type(data)))

# def _write_seq_to_file(f, name, seq, split_at):
#     f.write(b">")
#     f.write(fasta_encode_name(name).encode('utf-8'))
#     f.write(b"\n")
#     f.write(sequence_split(seq, chunk_len=split_at).encode('utf-8'))
#     f.write(b"\n")

# def _generate_from_dict(f, data, names_order, predicate, sequence_access, split_at):
#     for name in names_order:
#         seq = data[name]
#         if predicate(name, seq):
#             _write_seq_to_file(f, name, sequence_access(seq), split_at)

# def _generate_from_list(f, data, split_at):
#     for name, seq in data:
#         _write_seq_to_file(f, name, seq, split_at)

# # ----------------------------------------------------------------------

# sReNameDecoder = re.compile(r'%([0-9A-Fa-f][0-9A-Fa-f])')

# def decode_name(name):
#     def replace(match):
#         return chr(int(match.group(1), 16))
#     return sReNameDecoder.sub(replace, name)

# # ----------------------------------------------------------------------

# # def encode_name_old(name):
# #     return name.replace(' ', '^').replace(':', '%').replace('(', '<').replace(')', '>')

# # def decode_name_old(name):
# #     return name.replace('^', ' ').replace('%', ':').replace('<', '(').replace('>', ')')

# # ----------------------------------------------------------------------

# # ----------------------------------------------------------------------

# sReSubtype = re.compile(r'^(?:B/|A\((H3|H1))')

# def detect_subtype(sequences):
#     for name in sequences:
#         m = sReSubtype.match(name)
#         if m:
#             subtype = m.group(1) or "B"
#             logging.info('Subtype: {} ({})'.format(subtype, name))
#             return subtype
#     raise RuntimeError("cannot detect subtype by sequence names")

# # ----------------------------------------------------------------------

# def read(filename, duplicates="merge", report_duplicates=True, decode_names=True, uppercase_sequences=True):
#     """Returns dict {name: sequence}"""
#     return FastaReaderBasicDict(decode_names=decode_names, uppercase_sequences=uppercase_sequences).read(filename, duplicates=duplicates, report_duplicates=report_duplicates)

# def read_as_list(filename, decode_names=True, uppercase_sequences=True):
#     """Returns list of tuples (name, sequence) keeping the order from filename."""
#     return FastaReaderBasicList(decode_names=decode_names, uppercase_sequences=uppercase_sequences).read(filename)

# # ======================================================================

# class FastaReaderBasic:

#     def __init__(self, uppercase_sequences=False):
#         self.uppercase_sequences = uppercase_sequences

#     def read(self, filename, **kwargs):
#         return self.read_entries(self.open(filename), filename=os.path.basename(filename))

#     def open(self, filename):
#         self.filename = filename
#         source = open_file.open_for_reading_binary(filename)
#         return source

#     def read_entries(self, source, filename):
#         sequence = ''
#         raw_name = None
#         for line_no, line in enumerate(source, start=1):
#             try:
#                 line = line.decode('utf-8').strip()
#             except:
#                 raise FastaReaderError('{filename}:{line_no}: cannot decode line'.format(filename=filename, line_no=line_no))
#             if not line or line[0] == ';':               # empty or comment line
#                 pass
#             elif line[0] == '>':
#                 if raw_name:
#                     if not sequence:
#                         raise FastaReaderError('{filename}:{line_no}: {raw_name!r} without sequence'.format(raw_name=raw_name, filename=filename, line_no=line_no))
#                     sequence = self.normalize_sequence(sequence)
#                     self.check_sequence(sequence, line_no)
#                     yield {'raw_name': raw_name, 'sequence': sequence, 'source': filename}
#                 sequence = ''
#                 raw_name = line[1:]
#             else:
#                 if not raw_name:
#                     raise FastaReaderError('{filename}:{line_no}: sequence without name'.format(filename=filename, line_no=line_no))
#                 sequence += line
#         if raw_name:
#             if not sequence:
#                 raise FastaReaderError('{filename}:EOF: {raw_name!r} without sequence'.format(raw_name=raw_name, filename=filename))
#             sequence = self.normalize_sequence(sequence)
#             self.check_sequence(sequence, line_no)
#             yield {'raw_name': raw_name, 'sequence': sequence, 'source': filename}

#     sReSequence = re.compile(r"^[A-Za-z\-~:\*\.]+$")

#     def check_sequence(self, sequence, line_no):
#         if not self.sReSequence.match(sequence):
#             raise FastaReaderError('{filename}:{line_no}: invalid sequence read: {sequence}'.format(sequence=sequence, filename=self.filename, line_no=line_no))

#     def normalize_sequence(self, sequence):
#         sequence = sequence.replace("/", "-")   # / found in H1pdm sequences
#         if self.uppercase_sequences:
#             sequence = sequence.upper()
#         return sequence

# # ======================================================================

# class FastaReaderBasicDict (FastaReaderBasic):
#     """Gets dict of raw_name to sequence entries"""

#     def __init__(self, decode_names=True, uppercase_sequences=False):
#         super().__init__(uppercase_sequences=uppercase_sequences)
#         self.decode_names = decode_names

#     def read(self, filename, duplicates="merge", report_duplicates=True, **kwargs):
#         result = {}
#         for entry in self.read_entries(self.open(filename), filename=os.path.basename(filename)):
#             self.add_entry(result, entry, duplicates, report_duplicates)
#         return result

#     def add_entry(self, result, entry, duplicates, report_duplicates):
#         if self.decode_names:
#             name = decode_name(entry['raw_name'])
#         else:
#             name = entry['raw_name']
#         if name in result and not self.equal(entry['sequence'], result[name]):
#             if duplicates == "error":
#                 raise FastaReaderError("Duplicating name: {}, different sequences.".format(name))
#             elif duplicates == "merge":
#                 result[name].append(entry['sequence'])
#             if report_duplicates:
#                 module_logger.warning("Duplicating name: {}, different sequences.".format(name))
#         else:
#             if duplicates == "merge":
#                 result[name] = [entry['sequence']]
#             else:
#                 result[name] = entry['sequence']

#     def equal(self, new, existing):
#         if isinstance(existing, str):
#             r = new == existing
#         elif isinstance(existing, list):
#             r = any(new == e for e in existing)
#         else:
#             raise ValueError("Unsupported {}".format(type(existing)))
#         return r

# # ======================================================================

# class FastaReaderBasicList (FastaReaderBasic):
#     """Gets dict of raw_name to sequence entries"""

#     def __init__(self, decode_names=True, uppercase_sequences=False):
#         super().__init__(uppercase_sequences=uppercase_sequences)
#         self.decode_names = decode_names

#     def read(self, filename, **kwargs):
#         def name(entry):
#             if self.decode_names:
#                 return decode_name(entry['raw_name'])
#             else:
#                 return entry['raw_name']
#         return [(name(entry), entry['sequence']) for entry in self.read_entries(self.open(filename), filename=os.path.basename(filename))]

# # ======================================================================

# class FastaReaderWithNameNormalizing (FastaReaderBasic):

#     def __init__(self, lab=None, virus_type=None, uppercase_sequences=False):
#         super().__init__(uppercase_sequences=uppercase_sequences)
#         self.lab = lab
#         # from ..viruses import VirusesAPI
#         # self.viruses_api = VirusesAPI(lab=self.lab, virus_type=virus_type)

#     # def normalize_name(self, raw_name, name_type='whocc_fasta_match'):
#     #     name, location = self.viruses_api.normalize_name(raw_name, name_type=name_type)
#     #     return {k: v for k, v in (('name', name), ('country', location and location.country), ('continent', location and location.continent), ('latitude', location and location.latitude), ('longitude', location and location.longitude)) if v}

#     def normalize_name(self, raw_name, name_type='whocc_fasta_match'):
#         return {"name": raw_name}

#     def normalize_date(self, date):
#         return date
#         # try:
#         #     return date and parsers.datetime_parse_to_string(date, include_time=False)
#         # except parsers.ParsingError as err:
#         #     module_logger.warning(err)
#         #     return None

#     def normalize_passage(self, passage, report_prefix):
#         return passage
#         # try:
#         #     normalized = passage and parsers.passage_parse_to_string(lab=self.lab, source=passage, report_prefix='[PASSAGE] ' + (report_prefix or ''))
#         # except parsers.ParsingError as err:
#         #     module_logger.error('Passage {!r} parsing: {}'.format(passage, err))
#         #     normalized = passage
#         # return normalized

#     def normalize_lab_id(self, lab_id):
#         if self.lab == 'CDC' and lab_id and len(lab_id) > 10 and lab_id[10] == '_':
#             lab_id = lab_id[:10]
#         return lab_id

# # ======================================================================

# class FastaReaderWithNameParsing (FastaReaderWithNameNormalizing):

#     sParsers = (
#         #(re.compile(r'^(?P<type>B|H3|H1)(?P<location>[A-Z]+)(?P<isolation_number>\d+)(?P<year>\d\d)[A-Z]*\s', re.I), 'nimr_glued'),
#         (re.compile(r'^(?P<name>[^\s]+)\s+PileUp\sof', re.I), 'nimr_20090914'),
#         (re.compile(r'^(?P<designation>[^|]+)\s+\|\s+(?P<passage>[^\s]+)\s*\|\s+(?P<name>(?:EPI|201)\d+)\s*$', re.I), 'cdc_20100913'), # name | passage | fasta_id
#         (re.compile(r'^(?P<name1>EPI\d+)\s+\|\s+(?P<gene>HA|NA)\s+\|\s+(?P<designation>[^\|]+)\s+\|\s+(?P<name>EPI_[A-Z_0-9]+)\s+\|\s*(?P<passage>[^\s]+)?\s*\|\s*(?P<flu_type>.+)?\s*$', re.I), 'gisaid_melb'), # name1 | gene | designation | name | passage | flu_type
#         (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]*)\s+\|\s+(?P<lab_id>[^\|]*)?\s+\|\s+(?P<lab>[A-Za-z ]+)\s*$', re.I), 'gisaid'), # name | date | passage | lab_id | lab
#         (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]+)\s+\|\s+(?P<lab_id>[^\|]+)?\s+\|.*$', re.I), 'gisaid'), # name | date | passage | lab_id | something-else
#         (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?:(?P<year1>\d+)-(?P<month1>\d+)-(?P<day1>\d+)|(?P<year2>\d+)-(?P<month2>\d+)\s+\(day unknown\)|(?P<year3>\d+)\s+\(month and day unknown\))\s+\|\s+(?P<passage>[^\|]+)?\s*\|\s*(?P<lab_id>.+)?\s*$', re.I), 'gisaid'), # name | date | passage? | lab_id
#         (re.compile(r'^(?P<name>[^|]+)\s+\|\s+(?P<passage>[^\s]+)\s+\|\s*(?P<lab_id>.+)?\s*$', re.I), 'gisaid_without_date'), # name | passage | lab_id
#         (re.compile(r'^(?P<name>[^\s]+)\s\s+(?P<date>[\d/]+)\s\s+(?P<passage>[^\s]+)\s*$', re.I), 'melb_20100823'), # name  date  passage
#         (re.compile(r'^(?P<name>\d+S\d+)\s+\"Contig\s+\d+\"\s+\(\d+,\d+\)$', re.I), 'melb_20110921'),
#         (re.compile(r'^(?P<name>[^_]+/\d\d\d\d)[\s_]+[^/]*(?:[\s_]?:(?P<passage>[EC]\d*))[^/]*$', re.I), 'name_passage'), # CNIC
#         (re.compile(r'^(?P<name>[^_]+/\d\d\d\d)[\s_]+(?:Jan|Feb|Mar|Apr|may|Jun|Jul|Aug|Sep|Oct|Nov|Dec)?[^/]*$', re.I), 'name_only'), # CNIC
#         (re.compile(r'^(?P<name>[^\s]+_4)$', re.I), 'name_only'),   # CDC
#         (re.compile(r'^(?P<name>[^_]+)[\s_]+[^/]*$', re.I), 'name_only'), # CNIC
#         (re.compile(r'^(?P<name>.+/\d\d\d\d)[\s\-]*(?P<passage>[\.\dA-Z]+)?$', re.I), 'name_passage'), # NIID
#         (re.compile(r'.'), 'simple'),
#         )

#     def read(self, filename, name_type='whocc_fasta_match', **kwargs):
#         report_prefix = '[{}]'.format(os.path.basename(filename))
#         return (e for e in (self.postprocess(self.name_parse(entry, report_prefix=report_prefix), report_prefix=report_prefix, name_type=name_type) for entry in self.read_entries(self.open(filename), filename=os.path.basename(filename))) if e)

#     def postprocess(self, entry, report_prefix, name_type):
#         if entry.get('name'):
#             entry.update(self.normalize_name(entry['name'], name_type=name_type))
#         if entry.get('lab_id'):
#             entry['lab_id'] = self.normalize_lab_id(entry['lab_id'])
#         # if self.lab == 'CDC' and not entry.get('lab_id'):
#         #     module_logger.warning('[NOCDCID] {}: entry without lab_id removed: {}'.format(report_prefix or '', entry['raw_name'].strip()))
#         #     entry = None
#         return entry

#     def name_parse(self, entry, report_prefix=None):
#         for rex, func_name in self.sParsers:
#             m = rex.match(entry['raw_name'])
#             if m:
#                 # if func_name != 'simple':
#                 #     module_logger.info('{}'.format(func_name))
#                 entry.update(getattr(self, func_name)(entry['raw_name'], m, report_prefix=report_prefix))
#                 break
#         # module_logger.debug('name_parse {}'.format(entry))
#         return entry


# ======================================================================

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
