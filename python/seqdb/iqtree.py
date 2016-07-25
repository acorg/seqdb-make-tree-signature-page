# -*- Python -*-
# license
# license.

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import re, random, subprocess, time as time_m
from . import tree_maker

# ----------------------------------------------------------------------

class IqtreeResult (tree_maker.Result):

    def __init__(self, run_id, tree, score, start_score, time):
        super().__init__(run_id=run_id, tree=tree, score=score, time=time)
        self.start_score = start_score

    def tabbed_report(self):
        return "{:10.4f} {:>8s} {:10.4f} {}".format(self.score, self.time_str(self.time), self.start_score, str(self.tree))

# ----------------------------------------------------------------------

class IqtreeResults (tree_maker.Results):

    def tabbed_report_header(cls):
        return "{:^10s} {:^8s} {:^10s} {}".format("score", "time", "startscore", "tree")

    @classmethod
    def import_from(cls, source_dir, overall_time, submitted_tasks, survived_tasks):
        if Path(source_dir, "001").is_dir():
            r = IqtreeResults(None, overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)
            for subdir in source_dir.glob("*"):
                if subdir.is_dir():
                    r.results.extend(Iqtree.get_result(output_dir=subdir, run_id=".".join(tree.parts[-1].split(".")[:-2])) for tree in subdir.glob("*.best.phy"))
            r.recompute()
        else:
            r = IqtreeResults((Iqtree.get_result(output_dir=source_dir, run_id=".".join(tree.parts[-1].split(".")[:-2])) for tree in source_dir.glob("*.best.phy")), overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)
        return r

# ----------------------------------------------------------------------

class IqtreeTask (tree_maker.Task):

    def __init__(self, job, output_dir, run_ids):
        super().__init__(job=job, output_dir=output_dir, run_ids=run_ids, progname="IQTREE")

    def wait(self, wait_timeout=None):
        self.wait_begin()
        self.job.wait(timeout=wait_timeout)
        self.wait_end()
        return IqtreeResults((Iqtree.get_result(output_dir=self.output_dir, run_id=ri) for ri in self.run_ids), overall_time=self.overall_time, submitted_tasks=self.submitted_tasks, survived_tasks=len(self.run_ids))

# ----------------------------------------------------------------------

class Iqtree (tree_maker.Maker):

    def __init__(self, email):
        super().__init__(email, "iqtree", "-h", re.compile(r"IQ-TREE\s+version\s+([\d\.]+(?:-beta)?)", re.I))
        self.email = email
        self._find_program()
        self.random_gen = random.SystemRandom()
        self.model = "GTR+I+G4"           # K3Pu+I+G4
        self.default_args = ["-cptime", "600"]            # "-keep-ident",

    def submit_htcondor(self, num_runs, source, output_dir, run_id, outgroups :list, machines=None):
        from . import htcondor
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        run_ids = ["{}.{:04d}".format(run_id, run_no) for run_no in range(num_runs)]

        general_args = ["-s", str(source.resolve()), "-m", self.model] + self.default_args
        if outgroups:
            general_args += ["-o", ",".join(outgroups)]
        run_ids = ["{}.{:04d}".format(run_id, run_no) for run_no in range(num_runs)]
        args = [(general_args + ["-pre", ri, "-seed", str(self.random_seed())]) for ri in run_ids]
        job = htcondor.submit(program=self.program,
                              program_args=args,
                              description="IQTREE {run_id} {num_runs}".format(run_id=run_id, num_runs=num_runs),
                              current_dir=output_dir,
                              capture_stdout=False, email=self.email, notification="Error", machines=machines)
        module_logger.info('Submitted IQTREE: {}'.format(job))
        module_logger.info('IQTREE parameters: bfgs:{} model_optimization_precision: {} outgroups:{}'.format(bfgs, model_optimization_precision, outgroups))
        return IqtreeTask(job=job, output_dir=output_dir, run_ids=run_ids)

    # ----------------------------------------------------------------------

    # sReIqtreeScore = re.compile(r"\[!IqtreeScore\s+-(\d+\.\d+)\]")
    # sReIqtreeTime = re.compile(r"Time used = (\d+) hours, (\d+) minutes and (\d+) seconds")

    @classmethod
    def get_result(cls, output_dir, run_id):
        tree = Path(output_dir, run_id + ".best.phy")
        score, execution_time, start_score = None, None, None
        logfile = Path(output_dir, run_id + ".log00.log")
        for log_line in logfile.open():
            fields = log_line.split("\t")
            if len(fields) == 4:
                if fields[0] == "Final":
                    score = - float(fields[1])
                    execution_time = int(fields[2])
                elif fields[0] == "0":
                    start_score = - float(fields[1])
        if score is None or execution_time is None or start_score is None:
            raise RuntimeError("Unable to parse " + str(logfile))
        return IqtreeResult(run_id=run_id, tree=str(tree), score=score, start_score=start_score, time=execution_time)

    # ----------------------------------------------------------------------

    def _make_conf(self, run_id, source, source_tree, output_dir, outgroup, attachmentspertaxon, randseed, genthreshfortopoterm, searchreps, stoptime, strip_comments):
        """Returns filename of the written conf file"""
        if not outgroup or not isinstance(outgroup, list) or not all(isinstance(e, int) and e > 0 for e in outgroup):
            raise ValueError("outgroup must be non-empty list of taxa indices in the fasta file starting with 1")
        global IQTREE_CONF
        iqtree_args = {
            "source": str(Path(source).resolve()),
            "streefname": str(Path(source_tree).resolve()) if source_tree else "stepwise",
            "output_prefix": str(Path(output_dir, run_id)),
            "attachmentspertaxon": attachmentspertaxon, # 2000,      # default: 50
            "randseed": randseed,
            "genthreshfortopoterm": genthreshfortopoterm,    # termination condition, default: 20000
            "searchreps": searchreps,                  # default: 2
            "stoptime": stoptime,
            "outgroup": " ".join(str(e) for e in outgroup),
            }
        conf = IQTREE_CONF.format(**iqtree_args)
        if strip_comments:
            conf = "\n".join(line for line in conf.splitlines() if line.strip() and line.strip()[0] != "#")
        conf_filename = Path(output_dir, run_id + ".iqtree.conf")
        with conf_filename.open("w") as f:
            f.write(conf)
        return conf_filename

# ----------------------------------------------------------------------

def postprocess(target_dir, source_dir):
    results = IqtreeResults.import_from(source_dir, overall_time=None, submitted_tasks=None, survived_tasks=None)
    module_logger.info('IQTREE {}'.format(results.report_best()))
    results.make_txt(Path(target_dir, "result.iqtree.txt"))
    results.make_json(Path(target_dir, "result.iqtree.json"))
    return results

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
