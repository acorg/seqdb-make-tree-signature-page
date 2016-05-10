# -*- Python -*-
# license
# license.

# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
import json
from pathlib import Path
from .raxml import Raxml, RaxmlResult
from .garli import Garli

# ======================================================================

def run_raxml_garli(working_dir, run_id, fasta_file, base_seq_name, raxml_bfgs, raxml_num_runs, garli_num_runs, garli_attachmentspertaxon, email, machines):
    run_id = run_id.replace(' ', '-').replace('/', '-') # raxml cannot handle spaces and slashes in run-id

    outgroups = [base_seq_name]
    raxml_output_dir = Path(working_dir, "raxml")
    raxml = Raxml(email=email)
    raxml_job = raxml.submit_htcondor(num_runs=raxml_num_runs, source=fasta_file, output_dir=raxml_output_dir,
                                      run_id=run_id, bfgs=raxml_bfgs, outgroups=outgroups, machines=machines)
    r_raxml = raxml_job.wait()
    module_logger.info('RAxML {}'.format(r_raxml.report_best()))
    with Path(working_dir, "result.raxml.txt").open("w") as f:
        f.write("Longest time: " + r_raxml.longest_time_str()+ "\n\n")
        f.write(r_raxml.tabbed_report_header()+ "\n")
        f.write("\n".join(rr.tabbed_report() for rr in r_raxml.results) + "\n")

    # --------------------------------------------------

    garli_output_dir = Path(working_dir, "garli")
    garli = Garli(email=email)
    garli_job = garli.submit_htcondor(num_runs=garli_num_runs, source=fasta_file, source_tree=r_raxml.best_tree(), output_dir=garli_output_dir,
                                      run_id=run_id, attachmentspertaxon=garli_attachmentspertaxon, machines=machines)
    r_garli = garli_job.wait()
    module_logger.info('GARLI {}'.format(r_garli.report_best()))
    with Path(working_dir, "result.garli.txt").open("w") as f:
        f.write("Longest time: " + r_garli.longest_time_str()+ "\n\n")
        f.write(r_garli.tabbed_report_header()+ "\n")
        f.write("\n".join(rr.tabbed_report() for rr in r_garli.results) + "\n")

    # --------------------------------------------------

    longest_time = r_raxml.longest_time + r_garli.longest_time
    longest_time_s = RaxmlResult.time_str(longest_time)
    module_logger.info('Longest time: ' + longest_time_s)

    r_best = vars(r_garli.results[0])
    r_best["longest_time"] = longest_time
    with Path(working_dir, "result.best.json").open("w") as f:
        f.write(json.dumps(r_best, indent=2, sort_keys=True, cls=JSONEncoder) + "\n")

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
    with Path(working_dir, "result.all.json").open("w") as f:
        f.write(json.dumps(r, indent=2, sort_keys=True, cls=JSONEncoder) + "\n")

    return r

# ----------------------------------------------------------------------

class JSONEncoder (json.JSONEncoder):

    def default(self, o):
        return "<" + repr(o) + ">"

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
