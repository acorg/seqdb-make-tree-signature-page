# -*- Python -*-
# license
# license.

# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import os, operator, subprocess
from . import json
from .raxml import Raxml, RaxmlResult, make_r_score_vs_time
from .garli import Garli, GarliResults
from .fasttree import Fasttree, FasttreeResults

# ======================================================================

class BasicRunner:

    def __init__(self, working_dir, wait_timeout, seqdb, run_id, fasta_file, number_of_sequences, base_seq_name, raxml_settings, garli_settings, email, machines):
        self.seqdb = seqdb
        self.wait_timeout = wait_timeout
        self.raxml_output_dir = Path(working_dir, "raxml")
        self.garli_output_dir = Path(working_dir, "garli")
        self.settings = {
            "working_dir": working_dir,
            "run_id": run_id.replace(' ', '-').replace('/', '-'), # raxml cannot handle spaces and slashes in run-id
            "raxml": raxml_settings,
            "garli": garli_settings,
            "base_seq_name": base_seq_name,
            "number_of_sequences": number_of_sequences,
            "fasta_file": str(Path(fasta_file).resolve()),
            "email": email,
            "machines": machines,
            }
        self.settings["raxml"]["output_dir"] = self.raxml_output_dir
        self.settings["garli"]["output_dir"] = self.garli_output_dir
        self.state = "init"               # init, raxml_submitted, garli_submitted, completed
        self.save_settings()

    def save_settings(self):
        json.dumpf(Path(self.settings["working_dir"], "settings.json"), self.settings)

    def finished(self):
        return self.state == "completed"

    def iteration(self, **kwargs):
        getattr(self, "on_state_" + self.state)(**kwargs)   # on_state_* must be provided by derived classes

    def raxml_submit(self, submit=True):
        raxml = Raxml(email=self.settings["email"])
        self.raxml_task = raxml.submit_htcondor(
            num_runs=self.settings["raxml"]["num_runs"],
            source=Path(self.settings["fasta_file"]),
            source_tree=self.raxml_source_tree(),
            output_dir=self.settings["raxml"]["output_dir"],
            run_id=self.settings["run_id"],
            bfgs=self.settings["raxml"]["bfgs"],
            model_optimization_precision=self.settings["raxml"]["model_optimization_precision"],
            outgroups=[self.settings["base_seq_name"]],
            machines=self.settings["machines"],
            submit=submit)
        if submit:
            self.state = "raxml_submitted"
        else:
            self.state = "raxml_submission_prepared"

    def raxml_completed(self):
        self.raxml_results = self.raxml_task.results()
        self.raxml_results.make_txt(Path(self.settings["working_dir"], "result.raxml.txt"))
        self.raxml_results.make_json(Path(self.settings["working_dir"], "result.raxml.json"))
        make_r_score_vs_time(target_dir=self.settings["working_dir"], source_dir=self.settings["raxml"]["output_dir"], results=self.raxml_results)

    def raxml_source_tree(self):
        return None

    def garli_submit(self, tree):
        garli = Garli(email=self.settings["email"])
        self.garli_task = garli.submit_htcondor(
            num_runs=self.settings["garli"]["num_runs"],
            source=self.settings["fasta_file"],
            source_tree=tree,
            output_dir=self.settings["garli"]["output_dir"],
            run_id=self.settings["run_id"],
            attachmentspertaxon=self.settings["garli"]["attachmentspertaxon"],
            stoptime=self.settings["garli"]["stoptime"],
            machines=self.settings["machines"])
        self.state = "garli_submitted"

    def garli_completed(self):
        self.garli_results = self.garli_task.results()
        module_logger.info('GARLI {}'.format(self.garli_results.report_best()))
        self.garli_results.make_txt(Path(self.settings["working_dir"], "result.garli.txt"))
        self.garli_results.make_json(Path(self.settings["working_dir"], "result.garli.json"))

    def make_results_for_self(self):
        self.results = self.make_results(raxml_results=self.raxml_results, garli_results=self.garli_results, working_dir=self.settings["working_dir"], seqdb=self.seqdb)

    def on_state_completed(self, **kwargs):
        pass

    @classmethod
    def make_results(cls, raxml_results, garli_results, working_dir, seqdb):
        longest_time = raxml_results.longest_time + garli_results.longest_time
        longest_time_s = RaxmlResult.time_str(longest_time)
        module_logger.info('Longest time: ' + longest_time_s)
        if raxml_results.overall_time and garli_results.overall_time:
            overall_time = raxml_results.overall_time + garli_results.overall_time
            overall_time_s = RaxmlResult.time_str(overall_time)
            module_logger.info('Overall time: ' + overall_time_s)
        else:
            overall_time = None
            overall_time_s = ""

        r_best = vars(garli_results.results[0])
        r_best["longest_time"] = longest_time
        r_best["longest_time_s"] = longest_time_s
        if overall_time:
            r_best["overall_time"] = overall_time
        if overall_time_s:
            r_best["overall_time_s"] = overall_time_s
        json.dumpf(Path(working_dir, "result.best.json"), r_best)

        with Path(working_dir, "result.all.txt").open("w") as f:
            f.write("Longest time: " + longest_time_s + "\n")
            if overall_time_s:
                f.write("Overall time: " + overall_time_s + "\n")
            f.write("GARLI score : " + str(r_best["score"]) + "\n")
            f.write("Tree        : " + str(r_best["tree"]) + "\n")
        results = {
            " total": {
                "longest_time": longest_time,
                "longest_time_s": longest_time_s,
                "overall_time": overall_time,
                "overall_time_s": overall_time_s,
                "tree": str(garli_results.results[0].tree),
                "garli_score": garli_results.results[0].score,
                },
            "garli": [vars(r) for r in garli_results.results],
            "raxml": [r if isinstance(r, dict) else vars(r) for r in raxml_results.results],
            }
        json.dumpf(Path(working_dir, "result.all.json"), results)

        from .draw_tree import draw_tree
        draw_tree(tree_file=r_best["tree"],
                  seqdb=seqdb,
                  output_file=Path(working_dir, "tree.pdf"),
                  title="{{virus_type}} GARLI-score: {} Time: {} ({})".format(r_best["score"], overall_time_s, longest_time_s),
                  pdf_width=1000, pdf_height=850
                  )
        return results

# ----------------------------------------------------------------------

class RaxmlBestGarli (BasicRunner):

    def on_state_init(self, submit=True, **kwargs):
        self.raxml_submit(submit=submit)

    def on_state_raxml_submitted(self, **kwargs):
        self.raxml_task.wait(self.wait_timeout)
        if self.raxml_task.finished():
            self.raxml_completed()
            self.garli_submit(self.raxml_results.best_tree())

    def on_state_garli_submitted(self, **kwargs):
        self.garli_task.wait(self.wait_timeout)
        if self.garli_task.finished():
            self.garli_completed()
            self.make_results_for_self()
            self.state = "completed"

# ----------------------------------------------------------------------

class RaxmlSurvivedBestGarli (RaxmlBestGarli):

    def on_state_raxml_submitted(self, **kwargs):
        self.raxml_task.wait(self.wait_timeout)
        if not self.raxml_task.finished():
            Raxml.analyse_logs(output_dir=self.raxml_output_dir, run_ids=self.raxml_task.run_ids, kill_rate=self.settings["raxml"]["kill_rate"], job=self.raxml_task.job)
        else:
            self.raxml_completed()
            self.garli_submit(self.raxml_results.best_tree())

# ----------------------------------------------------------------------

class FasttreeRaxmlSurvivedBestGarli (RaxmlSurvivedBestGarli):

    def __init__(self, *a, **kw):
        raise NotImplementedError()

    def on_state_init(self, submit=True, **kwargs):
        self.run_fasttree()

# ----------------------------------------------------------------------

class RaxmlAllGarli (BasicRunner):

    def __init__(self, *a, **kw):
        raise NotImplementedError()

    def on_state_init(self, submit=True, **kwargs):
        self.raxml_submit(submit=submit)

# ----------------------------------------------------------------------

def run_raxml_best_garli(working_dir, seqdb, run_id, fasta_file, number_of_sequences, base_seq_name, raxml_kill_rate, raxml_bfgs, raxml_model_optimization_precision, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, garli_stoptime, email, machines):
    """Run RAxML raxml_num_runs times, take the best result and run GARLI garli_num_runs times starting with that best result."""
    runner = RaxmlBestGarli(
        working_dir=working_dir, wait_timeout=None, seqdb=seqdb, run_id=run_id, fasta_file=fasta_file, number_of_sequences=number_of_sequences, base_seq_name=base_seq_name,
        raxml_settings={"kill_rate": raxml_kill_rate, "bfgs": raxml_bfgs, "model_optimization_precision": raxml_model_optimization_precision, "num_runs": raxml_num_runs},
        garli_settings={"num_runs": garli_num_runs, "attachmentspertaxon": garli_attachmentspertaxon, "stoptime": garli_stoptime},
        email=email, machines=machines)
    while not runner.finished():
        runner.iteration()
    return runner.results()

    # run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id
    # save_settings(working_dir=working_dir, run_id=run_id, mode="best", fasta_file=Path(fasta_file).resolve(), number_of_sequences=number_of_sequences, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # r_raxml = run_raxml(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, source_tree=None, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, email=email, machines=machines)
    # r_garli = run_garli(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, tree=r_raxml.best_tree(), garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # return make_results(working_dir=working_dir, r_raxml=r_raxml, r_garli=r_garli, seqdb=seqdb)

# ----------------------------------------------------------------------

def run_raxml_survived_best_garli(working_dir, seqdb, run_id, fasta_file, number_of_sequences, base_seq_name, raxml_kill_rate, raxml_bfgs, raxml_model_optimization_precision, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, garli_stoptime, email, machines):
    """Run RAxML raxml_num_runs times with raxml_kill_rate, take the best result and run GARLI garli_num_runs times starting with that best result."""
    runner = RaxmlSurvivedBestGarli(
        working_dir=working_dir, wait_timeout=600, seqdb=seqdb, run_id=run_id, fasta_file=fasta_file, number_of_sequences=number_of_sequences, base_seq_name=base_seq_name,
        raxml_settings={"kill_rate": raxml_kill_rate, "bfgs": raxml_bfgs, "model_optimization_precision": raxml_model_optimization_precision, "num_runs": raxml_num_runs},
        garli_settings={"num_runs": garli_num_runs, "attachmentspertaxon": garli_attachmentspertaxon, "stoptime": garli_stoptime},
        email=email, machines=machines)
    while not runner.finished():
        runner.iteration()
    return runner.results()

    # run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id
    # save_settings(working_dir=working_dir, run_id=run_id, mode="survived-best", fasta_file=Path(fasta_file).resolve(), number_of_sequences=number_of_sequences, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # r_raxml = run_raxml_survived(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, source_tree=None, base_seq_name=base_seq_name, raxml_kill_rate=raxml_kill_rate, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, email=email, machines=machines)
    # r_garli = run_garli(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, tree=r_raxml.best_tree(), garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # return make_results(working_dir=working_dir, r_raxml=r_raxml, r_garli=r_garli, seqdb=seqdb)

# ----------------------------------------------------------------------

def run_fasttree_raxml_survived_best_garli(working_dir, seqdb, run_id, fasta_file, number_of_sequences, base_seq_name, raxml_kill_rate, raxml_bfgs, raxml_model_optimization_precision, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, garli_stoptime, email, machines):
    """Runs fasttree once, run RAxML raxml_num_runs times with raxml_kill_rate, take the best result and run GARLI garli_num_runs times starting with that best result."""
    runner = FasttreeRaxmlSurvivedBestGarli(
        working_dir=working_dir, wait_timeout=600, seqdb=seqdb, run_id=run_id, fasta_file=fasta_file, number_of_sequences=number_of_sequences, base_seq_name=base_seq_name,
        raxml_settings={"kill_rate": raxml_kill_rate, "bfgs": raxml_bfgs, "model_optimization_precision": raxml_model_optimization_precision, "num_runs": raxml_num_runs},
        garli_settings={"num_runs": garli_num_runs, "attachmentspertaxon": garli_attachmentspertaxon, "stoptime": garli_stoptime},
        email=email, machines=machines)
    while not runner.finished():
        runner.iteration()
    return runner.results()

    # run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id
    # save_settings(working_dir=working_dir, run_id=run_id, mode="fasttree-survived-best", fasta_file=Path(fasta_file).resolve(), number_of_sequences=number_of_sequences, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # r_fasttree = run_fasttree(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, email=email, machines=machines)
    # r_raxml = run_raxml_survived(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, source_tree=r_fasttree.best_tree(), base_seq_name=base_seq_name, raxml_kill_rate=raxml_kill_rate, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, email=email, machines=machines)
    # r_garli = run_garli(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, tree=r_raxml.best_tree(), garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # return make_results(working_dir=working_dir, r_raxml=r_raxml, r_garli=r_garli, seqdb=seqdb)

# ----------------------------------------------------------------------

def run_raxml_all_garli(working_dir, seqdb, run_id, fasta_file, number_of_sequences, base_seq_name, raxml_kill_rate, raxml_bfgs, raxml_model_optimization_precision, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, garli_stoptime, email, machines):
    """Run RAxML raxml_num_runs times, take all the results and run GARLI garli_num_runs times starting with each result."""
    runner = RaxmlAllGarli(
        working_dir=working_dir, wait_timeout=None, seqdb=seqdb, run_id=run_id, fasta_file=fasta_file, number_of_sequences=number_of_sequences, base_seq_name=base_seq_name,
        raxml_settings={"kill_rate": raxml_kill_rate, "bfgs": raxml_bfgs, "model_optimization_precision": raxml_model_optimization_precision, "num_runs": raxml_num_runs},
        garli_settings={"num_runs": garli_num_runs, "attachmentspertaxon": garli_attachmentspertaxon, "stoptime": garli_stoptime},
        email=email, machines=machines)
    while not runner.finished():
        runner.iteration()
    return runner.results()

    # run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id
    # save_settings(working_dir=working_dir, run_id=run_id, mode="all", fasta_file=Path(fasta_file).resolve(), number_of_sequences=number_of_sequences, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # r_raxml = run_raxml(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, source_tree=None, base_seq_name=base_seq_name, raxml_kill_rate=raxml_kill_rate, raxml_bfgs=raxml_bfgs, raxml_model_optimization_precision=raxml_model_optimization_precision, raxml_num_runs=raxml_num_runs, email=email, machines=machines)
    # r_garli = run_garli_multi(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, trees=[r.tree for r in r_raxml.results], garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, garli_stoptime=garli_stoptime, email=email, machines=machines)
    # return make_results(working_dir=working_dir, r_raxml=r_raxml, r_garli=r_garli, seqdb=seqdb)

# ======================================================================

def run_in_background(working_dir):
    log_file = Path(working_dir, "stdout.log")
    if os.fork() == 0:
        # child
        f = log_file.open("w")
        os.dup2(f.fileno(), 1)
        os.dup2(f.fileno(), 2)
        os.close(0)
    else:
        # parent
        module_logger.info('Log is written to {}'.format(log_file))
        exit(0)

# ======================================================================

# def save_settings(working_dir, **args):
#     json.dumpf(Path(working_dir, "settings.json"), args)

# ----------------------------------------------------------------------

# def run_raxml(working_dir, run_id, fasta_file, source_tree, base_seq_name, raxml_bfgs, raxml_model_optimization_precision, raxml_num_runs, email, machines):
#     raxml_output_dir = Path(working_dir, "raxml")
#     raxml = Raxml(email=email)
#     raxml_job = raxml.submit_htcondor(num_runs=raxml_num_runs, source=fasta_file, source_tree=source_tree, output_dir=raxml_output_dir,
#                                       run_id=run_id, bfgs=raxml_bfgs, model_optimization_precision=raxml_model_optimization_precision,
#                                       outgroups=[base_seq_name], machines=machines)
#     r_raxml = raxml_job.wait()
#     module_logger.info('RAxML {}'.format(r_raxml.report_best()))
#     r_raxml.make_txt(Path(working_dir, "result.raxml.txt"))
#     r_raxml.make_json(Path(working_dir, "result.raxml.json"))
#     make_r_score_vs_time(target_dir=working_dir, source_dir=raxml_output_dir, results=r_raxml)
#     return r_raxml

# # ----------------------------------------------------------------------

# def run_raxml_survived(working_dir, run_id, fasta_file, source_tree, base_seq_name, raxml_kill_rate, raxml_bfgs, raxml_model_optimization_precision, raxml_num_runs, email, machines):
#     raxml_output_dir = Path(working_dir, "raxml")
#     raxml = Raxml(email=email)
#     raxml_job = raxml.submit_htcondor(num_runs=raxml_num_runs, source=fasta_file, source_tree=source_tree, output_dir=raxml_output_dir,
#                                       run_id=run_id, bfgs=raxml_bfgs, model_optimization_precision=raxml_model_optimization_precision,
#                                       outgroups=[base_seq_name], machines=machines)
#     r_raxml = raxml_job.wait(kill_rate=raxml_kill_rate, wait_timeout=60)
#     module_logger.info('RAxML {}'.format(r_raxml.report_best()))
#     r_raxml.make_txt(Path(working_dir, "result.raxml.txt"))
#     r_raxml.make_json(Path(working_dir, "result.raxml.json"))
#     make_r_score_vs_time(target_dir=working_dir, source_dir=raxml_output_dir, results=r_raxml)
#     return r_raxml

# # ----------------------------------------------------------------------

# def run_garli(working_dir, run_id, fasta_file, tree, garli_num_runs, garli_attachmentspertaxon, garli_stoptime, email, machines):
#     garli_output_dir = Path(working_dir, "garli")
#     garli = Garli(email=email)
#     garli_job = garli.submit_htcondor(num_runs=garli_num_runs, source=fasta_file, source_tree=tree, output_dir=garli_output_dir,
#                                       run_id=run_id, attachmentspertaxon=garli_attachmentspertaxon, stoptime=garli_stoptime, machines=machines)
#     r_garli = garli_job.wait()
#     module_logger.info('GARLI {}'.format(r_garli.report_best()))
#     r_garli.make_txt(Path(working_dir, "result.garli.txt"))
#     r_garli.make_json(Path(working_dir, "result.garli.json"))
#     return r_garli

# # ----------------------------------------------------------------------

# def run_garli_multi(working_dir, run_id, fasta_file, trees, garli_num_runs, garli_attachmentspertaxon, garli_stoptime, email, machines):
#     jobs = []
#     for no, tree in enumerate(trees, start=1):
#         no_s = "{:03d}".format(no)
#         garli_output_dir = Path(working_dir, "garli", no_s)
#         jobs.append(
#             Garli(email=email).submit_htcondor(
#                 num_runs=garli_num_runs, source=fasta_file, source_tree=tree, output_dir=garli_output_dir,
#                 run_id=run_id + "." + no_s, attachmentspertaxon=garli_attachmentspertaxon, stoptime=garli_stoptime, machines=machines))
#     r_garli = GarliResults(None)
#     for job in jobs:
#         r_job = job.wait()
#         r_garli.results.extend(r_job.results)
#     r_garli.recompute()
#     module_logger.info('GARLI (multi {}) {}'.format(len(jobs), r_garli.report_best()))
#     r_garli.make_txt(Path(working_dir, "result.garli.txt"))
#     r_garli.make_json(Path(working_dir, "result.garli.json"))
#     return r_garli

# # ----------------------------------------------------------------------

# def run_fasttree(working_dir, run_id, fasta_file, email, machines):
#     fasttree_output_dir = Path(working_dir, "fasttree")
#     fasttree = Fasttree(email=email)
#     fasttree_job = fasttree.submit_htcondor(num_runs=1, source=fasta_file, output_dir=fasttree_output_dir,
#                                       run_id=run_id, machines=machines)
#     r_fasttree = fasttree_job.wait()
#     module_logger.info('Fasttree {}'.format(r_fasttree.report_best()))
#     r_fasttree.make_txt(Path(working_dir, "result.fasttree.txt"))
#     r_fasttree.make_json(Path(working_dir, "result.fasttree.json"))
#     return r_fasttree

# ----------------------------------------------------------------------

# def make_results(working_dir, r_raxml, r_garli, seqdb):
#     longest_time = r_raxml.longest_time + r_garli.longest_time
#     longest_time_s = RaxmlResult.time_str(longest_time)
#     module_logger.info('Longest time: ' + longest_time_s)
#     if r_raxml.overall_time and r_garli.overall_time:
#         overall_time = r_raxml.overall_time + r_garli.overall_time
#         overall_time_s = RaxmlResult.time_str(overall_time)
#         module_logger.info('Overall time: ' + overall_time_s)
#     else:
#         overall_time = None
#         overall_time_s = ""

#     r_best = vars(r_garli.results[0])
#     r_best["longest_time"] = longest_time
#     r_best["longest_time_s"] = longest_time_s
#     if overall_time:
#         r_best["overall_time"] = overall_time
#     if overall_time_s:
#         r_best["overall_time_s"] = overall_time_s
#     json.dumpf(Path(working_dir, "result.best.json"), r_best)

#     with Path(working_dir, "result.all.txt").open("w") as f:
#         f.write("Longest time: " + longest_time_s + "\n")
#         if overall_time_s:
#             f.write("Overall time: " + overall_time_s + "\n")
#         f.write("GARLI score : " + str(r_best["score"]) + "\n")
#         f.write("Tree        : " + str(r_best["tree"]) + "\n")

#     r = {
#         " total": {
#             "longest_time": longest_time,
#             "longest_time_s": longest_time_s,
#             "overall_time": overall_time,
#             "overall_time_s": overall_time_s,
#             "tree": str(r_garli.results[0].tree),
#             "garli_score": r_garli.results[0].score,
#             },
#         "garli": [vars(r) for r in r_garli.results],
#         "raxml": [vars(r) for r in r_raxml.results],
#         }
#     json.dumpf(Path(working_dir, "result.all.json"), r)

#     from .draw_tree import draw_tree
#     draw_tree(tree_file=r_best["tree"],
#               seqdb=seqdb,
#               output_file=Path(working_dir, "tree.pdf"),
#               title="{{virus_type}} GARLI-score: {} Time: {} ({})".format(r_best["score"], overall_time_s, longest_time_s),
#               pdf_width=1000, pdf_height=850
#               )
#     return r

# # ----------------------------------------------------------------------

def make_r_garli_start_final_scores(working_dir, results):
    if results["garli"]:
        res = sorted(results["garli"], key=operator.itemgetter("start_score"))
        min_score = - min(min(e["score"] for e in res), min(e["start_score"] for e in res))
        max_score = - max(max(e["score"] for e in res), max(e["start_score"] for e in res))
        start_scores = [-e["start_score"] for e in res]
        scores = [-e["score"] for e in res]
        rscript_name = Path(working_dir, "garli.starting-final-scores.r")
        with rscript_name.open("w") as f:
            f.write('doplot <- function() {\n')
            f.write('    plot(c(1, {num_results}), c({min_score}, {max_score}), type="n", main="Score improvement by GARLI", xlab="GARLI run number", ylab="GARLI score")\n'.format(num_results=len(res), min_score=min_score, max_score=max_score))
            f.write('    legend("bottomleft", c("starting scores", "final scores"), pch=c("-", "-"), col=c("magenta", "blue"), bty="n", lwd=5)\n')
            f.write('    lines(1:{num_results}, c({start_scores}), col="magenta")\n'.format(num_results=len(res), start_scores=",".join(str(s) for s in start_scores)))
            f.write('    lines(1:{num_results}, c({scores}), col="blue")\n'.format(num_results=len(res), scores=",".join(str(s) for s in scores)))
            f.write('}\n\n')
            f.write('pdf("{fn}", 14, 7)\n'.format(fn=Path(working_dir, "garli.starting-final-scores.pdf")))
            f.write('doplot()\n')
            f.write('dev.off()\n\n')
            f.write('png("{fn}", 1600, 800)\n'.format(fn=Path(working_dir, "garli.starting-final-scores.png")))
            f.write('doplot()\n')
            f.write('dev.off()\n\n')
        subprocess.run(["Rscript", str(rscript_name)], stdout=subprocess.DEVNULL)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
