# -*- Python -*-
# license
# license.

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import os, time as time_m, datetime, operator, random, subprocess, copy
from . import json

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
        if time is not None:
            s = str(datetime.timedelta(seconds=time))
        else:
            s = ""
        try:
            return s[:s.index('.')]
        except:
            return s

# ----------------------------------------------------------------------

class Results:

    def __init__(self, results=None, overall_time=None, submitted_tasks=None, survived_tasks=None):
        self.results = sorted(results, key=operator.attrgetter("score")) if results else []
        self.longest_time = max(self.results, key=operator.attrgetter("time")).time if self.results else 0
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
            if self.overall_time:
                f.write("Overall time:    " + Result.time_str(self.overall_time)+ "\n")
            if self.submitted_tasks:
                f.write("Submitted tasks: " + str(self.submitted_tasks) + "\n")
            if self.survived_tasks and self.survived_tasks != self.submitted_tasks:
                f.write("Survived tasks:  " + str(self.survived_tasks) + "\n")
            f.write("\n")
            f.write(self.tabbed_report_header()+ "\n")
            f.write("\n".join(rr.tabbed_report() for rr in self.results) + "\n")

    def make_json(self, filepath :Path):
        data = copy.copy(vars(self))
        data["results"] = [e.json() for e in data["results"]]
        json.dumpf(filepath, data)

    @classmethod
    def from_json(cls, filepath :Path):
        r = cls()
        data = json.loadf(filepath)
        for k, v in data.items():
            setattr(r, k, v)
        if getattr(r, "results", None) and isinstance(r.results[0], str):
            r.results = []                # bug in output, ignore it
        return r

# ----------------------------------------------------------------------

class Task:

    def __init__(self, job, output_dir, run_ids, progname):
        self.job = job
        self.output_dir = output_dir
        self.run_ids = run_ids
        self.progname = progname
        self.submitted_tasks = len(run_ids)
        self.start = None
        self.overall_time = None

    def started(self):
        return self.start is not None

    def finished(self):
        return self.overall_time is not None

    def wait(self, wait_timeout=None):
        if not self.started():
            self.wait_begin()
        if not self.finished():
            state = self.job.wait(timeout=wait_timeout)
            if state == "done":
                self.wait_end()

    def wait_begin(self):
        self.start = time_m.time()

    def wait_end(self):
        self.overall_time = time_m.time() - self.start
        module_logger.info(self.progname + ' jobs completed in ' + Result.time_str(self.overall_time))

# ----------------------------------------------------------------------

class Maker:

    def __init__(self, email, progname, version_switch, version_rex):
        self.email = email
        self.find_program(progname, version_switch, version_rex)
        self.random_gen = random.SystemRandom()

    def random_seed(self):
        return self.random_gen.randint(1, 0xFFFFFFF)   # note max for garli is 0x7ffffffe

    def find_program(self, progname, version_switch, version_rex):
        import socket
        hostname = socket.getfqdn()
        # module_logger.debug('hostname {}'.format(hostname))
        if hostname == "jagd":
            bin_dir = "/Users/eu/ac/bin"
        elif hostname == "albertine.antigenic-cartography.org":
            bin_dir = "/syn/bin"
        else:
            bin_dir = None
        if bin_dir:
            program = os.path.join(bin_dir, progname)
        else:
            program = progname
        if version_switch:
            prog_ver = program + " " + version_switch
        else:
            prog_ver = program
        # output = subprocess.check_output(prog_ver, shell=True, stderr=subprocess.STDOUT).decode("utf-8")
        output = subprocess.run(prog_ver, shell=True, check=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.decode("utf-8")
        m = version_rex.search(output)
        if m:
            module_logger.info('{} {}'.format(progname, m.group(1)))
        else:
            raise ValueError("Unrecognized {} version\n{}".format(progname, output))
        self.program = program

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
