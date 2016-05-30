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
        return "{:10.4f} {:>8s} {}".format(self.score, self.time_str(self.time), str(self.tree))

# ----------------------------------------------------------------------

class FasttreeResults (tree_maker.Results):

    # def __init__(self, results, overall_time, submitted_tasks, survived_tasks):
    #     super().__init__(results=results, overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)

    def max_start_score(self):
        max_e_all = max(self.results, key=lambda e: max(e.score, *e.start_scores))
        return max(max_e_all.score, *max_e_all.start_scores)

    def tabbed_report_header(cls):
        return "{:^10s} {:^8s} {:^10s} {:^10s} {}".format("score", "time", "startscore", "endscore", "tree")

    @classmethod
    def import_from(cls, source_dir, overall_time, submitted_tasks, survived_tasks):
        return FasttreeResults((Fasttree.get_result(source_dir, ".".join(tree.parts[-1].split(".")[1:])) for tree in source_dir.glob("Fasttree_bestTree.*")), overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)

# ----------------------------------------------------------------------

class FasttreeTask (tree_maker.Task):

    def __init__(self, job, output_dir, run_ids):
        super().__init__(job=job, output_dir=output_dir, run_ids=run_ids, progname="Fasttree")

    def wait(self, kill_rate=None, wait_timeout=None):
        self.wait_begin()
        if kill_rate:
            while self.job.wait(timeout=wait_timeout or 600) != "done":
                Fasttree.analyse_logs(output_dir=self.output_dir, run_ids=self.run_ids, kill_rate=kill_rate, job=self.job)
        else:
            self.job.wait()
        self.wait_end()
        return FasttreeResults.import_from(source_dir=self.output_dir, overall_time=self.overall_time, submitted_tasks=self.submitted_tasks, survived_tasks=len(self.run_ids))

# ----------------------------------------------------------------------

class Fasttree (tree_maker.Maker):

    def __init__(self, email):
        super().__init__(email, "fasttree", "", re.compile(r"FastTree\s+version\s+([\d\.]+)"))
        self.default_args = ["-nt", "-gtr", "-gamma", "-quiet"]

    # ----------------------------------------------------------------------

    def submit_htcondor(self, source, output_dir, run_id, num_runs, machines=None):
        from . import htcondor
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        general_args = ["-s", str(source.resolve()), "-w", str(output_dir.resolve()), "-m", self.model, "-e", str(model_optimization_precision), "-T", "1", "-N", "1"] + self.default_args
        run_ids = ["{}.{:04d}".format(run_id, run_no) for run_no in range(num_runs)]
        args = [(general_args + ["-n", ri, "-p", str(self._random_seed())]) for ri in run_ids]
        job = htcondor.submit(program=self.program,
                              program_args=args,
                              description="Fasttree {run_id} {num_runs} {bfgs}".format(run_id=run_id, num_runs=num_runs, bfgs="" if bfgs else "no-bfgs"),
                              current_dir=output_dir,
                              capture_stdout=False, email=self.email, notification="Error", machines=machines)
        module_logger.info('Submitted Fasttree: {}'.format(job))
        module_logger.info('Fasttree parameters: bfgs:{} model_optimization_precision: {} outgroups:{}'.format(bfgs, model_optimization_precision, outgroups))
        return FasttreeTask(job=job, output_dir=output_dir, run_ids=run_ids)

    # ----------------------------------------------------------------------

    sInfoBestScore = re.compile(r"Final GAMMA-based Score of best tree -([\d\.]+)", re.I)
    sInfoExecutionTime = re.compile(r"Overall execution time: ([\d\.]+) secs ", re.I)

    @classmethod
    def get_result(cls, output_dir, run_id):
        info_data = Path(output_dir, "Fasttree_info." + run_id).open().read()
        m_score = cls.sInfoBestScore.search(info_data)
        if m_score:
            best_score = m_score.group(1)
        else:
            raise ValueError("Fasttree: cannot extract best score from " + str(Path(output_dir, "Fasttree_info." + run_id)))
        m_time = cls.sInfoExecutionTime.search(info_data)
        if m_time:
            execution_time = float(m_time.group(1))
        else:
            execution_time = None
        log_data = Path(output_dir, "Fasttree_log." + run_id).open().readlines()
        start_scores = [- float(log_data[0].split()[1]), - float(log_data[-1].split()[1])]
        return FasttreeResult(run_id=run_id, tree=str(Path(output_dir, "Fasttree_bestTree." + run_id)), score=float(best_score), start_scores=start_scores, time=execution_time)

    # ----------------------------------------------------------------------
    # for "survived" mode

    @classmethod
    def analyse_logs(cls, output_dir, run_ids, kill_rate, job):

        def load_log_file(filepath):
            for attempt in range(10):
                try:
                    r = [{"t": float(e[0]), "s": -float(e[1]), "f": str(filepath).split(".")[-1]} for e in (line.strip().split() for line in filepath.open())]
                    if not r:   # pending
                        r = [{"t": 0, "s": 0}]
                    return r
                except ValueError as err:
                    pass        # file is being written at the moment, try again later
                    module_logger.info('(ignored) cannot process {}: {}'.format(filepath.name, err))
                time_m.sleep(3)
            raise RuntimeError("Cannot process {}".format(filepath))

        def time_score_from_log(files):
            return min((load_log_file(filepath)[-1] for filepath in files), key=operator.itemgetter("s"))

        completed = [run_id for run_id in run_ids if Path(output_dir, "Fasttree_bestTree." + run_id).exists()]
        if completed:
            best_completed = time_score_from_log(Path(output_dir, "Fasttree_log." + run_id) for run_id in completed)
            # module_logger.info('completed: {} best: {}'.format(len(completed), best_completed))
            running_logs = [f for f in (Path(output_dir, "Fasttree_log." + run_id) for run_id in run_ids if run_id not in completed) if f.exists()]
            data = {int(str(f).split(".")[-1]): load_log_file(f) for f in running_logs}
            scores_for_longer_worse_than_best_completed = {k: v[-1]["s"] for k, v in data.items() if v[-1]["t"] > best_completed["t"] and v[-1]["s"] > best_completed["s"]}
            by_score = sorted(scores_for_longer_worse_than_best_completed, key=lambda e: scores_for_longer_worse_than_best_completed[e])
            n_to_kill = int(len(by_score) * kill_rate)
            if n_to_kill > 0:
                to_kill = by_score[-n_to_kill:]
                module_logger.info('completed: {} best: {} worse_than_best_completed: {} to kill: {}'.format(len(completed), best_completed, by_score, to_kill))
                job.kill_tasks(to_kill)
                run_id_to_del = [ri for ri in run_ids if int(ri.split(".")[-1]) in to_kill]
                # module_logger.info('run_id_to_del {}'.format(run_id_to_del))
                for ri in run_id_to_del:
                    run_ids.remove(ri)
                # module_logger.info('To kill {}: {} run_ids left: {}'.format(n_to_kill, to_kill, run_ids))
                # module_logger.info('To kill {}: {}'.format(n_to_kill, to_kill))
            # else:
            #     module_logger.info('Nothing to kill')

    # @classmethod
    # def analyse_logs_old1(cls, output_dir, run_ids, kill_rate, job):

    #     def load_log_file(filepath):
    #         for attempt in range(10):
    #             try:
    #                 r = [{"t": float(e[0]), "s": -float(e[1])} for e in (line.strip().split() for line in filepath.open())]
    #                 if not r:   # pending
    #                     r = [{"t": 0, "s": 0}]
    #                 return r
    #             except ValueError as err:
    #                 pass        # file is being written at the moment, try again later
    #                 module_logger.info('(ignored) cannot process {}: {}'.format(filepath.name, err))
    #             time_m.sleep(3)
    #         raise RuntimeError("Cannot process {}".format(filepath))

    #     completed = [run_id for run_id in run_ids if Path(output_dir, "Fasttree_bestTree." + run_id).exists()]
    #     module_logger.info('analyse_logs completed: {}'.format(len(completed)))
    #     if completed:
    #         ff = [f for f in (Path(output_dir, "Fasttree_log." + run_id) for run_id in run_ids if not Path(output_dir, "Fasttree_bestTree." + run_id).exists()) if f.exists()] # just incomplete runs
    #         data = {int(str(f).split(".")[-1]): load_log_file(f) for f in ff}
    #         scores = {k: v[-1]["s"] for k,v in data.items()}
    #         by_score = sorted(scores, key=lambda e: scores[e])
    #         module_logger.info('Scores\n  {}'.format("  \n".join("{:04d} {}".format(k, scores[k]) for k in by_score)))
    #         n_to_kill = int(len(by_score) * kill_rate)
    #         if n_to_kill > 0:
    #             to_kill = by_score[-n_to_kill:]
    #             job.kill_tasks(to_kill)
    #             for run_id_to_del in (ri for ri in run_ids if int(ri.split(".")[-1]) in to_kill):
    #                 run_ids.remove(run_id_to_del)
    #             module_logger.info('To kill {}: {} run_ids left: {}'.format(n_to_kill, to_kill, run_ids))
    #         else:
    #             module_logger.info('Nothing to kill')

    # ----------------------------------------------------------------------

# ----------------------------------------------------------------------

def postprocess(target_dir, source_dir):
    results = FasttreeResults.import_from(source_dir, overall_time=None, submitted_tasks=None, survived_tasks=None)
    module_logger.info('Fasttree {}'.format(results.report_best()))
    results.make_txt(Path(target_dir, "result.fasttree.txt"))
    results.make_json(Path(target_dir, "result.fasttree.json"))
    make_r_score_vs_time(target_dir=target_dir, source_dir=source_dir, results=results)
    return results

# ----------------------------------------------------------------------


# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
