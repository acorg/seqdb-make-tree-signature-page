# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import re, subprocess, random, operator, time as time_m, datetime
from . import tree_maker
from .timeit import timeit

# ----------------------------------------------------------------------
# https://github.com/stamatak/standard-RAxML
# install-raxml
# docs: https://github.com/stamatak/standard-RAxML/blob/master/manual/NewManual.pdf
# ----------------------------------------------------------------------

class RaxmlResult (tree_maker.Result):

    def __init__(self, run_id, tree, score, start_scores, time):
        super().__init__(run_id=run_id, tree=tree, score=score, time=time)
        self.start_scores = start_scores

    def tabbed_report(self):
        return "{:10.4f} {:>8s} {:10.4f} {:10.4f} {}".format(self.score, self.time_str(self.time), self.start_scores[0], self.start_scores[1], str(self.tree))

# ----------------------------------------------------------------------

class RaxmlResults (tree_maker.Results):

    # def __init__(self, results, overall_time, submitted_tasks, survived_tasks):
    #     super().__init__(results=results, overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)

    def max_start_score(self):
        max_e_all = max(self.results, key=lambda e: max(e.score, *e.start_scores))
        return max(max_e_all.score, *max_e_all.start_scores)

    def tabbed_report_header(cls):
        return "{:^10s} {:^8s} {:^10s} {:^10s} {}".format("score", "time", "startscore", "endscore", "tree")

    @classmethod
    def import_from(cls, source_dir, overall_time, submitted_tasks, survived_tasks):
        return RaxmlResults((Raxml.get_result(source_dir, ".".join(tree.parts[-1].split(".")[1:])) for tree in source_dir.glob("RAxML_bestTree.*")), overall_time=overall_time, submitted_tasks=submitted_tasks, survived_tasks=survived_tasks)

# ----------------------------------------------------------------------

class RaxmlTask (tree_maker.Task):

    def __init__(self, job, output_dir, run_ids):
        super().__init__(job=job, output_dir=output_dir, run_ids=run_ids, progname="RAxML")

    # def wait(self, kill_rate=None, wait_timeout=None):
    #     self.wait_begin()
    #     if kill_rate:
    #         while self.job.wait(timeout=wait_timeout or 600) != "done":
    #             Raxml.analyse_logs(output_dir=self.output_dir, run_ids=self.run_ids, kill_rate=kill_rate, job=self.job)
    #     else:
    #         self.job.wait(timeout=wait_timeout)
    #     self.wait_end()
    #     return RaxmlResults.import_from(source_dir=self.output_dir, overall_time=self.overall_time, submitted_tasks=self.submitted_tasks, survived_tasks=len(self.run_ids))

    def results(self):
        if not self.finished():
            raise RuntimeError("Cannot get results: raxml job not finished")
        return RaxmlResults.import_from(source_dir=self.output_dir, overall_time=self.overall_time, submitted_tasks=self.submitted_tasks, survived_tasks=len(self.run_ids))

# ----------------------------------------------------------------------

class Raxml (tree_maker.Maker):

    def __init__(self, email):
        super().__init__(email, "raxml", "-v", re.compile(r"RAxML\s+version\s+([\d\.]+)"))
        self.threads = 1
        self.model = "GTRGAMMAI"
        self.default_args = ["-c", "4", "-f", "d", "--silent", "--no-seq-check"]

    # ----------------------------------------------------------------------

    def submit_htcondor(self, source :Path, source_tree, output_dir :Path, run_id, num_runs, bfgs, model_optimization_precision, outgroups :list, machines=None):
        from . import htcondor
        output_dir.mkdir(parents=True, exist_ok=True)
        general_args = ["-s", str(source.resolve()), "-w", str(output_dir.resolve()), "-m", self.model, "-e", str(model_optimization_precision), "-T", "1", "-N", "1"] + self.default_args
        if source_tree is not None:
            general_args += ["-t", str(source_tree)]
        if outgroups:
            general_args += ["-o", ",".join(outgroups)]
        run_ids = ["{}.{:04d}".format(run_id, run_no) for run_no in range(num_runs)]
        args = [(general_args + ["-n", ri, "-p", str(self.random_seed())]) for ri in run_ids]
        job = htcondor.submit(program=self.program,
                              program_args=args,
                              description="RAxML {run_id} {num_runs} {bfgs}".format(run_id=run_id, num_runs=num_runs, bfgs="" if bfgs else "no-bfgs"),
                              current_dir=output_dir,
                              capture_stdout=False, email=self.email, notification="Error", machines=machines)
        module_logger.info('Submitted RAxML: {}'.format(job))
        module_logger.info('RAxML parameters: bfgs:{} model_optimization_precision: {} outgroups:{}'.format(bfgs, model_optimization_precision, outgroups))
        return RaxmlTask(job=job, output_dir=output_dir, run_ids=run_ids)

    # ----------------------------------------------------------------------

    def run_locally(self, source, output_dir, run_id, num_runs=1, bfgs=True):
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        args = [self.program, "-s", str(source), "-n", str(run_id), "-w", str(output_dir), "-m", self.model, "-T", str(self.threads), "-p", str(self.random_seed()), "-N", str(num_runs)] + self.default_args
        if not bfgs:
            args.append("--no-bfgs")
        with timeit("RAxML local"):
            subprocess.run(args, stdout=subprocess.DEVNULL, check=True)
        return self.get_result(output_dir, run_id)

    # ----------------------------------------------------------------------

    sInfoBestScore = re.compile(r"Final GAMMA-based Score of best tree -([\d\.]+)", re.I)
    sInfoExecutionTime = re.compile(r"Overall execution time: ([\d\.]+) secs ", re.I)

    @classmethod
    def get_result(cls, output_dir, run_id):
        info_data = Path(output_dir, "RAxML_info." + run_id).open().read()
        m_score = cls.sInfoBestScore.search(info_data)
        if m_score:
            best_score = m_score.group(1)
        else:
            raise ValueError("Raxml: cannot extract best score from " + str(Path(output_dir, "RAxML_info." + run_id)))
        m_time = cls.sInfoExecutionTime.search(info_data)
        if m_time:
            execution_time = float(m_time.group(1))
        else:
            execution_time = None
        log_data = Path(output_dir, "RAxML_log." + run_id).open().readlines()
        start_scores = [- float(log_data[0].split()[1]), - float(log_data[-1].split()[1])]
        return RaxmlResult(run_id=run_id, tree=str(Path(output_dir, "RAxML_bestTree." + run_id)), score=float(best_score), start_scores=start_scores, time=execution_time)

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

        completed = [run_id for run_id in run_ids if Path(output_dir, "RAxML_bestTree." + run_id).exists()]
        if completed:
            best_completed = time_score_from_log(Path(output_dir, "RAxML_log." + run_id) for run_id in completed)
            # module_logger.info('completed: {} best: {}'.format(len(completed), best_completed))
            running_logs = [f for f in (Path(output_dir, "RAxML_log." + run_id) for run_id in run_ids if run_id not in completed) if f.exists()]
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

    #     completed = [run_id for run_id in run_ids if Path(output_dir, "RAxML_bestTree." + run_id).exists()]
    #     module_logger.info('analyse_logs completed: {}'.format(len(completed)))
    #     if completed:
    #         ff = [f for f in (Path(output_dir, "RAxML_log." + run_id) for run_id in run_ids if not Path(output_dir, "RAxML_bestTree." + run_id).exists()) if f.exists()] # just incomplete runs
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
    results = RaxmlResults.import_from(source_dir, overall_time=None, submitted_tasks=None, survived_tasks=None)
    module_logger.info('RAxML {}'.format(results.report_best()))
    results.make_txt(Path(target_dir, "result.raxml.txt"))
    results.make_json(Path(target_dir, "result.raxml.json"))
    make_r_score_vs_time(target_dir=target_dir, source_dir=source_dir, results=results)
    return results

# ----------------------------------------------------------------------

def make_r_score_vs_time(target_dir, source_dir, results):
    filepath = Path(target_dir, "raxml.score-vs-time.r")
    module_logger.info('Generating {}'.format(filepath))
    colors = {results.results[0].tree: "green", results.results[1].tree: "cyan", results.results[2].tree: "blue", results.results[-1].tree: "red"}
    with filepath.open("w") as f:
        f.write('doplot <- function(lwd) {\n')
        f.write('    plot(c(0, {longest_time}), c({min_score}, {max_score}), type="n", xlab="time (hours)", ylab="RAxML score", main="RAxML processing" )\n'.format(
            longest_time=results.longest_time / 3600,
            min_score=-results.results[0].score,
            max_score=-results.max_start_score()
            # max_score=-results.results[-1].score,
            ))
        f.write('    legend("bottomright", lwd=5, legend=c({trees}), col=c({colors}))\n'.format(
            trees=",".join(repr(t.split("/")[-1]) for t in sorted(colors)),
            colors=",".join(repr(colors[t]) for t in sorted(colors))))
        for r_e in reversed(results.results): # reversed for the best score line appear on top
            f.write('    d <- read.table("{log}")\n'.format(log=r_e.tree.replace("/RAxML_bestTree.", "/RAxML_log.")))
            f.write('    dlen <- length(d$V1)\n')
            f.write('    d$V1 <- d$V1 / 3600\n')
            color = colors.get(r_e.tree, "grey")
            f.write('    lines(d, col="{color}", lwd=lwd)\n'.format(color=color))
            f.write('    points(d$V1[dlen], d$V2[dlen], col="black")\n')
        f.write('}\n\n')
        f.write('pdf("{fn}", 10, 10)\n'.format(fn=filepath.with_suffix(".pdf")))
        f.write('doplot(lwd=0.1)\n')
        f.write('dev.off()\n\n')
        f.write('png("{fn}", 1200, 1200)\n'.format(fn=filepath.with_suffix(".png")))
        f.write('doplot(lwd=0.5)\n')
        f.write('dev.off()\n\n')
    subprocess.run(["Rscript", str(filepath)], stdout=subprocess.DEVNULL)
    module_logger.info('Plot {} generated'.format(filepath.with_suffix(".pdf")))

# ----------------------------------------------------------------------

"""
RAxML options http://sco.h-its.org/exelixis/resource/download/NewManual.pdf

-c 4
-f d
--silent
--no-seq-check
-m GTRGAMMAI
-T 1
-N 1
­p <random seed>
--no-bfgs

-e 0.001 (default) set model optimization precision in log likelihood units for final
      optimization of tree topology

"""

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
