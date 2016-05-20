# -*- Python -*-
# license
# license.

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path

# ----------------------------------------------------------------------

class Error (Exception):
    pass

# ----------------------------------------------------------------------

class Result:

    def __init__(self, run_id, tree, score, time):
        self.run_id = run_id
        self.tree = tree
        self.score = score
        self.time = time

    def __repr__(self):
        return str(vars(self))

    def json(self):
        return vars(self)

    @classmethod
    def time_str(cls, time):
        s = str(datetime.timedelta(seconds=time))
        try:
            return s[:s.index('.')]
        except:
            return s

# ----------------------------------------------------------------------

class Results:

    def __init__(self, results=None, overall_time=None, submitted_tasks=None, survived_tasks=None):
        self.results = sorted(results, key=operator.attrgetter("score")) if results else []
        self.longest_time = max(self.results, key=operator.attrgetter("time")).time if results else 0
        self.overall_time = overall_time
        self.submitted_tasks = submitted_tasks
        self.survived_tasks = survived_tasks

    def recompute(self):
        self.results.sort(key=operator.attrgetter("score"))
        self.longest_time = max(self.results, key=operator.attrgetter("time")).time

    def longest_time_str(self):
        return Result.time_str(self.longest_time)

    def best_tree(self):
        return self.results[0].tree

    def report_best(self):
        return "{} {} {}".format(self.results[0].score, self.longest_time_str(), self.best_tree())

    def make_txt(self, filepath :Path):
        with filepath.open("w") as f:
            f.write("Longest time:    " + self.longest_time_str()+ "\n")
            f.write("Overall time:    " + Result.time_str(self.overall_time)+ "\n")
            if self.submitted_tasks:
                f.write("Submitted tasks: " + str(self.submitted_tasks) + "\n")
            if self.survived_tasks and self.survived_tasks != self.submitted_tasks:
                f.write("Survived tasks:  " + str(self.survived_tasks) + "\n")
            f.write("\n")
            f.write(self.tabbed_report_header()+ "\n")
            f.write("\n".join(rr.tabbed_report() for rr in self.results) + "\n")

    def make_json(self, filepath :Path):
        json.dumpf(filepath, vars(self))

# ----------------------------------------------------------------------

class Task:

    def __init__(self, job, output_dir, run_ids, progname):
        self.job = job
        self.output_dir = output_dir
        self.run_ids = run_ids
        self.progname = progname
        self.submitted_tasks = len(run_ids)

    def wait_begin(self):
        self.start = time_m.time()

    def wait_end(self):
        self.overall_time = time_m.time() - self.start
        module_logger.info(self.progname + ' jobs completed in ' + Result.time_str(self.overall_time))

# ----------------------------------------------------------------------



# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
