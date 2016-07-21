# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import os, re, operator, pprint
from pathlib import Path
import logging; module_logger = logging.getLogger(__name__)
from . import open_file

# ----------------------------------------------------------------------

# See acmacs.whocc.hidb

sLocationKey = "L"
sLineageKey = "l"
sLabIdsKey = "i"
sDatesKey = "d"
sNameKey = "N"
sOtherNamesKey = "o"

sVariantsKey = "v"
sTablesKey = "t"
sPassageKey = "p"
sReassortantKey = "r"
sAnnotationsKey = "a"
sExtraKey = "e"
sSerumIdKey = "s"
sSerumSpeciesKey = "f"

# ----------------------------------------------------------------------

class HiDb:

    # HiDb format decription is in "legend" in ~/WHO/hidb.*.json.xz

    def __init__(self, dirname):
        self.db = {
            "A(H1N1)": self._load(dirname, "h1pdm"),
            "A(H3N2)": self._load(dirname, "h3"),
            "B":       self._load(dirname, "b"),
            }
        self.ids = None

    def _load(self, dirname, infix):
        filename = Path(dirname, "hidb." + infix + ".json.xz")
        data = open_file.read_json(filename)
        if data.get("  version") != "whocc-antigens-sera-v3":
            raise ValueError("{}: unsupported hidb version: {}".format(filename, data.get("  version")))
        return data

    def find_antigen_by_cdcid(self, cdcid, virus_type=None):
        if self.ids is None:
            self.ids = {vt: {aid: antigen for antigen in vt_db["antigens"] for aid in antigen.get("i", [])} for vt, vt_db in self.db.items()}
        look_for = "CDC#{}".format(cdcid)
        if virus_type is not None:
            if self.ids.get(virus_type):
                ag = self.ids[virus_type].get(look_for)
            else:
                ag = None
        else:
            for ids in self.ids.values():
                ag = ids.get(look_for)
                if ag is not None:
                    break
            else:
                ag = None
        return ag

    def find_antigen_by_name(self, name, virus_type=None):
        if virus_type is not None:
            if self.db.get(virus_type):
                dbs = [self.db[virus_type]["antigens"]]
            else:
                dbs = []
        elif name[0] == "A" and name[8] == ["/"]:
            dbs = [self.db[name[:7]]["antigens"]]
        elif name[:2] == "B/":
            dbs = [self.db[name[0]]["antigens"]]
        else:
            dbs = [db["antigens"] for db in self.db.values()]
        for db in dbs:
            ag = self.find_antigen_by_name_in_db(name, db)
            if ag is not None:
                break
        else:
            ag = None
        return ag

    # Adoption if python 3.5 bisect module function
    def find_antigen_by_name_in_db(self, name, db):
        lo = 0
        hi = len(db)
        while lo < hi:
            mid = (lo + hi) // 2
            mid_name = db[mid][sNameKey]
            if mid_name == name:
                return db[mid]
            if mid_name < name:
                lo = mid + 1
            else:
                hi = mid
        return None

    # --------------------------------------------------

    sLevelReassortantMismatch = 99999
    sLevelToExclude = sLevelReassortantMismatch

    def match(self, name, seq_passages_reassortant, hi_entry):
        """Returns list of matches. Each match is a dict: {"s": seq_passage, "r": seq_reassortant, "h": hi_variant}"""

        # if there is just one possibility, we have to check its level, because it can be sLevelReassortantMismatch
        # # if len(seq_passages_reassortant) == 1 and len(hi_entry[sVariantsKey]) == 1:
        # #     # just one possibility, always matches regardless of actual passages and extra
        # #     r = [{"p": seq_passages_reassortant[0]["p"][0], "r": seq_passages_reassortant[0]["r"], "h": hi_entry[sVariantsKey][0]}]
        # # else:

        # if "BOLIVIA/559/2013" in name:
        #     module_logger.debug('M0 {}\nSEQ: {}\nHI: {}\n'.format(name, pprint.pformat(seq_passages_reassortant, width=200), pprint.pformat(hi_entry, width=200)))

        seq_left = seq_passages_reassortant[:]
        hi_left = hi_entry[sVariantsKey][:]
        r = []
        while seq_left and hi_left:
            levels = sorted(({"level": self._match_level(seq_p, hi_variant, reassortant=seq_group["r"]), "p": seq_p, "r": seq_group["r"], "hi_variant": hi_variant, "hi_variant_no": hi_variant_no, "seq_group_no": seq_group_no} for seq_group_no, seq_group in enumerate(seq_left) for seq_p in seq_group["p"] for hi_variant_no, hi_variant in enumerate(hi_left)), key=operator.itemgetter("level"))
            # if "B/TEXAS/2/2013" in name:
            #     module_logger.debug('{} seq:[{} {}] hi:[{} {}]\n{}\n'.format(name, levels[0]["r"], levels[0]["p"], levels[0]["hi_variant"].get("r"), levels[0]["hi_variant"]["p"], pprint.pformat(levels, width=300)))
            if levels[0]["level"] < self.sLevelToExclude:
                r.append({"p": levels[0]["p"], "r": levels[0]["r"], "h": hi_left.pop(levels[0]["hi_variant_no"])})
            else:
                module_logger.warning('Nothing to match\n      {}\n      Seq: {}\n      HI: {}\n      Level: {}'.format(name, seq_left, levels[0]['hi_variant'], "reassortant-mismatch" if levels[0]['level'] == self.sLevelToExclude else levels[0]['level']))
            del seq_left[levels[0]["seq_group_no"]]
        #module_logger.debug('match_passages\n--> {}\n  {}\n  {}'.format(r, seq_passages_reassortant, hi_variants_passages))
        return r

    sRePassage = re.compile("".join(r"(?:(?P<p{}>[A-Z]+[\d\?]*)/?)?".format(i) for i in range(10)) + r"(?:\s*\((?P<date>\d+-\d+-\d+)\))?")

    def _match_level(self, sp, hi_variant, reassortant):
        hi_passage = hi_variant.get(sPassageKey)
        if hi_variant.get(sReassortantKey, "") not in reassortant:
            # reassortant do not match, not consider at all
            level = self.sLevelReassortantMismatch
        elif not sp:
            level = 90
            if hi_passage == "X?":
                level -= 1
        elif not hi_passage:
            level = 91
        elif sp == hi_passage:
            level = 0
        else:
            sm = self.sRePassage.match(sp)
            hm = self.sRePassage.match(hi_passage)
            if not sm or not hm:
                module_logger.warning('Cannot parse passage {!r} {!r}'.format(sp, hi_passage))
                level = 200
            else:
                spp = [sm.group(k) for k in sorted(sm.groupdict()) if k != "date" and sm.group(k)]
                hpp = [hm.group(k) for k in sorted(hm.groupdict()) if k != "date" and hm.group(k)]
                if spp and hpp:
                    if spp == hpp:
                        # matches without date
                        if sm.group("date") and hm.group("date"):
                            level = 11        # different dates
                        else:
                            level = 10        # one of the dates missing
                    elif spp[-1] == hpp[-1]:
                        level = 20
                        for i in range(2, min(len(spp), len(hpp)) + 1):
                            if spp[-i] == hpp[-i]:
                                level -= 1
                            else:
                                break
                    elif self._same_passage_type(spp[-1], hpp[-1]):
                        level = 30
                        for i in range(2, min(len(spp), len(hpp)) + 1):
                            if self._same_passage_type(spp[-i], hpp[-i]):
                                level -= 1
                            else:
                                break
                    elif self._close_passage_type(spp[-1], hpp[-1]):
                        level = 40
                        for i in range(2, min(len(spp), len(hpp)) + 1):
                            if self._close_passage_type(spp[-i], hpp[-i]):
                                level -= 1
                            else:
                                break
                    else:
                        # module_logger.warning('{} {} {} --- {} {} {}'.format(sp, spp, sm.group("date"), hp, hpp, hm.group("date")))
                        level = 100
                else:
                    level = 110
        # if annotatitions != hi_variant.get(sAnnotationsKey):
        #     level += 2
        # if extra != hi_variant.get(sExtraKey):
        #     level += 1
        return level

    sReSplitPassageNumber = re.compile(r"^([A-Z]+)")

    def _same_passage_type(self, sp, hp):
        sp_m = self.sReSplitPassageNumber.match(sp)
        hp_m = self.sReSplitPassageNumber.match(hp)
        return sp_m and hp_m and sp_m.group(1) == hp_m.group(1)

    def _close_passage_type(self, sp, hp):
        sp_m = self.sReSplitPassageNumber.match(sp)
        hp_m = self.sReSplitPassageNumber.match(hp)
        sp_p = sp_m and ("MDCK" if sp_m.group(1) == "SIAT" else sp_m.group(1))
        hp_p = hp_m and ("MDCK" if hp_m.group(1) == "SIAT" else hp_m.group(1))
        return sp_p and hp_p and sp_p == hp_p

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
