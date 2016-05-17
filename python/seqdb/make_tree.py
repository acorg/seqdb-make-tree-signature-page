# -*- Python -*-
# license
# license.

# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
from . import json
from .raxml import Raxml, RaxmlResult
from .garli import Garli, GarliResults

# ======================================================================

def run_raxml_best_garli(working_dir, run_id, fasta_file, base_seq_name, raxml_bfgs, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, email, machines):
    """Run RAxML raxml_num_runs times, take the best result and run GARLI garli_num_runs times starting with that best result."""
    run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id
    r_raxml = run_raxml(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_num_runs=raxml_num_runs, email=email, machines=machines)

    # outgroups = [base_seq_name]
    # raxml_output_dir = Path(working_dir, "raxml")
    # raxml = Raxml(email=email)
    # raxml_job = raxml.submit_htcondor(num_runs=raxml_num_runs, source=fasta_file, output_dir=raxml_output_dir,
    #                                   run_id=run_id, bfgs=raxml_bfgs, outgroups=outgroups, machines=machines)
    # r_raxml = raxml_job.wait()
    # module_logger.info('RAxML {}'.format(r_raxml.report_best()))
    # with Path(working_dir, "result.raxml.txt").open("w") as f:
    #     f.write("Longest time: " + r_raxml.longest_time_str()+ "\n\n")
    #     f.write(r_raxml.tabbed_report_header()+ "\n")
    #     f.write("\n".join(rr.tabbed_report() for rr in r_raxml.results) + "\n")

    # --------------------------------------------------

    r_garli = run_garli(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, tree=r_raxml.best_tree(), garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, email=email, machines=machines)

    # garli_output_dir = Path(working_dir, "garli")
    # garli = Garli(email=email)
    # garli_job = garli.submit_htcondor(num_runs=garli_num_runs, source=fasta_file, source_tree=r_raxml.best_tree(), output_dir=garli_output_dir,
    #                                   run_id=run_id, attachmentspertaxon=garli_attachmentspertaxon, machines=machines)
    # r_garli = garli_job.wait()
    # module_logger.info('GARLI {}'.format(r_garli.report_best()))
    # with Path(working_dir, "result.garli.txt").open("w") as f:
    #     f.write("Longest time: " + r_garli.longest_time_str()+ "\n\n")
    #     f.write(r_garli.tabbed_report_header()+ "\n")
    #     f.write("\n".join(rr.tabbed_report() for rr in r_garli.results) + "\n")

    # --------------------------------------------------

    return make_results(working_dir=working_dir, r_raxml=r_raxml, r_garli=r_garli)

    # longest_time = r_raxml.longest_time + r_garli.longest_time
    # longest_time_s = RaxmlResult.time_str(longest_time)
    # module_logger.info('Longest time: ' + longest_time_s)

    # r_best = vars(r_garli.results[0])
    # r_best["longest_time"] = longest_time
    # with Path(working_dir, "result.best.json").open("w") as f:
    #     f.write(json.dumps(r_best) + "\n")

    # r = {
    #     " total": {
    #         "longest_time": longest_time,
    #         "longest_time_s": longest_time_s,
    #         "tree": str(r_garli.results[0].tree),
    #         "garli_score": r_garli.results[0].score,
    #         },
    #     "garli": [vars(r) for r in r_garli.results],
    #     "raxml": [vars(r) for r in r_raxml.results],
    #     }
    # with Path(working_dir, "result.all.json").open("w") as f:
    #     f.write(json.dumps(r) + "\n")

    # return r

# ----------------------------------------------------------------------

def run_raxml_all_garli(working_dir, run_id, fasta_file, base_seq_name, raxml_bfgs, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, email, machines):
    """Run RAxML raxml_num_runs times, take all the results and run GARLI garli_num_runs times starting with each result."""
    run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id
    r_raxml = run_raxml(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, base_seq_name=base_seq_name, raxml_bfgs=raxml_bfgs, raxml_num_runs=raxml_num_runs, email=email, machines=machines)
    r_garli = run_garli_multi(working_dir=working_dir, run_id=run_id, fasta_file=fasta_file, trees=[r.tree for r in r_raxml.results], garli_num_runs=garli_num_runs, garli_attachmentspertaxon=garli_attachmentspertaxon, email=email, machines=machines)
    return make_results(working_dir=working_dir, r_raxml=r_raxml, r_garli=r_garli)

# ----------------------------------------------------------------------

def run_raxml(working_dir, run_id, fasta_file, base_seq_name, raxml_bfgs, raxml_num_runs, email, machines):
    raxml_output_dir = Path(working_dir, "raxml")
    raxml = Raxml(email=email)
    raxml_job = raxml.submit_htcondor(num_runs=raxml_num_runs, source=fasta_file, output_dir=raxml_output_dir,
                                      run_id=run_id, bfgs=raxml_bfgs, outgroups=[base_seq_name], machines=machines)
    r_raxml = raxml_job.wait()
    module_logger.info('RAxML {}'.format(r_raxml.report_best()))
    r_raxml.make_txt(Path(working_dir, "result.raxml.txt"))
    r_raxml.make_json(Path(working_dir, "result.raxml.json"))
    return r_raxml

# ----------------------------------------------------------------------

def run_garli(working_dir, run_id, fasta_file, tree, garli_num_runs, garli_attachmentspertaxon, email, machines):
    garli_output_dir = Path(working_dir, "garli")
    garli = Garli(email=email)
    garli_job = garli.submit_htcondor(num_runs=garli_num_runs, source=fasta_file, source_tree=tree, output_dir=garli_output_dir,
                                      run_id=run_id, attachmentspertaxon=garli_attachmentspertaxon, machines=machines)
    r_garli = garli_job.wait()
    module_logger.info('GARLI {}'.format(r_garli.report_best()))
    r_garli.make_txt(Path(working_dir, "result.garli.txt"))
    r_garli.make_json(Path(working_dir, "result.garli.json"))
    return r_garli

# ----------------------------------------------------------------------

def run_garli_multi(working_dir, run_id, fasta_file, trees, garli_num_runs, garli_attachmentspertaxon, email, machines):
    jobs = []
    for no, tree in enumerate(trees, start=1):
        no_s = "{:03d}".format(no)
        garli_output_dir = Path(working_dir, "garli", no_s)
        jobs.append(
            Garli(email=email).submit_htcondor(
                num_runs=garli_num_runs, source=fasta_file, source_tree=tree, output_dir=garli_output_dir,
                run_id=run_id + "." + no_s, attachmentspertaxon=garli_attachmentspertaxon, machines=machines))
    r_garli = GarliResults(None)
    for job in jobs:
        r_job = job.wait()
        r_garli.results.extend(r_job.results)
    r_garli.recompute()
    module_logger.info('GARLI (multi {}) {}'.format(len(jobs), r_garli.report_best()))
    r_garli.make_txt(Path(working_dir, "result.garli.txt"))
    r_garli.make_json(Path(working_dir, "result.garli.json"))
    return r_garli

# ----------------------------------------------------------------------

def make_results(working_dir, r_raxml, r_garli):
    longest_time = r_raxml.longest_time + r_garli.longest_time
    longest_time_s = RaxmlResult.time_str(longest_time)
    module_logger.info('Longest time: ' + longest_time_s)

    r_best = vars(r_garli.results[0])
    r_best["longest_time"] = longest_time
    r_best["longest_time_s"] = longest_time_s
    json.dumpf(Path(working_dir, "result.best.json"), r_best)

    with Path(working_dir, "result.all.txt").open("w") as f:
        f.write("Overall time: " + longest_time_s + "\n")
        f.write("GARLI score : " + str(r_best["score"]) + "\n")
        f.write("Tree        : " + str(r_best["tree"]) + "\n")

    r = {
        " total": {
            "longest_time": longest_time,
            "longest_time_s": longest_time_s,
            "tree": str(r_garli.results[0].tree),
            "garli_score": r_garli.results[0].score,
            },
        "garli": [vars(r) for r in r_garli.results],
        "raxml": [vars(r) for r in r_raxml.results],
        }
    json.dumpf(Path(working_dir, "result.all.json"), r)

    return r

# ----------------------------------------------------------------------

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
