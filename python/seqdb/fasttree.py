# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import re, subprocess, operator, time as time_m, datetime
from . import tree_maker
from .timeit import timeit

# ----------------------------------------------------------------------
# http://www.microbesonline.org/fasttree/
# ----------------------------------------------------------------------

class FasttreeResult (tree_maker.Result):

    def tabbed_report(self):
        return str(self.tree)

# ----------------------------------------------------------------------

class FasttreeResults (tree_maker.Results):

    # def __init__(self, results, overall_time, submitted_tasks, survived_tasks):
    #     super().__init__(results=results, overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)

    def tabbed_report_header(cls):
        return "{}".format("tree")

    @classmethod
    def import_from(cls, source_dir, overall_time, submitted_tasks):
        return FasttreeResults((Fasttree.get_result(source_dir, tree.stem) for tree in source_dir.glob("*" + Fasttree.tree_file_suffix)), overall_time=overall_time, submitted_tasks=submitted_tasks)

# ----------------------------------------------------------------------

class FasttreeTask (tree_maker.Task):

    def __init__(self, job, output_dir, run_ids):
        super().__init__(job=job, output_dir=output_dir, run_ids=run_ids, progname="Fasttree")

    def wait(self, kill_rate=None, wait_timeout=None):
        self.wait_begin()
        self.job.wait()
        self.wait_end()
        return FasttreeResults.import_from(source_dir=self.output_dir, overall_time=self.overall_time, submitted_tasks=self.submitted_tasks)

# ----------------------------------------------------------------------

class Fasttree (tree_maker.Maker):

    tree_file_suffix = ".phy"

    def __init__(self, email):
        super().__init__(email, "fasttree", "", re.compile(r"FastTree\s+version\s+([\d\.]+)"))
        self.default_args = ["-nt", "-gtr", "-gamma", "-nopr", "-nosupport"]

    # ----------------------------------------------------------------------

    def submit_htcondor(self, source, output_dir, run_id, num_runs=1, machines=None):
        from . import htcondor
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        run_ids = ["{}.{:04d}".format(run_id, run_no) for run_no in range(num_runs)]
        args = [(self.default_args + ["-out", ri + self.tree_file_suffix, "-seed", str(self._random_seed()), str(source.resolve())]) for ri in run_ids]
        job = htcondor.submit(program=self.program,
                              program_args=args,
                              description="Fasttree {run_id} {num_runs}".format(run_id=run_id, num_runs=num_runs),
                              current_dir=output_dir,
                              capture_stdout=False, email=self.email, notification="Error", machines=machines)
        module_logger.info('Submitted Fasttree: {}'.format(job))
        return FasttreeTask(job=job, output_dir=output_dir, run_ids=run_ids)

    # ----------------------------------------------------------------------

    sInfoBestScore = re.compile(r"Final GAMMA-based Score of best tree -([\d\.]+)", re.I)
    sInfoExecutionTime = re.compile(r"Overall execution time: ([\d\.]+) secs ", re.I)

    @classmethod
    def get_result(cls, output_dir, run_id):
        return FasttreeResult(run_id=run_id, tree=str(Path(output_dir, run_id + self.tree_file_suffix)), score=0, time=None)

# ----------------------------------------------------------------------

def postprocess(target_dir, source_dir):
    results = FasttreeResults.import_from(source_dir, overall_time=None, submitted_tasks=None)
    module_logger.info('Fasttree {}'.format(results.report_best()))
    results.make_txt(Path(target_dir, "result.fasttree.txt"))
    results.make_json(Path(target_dir, "result.fasttree.json"))
    return results

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
