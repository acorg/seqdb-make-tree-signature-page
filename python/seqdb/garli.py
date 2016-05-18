# -*- Python -*-
# license
# license.

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import re, random, subprocess, time as time_m, operator
from . import raxml, json

# ----------------------------------------------------------------------

class GarliError (Exception):
    pass

# ----------------------------------------------------------------------

class GarliResult:

    def __init__(self, tree, score, start_score, time):
        self.tree = tree
        self.score = score
        self.start_score = start_score
        self.time = time

    def __repr__(self):
        return str(vars(self))

    def tabbed_report(self):
        return "{:10.4f} {:>8s} {:10.4f} {}".format(self.score, raxml.RaxmlResult.time_str(self.time), self.start_score, str(self.tree))

    def json(self):
        return vars(self)

# ----------------------------------------------------------------------

class GarliResults:

    def __init__(self, results):
        self.results = sorted(results, key=operator.attrgetter("score")) if results else []
        self.longest_time = max(self.results, key=operator.attrgetter("time")).time if results else 0

    def recompute(self):
        self.results.sort(key=operator.attrgetter("score"))
        self.longest_time = max(self.results, key=operator.attrgetter("time")).time

    def longest_time_str(self):
        return raxml.RaxmlResult.time_str(self.longest_time)

    def best_tree(self):
        return self.results[0].tree

    def report_best(self):
        return "{} {} {}".format(self.results[0].score, self.longest_time_str(), self.best_tree())

    def make_txt(self, filepath :Path):
        with filepath.open("w") as f:
            f.write("Longest time: " + self.longest_time_str()+ "\n\n")
            f.write(self.tabbed_report_header()+ "\n")
            f.write("\n".join(rr.tabbed_report() for rr in self.results) + "\n")

    def make_json(self, filepath :Path):
        json.dumpf(filepath, {"longest_time": self.longest_time, "results": self.results})

    @classmethod
    def tabbed_report_header(cls):
        return "{:^10s} {:^8s} {:^10s} {}".format("score", "time", "startscore", "tree")

    @classmethod
    def import_from(cls, source_dir):
        if Path(source_dir, "001").is_dir():
            r = GarliResults(None)
            for subdir in source_dir.glob("*"):
                if subdir.is_dir():
                    r.results.extend(Garli.get_result(output_dir=subdir, run_id=".".join(tree.parts[-1].split(".")[:-2])) for tree in subdir.glob("*.best.phy"))
            r.recompute()
        else:
            r = GarliResults(Garli.get_result(output_dir=source_dir, run_id=".".join(tree.parts[-1].split(".")[:-2])) for tree in source_dir.glob("*.best.phy"))
        return r

# ----------------------------------------------------------------------

class GarliTask:

    def __init__(self, job, output_dir, run_ids, start):
        self.job = job
        self.output_dir = output_dir
        self.run_ids = run_ids
        self.start = start

    def wait(self):
        status  = self.job.wait()
        if status == "FAILED":
            raise GarliError("HTCondor job failed (aborted by a user?)")
        return GarliResults(Garli.get_result(output_dir=self.output_dir, run_id=ri) for ri in self.run_ids)

# ----------------------------------------------------------------------

class Garli:

    def __init__(self, email):
        self.email = email
        self._find_program()
        self.random_gen = random.SystemRandom()
        # self.model = "GTRGAMMAI"
        # self.default_args = ["-c", "4", "-f", "d", "--silent", "--no-seq-check"]

    def submit_htcondor(self, num_runs, source, output_dir, run_id, source_tree=None, outgroup=[1], attachmentspertaxon=1000000, genthreshfortopoterm=20000, stoptime=3600*24*7, searchreps=1, strip_comments=True, machines=None):
        from . import htcondor
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        run_ids = ["{}.{:04d}".format(run_id, run_no) for run_no in range(num_runs)]
        conf_files = [self._make_conf(run_id, source=source, source_tree=source_tree, outgroup=outgroup, output_dir=output_dir, attachmentspertaxon=attachmentspertaxon, randseed=self._random_seed(), genthreshfortopoterm=genthreshfortopoterm, searchreps=searchreps, stoptime=stoptime, strip_comments=strip_comments) for run_id in run_ids]
        module_logger.info('{} garli conf files saved to {}'.format(len(conf_files), output_dir))
        args=[[str(Path(c).resolve())] for c in conf_files]
        start = time_m.time()
        job = htcondor.submit(program=self.program,
                              program_args=args,
                              description="GARLI {run_id} {num_runs}".format(run_id=run_id, num_runs=num_runs),
                              current_dir=output_dir,
                              capture_stdout=False, email=self.email, notification="Error", machines=machines)
        module_logger.info('Jobs submitted to htcondor: {}'.format(job))
        return GarliTask(job=job, output_dir=output_dir, run_ids=run_ids, start=start)

    # ----------------------------------------------------------------------

    # sReGarliScore = re.compile(r"\[!GarliScore\s+-(\d+\.\d+)\]")
    # sReGarliTime = re.compile(r"Time used = (\d+) hours, (\d+) minutes and (\d+) seconds")

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
        return GarliResult(tree=str(tree), score=score, start_score=start_score, time=execution_time)

    # ----------------------------------------------------------------------

    def _make_conf(self, run_id, source, source_tree, output_dir, outgroup, attachmentspertaxon, randseed, genthreshfortopoterm, searchreps, stoptime, strip_comments):
        """Returns filename of the written conf file"""
        if not outgroup or not isinstance(outgroup, list) or not all(isinstance(e, int) and e > 0 for e in outgroup):
            raise ValueError("outgroup must be non-empty list of taxa indices in the fasta file starting with 1")
        global GARLI_CONF
        garli_args = {
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
        conf = GARLI_CONF.format(**garli_args)
        if strip_comments:
            conf = "\n".join(line for line in conf.splitlines() if line.strip() and line.strip()[0] != "#")
        conf_filename = Path(output_dir, run_id + ".garli.conf")
        with conf_filename.open("w") as f:
            f.write(conf)
        return conf_filename

    # ----------------------------------------------------------------------

    def _find_program(self):
        import socket
        hostname = socket.getfqdn()
        # module_logger.debug('hostname {}'.format(hostname))
        if hostname == "jagd":
            self.program = "/Users/eu/ac/bin/garli"
        elif hostname == "albertine.antigenic-cartography.org":
            self.program = "/syn/bin/garli"
        else:
            self.program = "Garli"
        output = subprocess.check_output(self.program + " -v", shell=True, stderr=subprocess.STDOUT).decode("utf-8")
        m = re.search(r"GARLI\s+Version\s+([\d\.]+)", output)
        if m:
            module_logger.info('GARLI {}'.format(m.group(1)))
        else:
            raise ValueError("Unrecognized GARLI version\n" + output)

    def _random_seed(self):
        return self.random_gen.randint(1, 0x7FFFFFFF) # garli accepts 31-bit random number seed

# ----------------------------------------------------------------------

def postprocess(target_dir, source_dir):
    results = GarliResults.import_from(source_dir)
    module_logger.info('GARLI {}'.format(results.report_best()))
    results.make_txt(Path(target_dir, "result.garli.txt"))
    results.make_json(Path(target_dir, "result.garli.json"))
    return results

# ----------------------------------------------------------------------

GARLI_CONF = """\
[general]
datafname = {source}
streefname = {streefname}
# Prefix of all output filenames, such as log, treelog, etc. Change
# this for each run that you do or the program will overwrite previous
# results.
ofprefix = {output_prefix}

constraintfile = none

# (1 to infinity) The number of attachment branches evaluated for each
# taxon to be added to the tree during the creation of an ML
# stepwise-addition starting tree. Briefly, stepwise addition is an
# algorithm used to make a tree, and involves adding taxa in a random
# order to a growing tree. For each taxon to be added, a number of
# randomly chosen attachment branches are tried and scored, and then
# the best scoring one is chosen as the location of that taxon. The
# attachmentspertaxon setting controls how many attachment points are
# evaluated for each taxon to be added. A value of one is equivalent
# to a completely random tree (only one randomly chosen location is
# evaluated). A value of greater than 2 times the number of taxa in
# the dataset means that all attachment points will be evaluated for
# each taxon, and will result in very good starting trees (but may
# take a while on large datasets). Even fairly small values (< 10) can
# result in starting trees that are much, much better than random, but
# still fairly different from one another. Default: 50.
attachmentspertaxon = {attachmentspertaxon}

# -1 - random seed chosen automatically
randseed = {randseed}

# in Mb
availablememory = 4000

# The frequency with which the best score is written to the log file, default: 10
logevery = 1000

# Whether to write three files to disk containing all information
# about the current state of the population every saveevery
# generations, with each successive checkpoint overwriting the
# previous one. These files can be used to restart a run at the last
# written checkpoint by setting the restart configuration entry. Default: 0
writecheckpoints = 0

# Whether to restart at a previously saved checkpoint. To use this
# option the writecheckpoints option must have been used during a
# previous run. The program will look for checkpoint files that are
# named based on the ofprefix of the previous run. If you intend to
# restart a run, NOTHING should be changed in the config file except
# setting restart to 1. A run that is restarted from checkpoint will
# give exactly the same results it would have if the run had gone to
# completion. Default: 0
restart = 0

# If writecheckpoints or outputcurrentbesttopology are specified, this
# is the frequency (in generations) at which checkpoints or the
# current best tree are written to file. default: 100
saveevery = 1000

# Specifies whether some initial rough optimization is performed on
# the starting branch lengths and alpha parameter. This is always
# recommended. Default: 1.
refinestart = 1

# If true, the current best tree of the current search replicate is
# written to <ofprefix>.best.current.tre every saveevery
# generations. In versions before 0.96 the current best topology was
# always written to file, but that is no longer the case. Seeing the
# current best tree has no real use apart from satisfying your
# curiosity about how a run is going. Default: 0.
outputcurrentbesttopology = 0

# If true, each new topology encountered with a better score than the
# previous best is written to file. In some cases this can result in
# really big files (hundreds of MB) though, especially for random
# starting topologies on large datasets. Note that this file is
# interesting to get an idea of how the topology changed as the
# searches progressed, but the collection of trees should NOT be
# interpreted in any meaningful way. This option is not available
# while bootstrapping. Default: 0.
outputeachbettertopology = 0

# Specifies whether the automatic termination conditions will be
# used. The conditions specified by both of the following two
# parameters must be met. See the following two parameters for their
# definitions. If this is false, the run will continue until it
# reaches the time (stoptime) or generation (stopgen) limit. It is
# highly recommended that this option be used! Default: 1.
enforcetermconditions = 1

# This specifies the first part of the termination condition. When no
# new significantly better scoring topology (see significanttopochange
# below) has been encountered in greater than this number of
# generations, this condition is met. Increasing this parameter may
# improve the lnL scores obtained (especially on large datasets), but
# will also increase runtimes. Default: 20000.
genthreshfortopoterm = {genthreshfortopoterm}

# The second part of the termination condition. When the total
# improvement in score over the last intervallength x intervalstostore
# generations (default is 500 generations, see below) is less than
# this value, this condition is met. This does not usually need to be
# changed. Default: 0.05
scorethreshforterm = 0.05

# The lnL increase required for a new topology to be considered
# significant as far as the termination condition is concerned. It
# probably doesn’t need to be played with, but you might try
# increasing it slightly if your runs reach a stable score and then
# take a very long time to terminate due to very minor changes in
# topology. Default: 0.01
significanttopochange = 0.01

# Whether a phylip formatted tree files will be output in addition to
# the default nexus files for the best tree across all replicates
# (<ofprefix>.best.phy), the best tree for each replicate
# (<ofprefix>.best.all.phy) or in the case of bootstrapping, the best
# tree for each bootstrap replicate (<ofprefix.boot.phy>).
# We use .phy (it's newick tree format), use 1 here!
outputphyliptree = 1

# Whether to output three files of little general interest: the
# “fate”, “problog” and “swaplog” files. The fate file shows the
# parentage, mutation types and scores of every individual in the
# population during the entire search. The problog shows how the
# proportions of the different mutation types changed over the course
# of the run. The swaplog shows the number of unique swaps and the
# number of total swaps on the current best tree over the course of
# the run. Default: 0
outputmostlyuselessfiles = 0

# This option allow for orienting the tree topologies in a consistent
# way when they are written to file. Note that this has NO effect
# whatsoever on the actual inference and the specified outgroup is NOT
# constrained to be present in the inferred trees. If multiple
# outgroup taxa are specified and they do not form a monophyletic
# group, this setting will be ignored. If you specify a single
# outgroup taxon it will always be present, and the tree will always
# be consistently oriented. To specify an outgroup consisting of taxa
# 1, 3 and 5 the format is this: outgroup = 1 3 5
outgroup = {outgroup}

resampleproportion = 1.0
inferinternalstateprobs = 0
outputsitelikelihoods = 0
optimizeinputonly = 0
collapsebranches = 1

# The number of independent search replicates to perform during a
# program execution. You should always either do multiple search
# replicates or multiple program executions with any dataset to get a
# feel for whether you are getting consistent results, which suggests
# that the program is doing a decent job of searching. Note that if
# this is > 1 and you are performing a bootstrap analysis, this is the
# number of search replicates to be done per bootstrap replicate. That
# can increase the chance of finding the best tree per bootstrap
# replicate, but will also increase bootstrap runtimes
# enormously. Default: 2
searchreps = {searchreps}

bootstrapreps = 0

# ------------------- FOR NUCLEOTIDES --------------
datatype=nucleotide

# The number of relative substitution rate parameters (note that the
# number of free parameters is this value minus one). Equivalent to
# the “nst” setting in PAUP* and MrBayes. 1rate assumes that
# substitutions between all pairs of nucleotides occur at the same
# rate, 2rate allows different rates for transitions and
# transversions, and 6rate allows a different rate between each
# nucleotide pair. These rates are estimated unless the fixed option
# is chosen. New in version 0.96, parameters for any submodel of the
# GTR model may now be estimated. The format for specifying this is
# very similar to that used in the “rclass’ setting of PAUP*. Within
# parentheses, six letters are specified, with spaces between
# them. The six letters represent the rates of substitution between
# the six pairs of nucleotides, with the order being A-C, A-G, A-T,
# C-G, C-T and G-T. Letters within the parentheses that are the same
# mean that a single parameter is shared by multiple nucleotide
# pairs. For example, ratematrix = (a b a a b a) would specify the HKY
# 2-rate model (equivalent to ratematrix = 2rate). This entry,
# ratematrix = (a b c c b a) would specify 3 estimated rates of
# subtitution, with one rate shared by A-C and G-T substitutions,
# another rate shared by A-G and C-T substitutions, and the final rate
# shared by A-T and C-G substitutions. Default: 6rate
ratematrix = 6rate

# (equal, empirical, estimate, fixed) Specifies how the equilibrium
# state frequencies (A, C, G and T) are treated. The empirical setting
# fixes the frequencies at their observed proportions, and the other
# options should be self-explanatory. Default: estimate
statefrequencies = estimate

# (none, gamma, gammafixed) – The model of rate heterogeneity
# assumed. “gammafixed” requires that the alpha shape parameter is
# provided, and a setting of “gamma” estimates it. Default: gamma
ratehetmodel = gamma

# (1 to 20) – The number of categories of variable rates (not
# including the invariant site class if it is being used). Must be set
# to 1 if ratehetmodel is set to none. Note that runtimes and memory
# usage scale linearly with this setting. Default: 4
numratecats = 4

# (none, estimate, fixed) Specifies whether a parameter representing
# the proportion of sites that are unable to change (i.e. have a
# substitution rate of zero) will be included. This is typically
# referred to as “invariant sites”, but would better be termed
# “invariable sites”. Default: estimate
invariantsites = estimate

# ----------------------------------------------------------------------

[master]
nindivs = 4
holdover = 1
selectionintensity = .5
holdoverpenalty = 0

# The maximum number of generations to run.  Note that this supersedes
# the automated stopping criterion (see enforcetermconditions above),
# and should therefore be set to a very large value if automatic
# termination is desired.
stopgen = 1000000

# The maximum number of seconds for the run to continue.  Note that
# this supersedes the automated stopping criterion (see
# enforcetermconditions above), and should therefore be set to a very
# large value if automatic termination is desired.
stoptime = {stoptime}

startoptprec = 0.5
minoptprec = 0.01
numberofprecreductions = 20
treerejectionthreshold = 50.0
topoweight = 1.0
modweight = 0.05
brlenweight = 0.2
randnniweight = 0.1
randsprweight = 0.3
limsprweight =  0.6
intervallength = 100
intervalstostore = 5

limsprrange = 6
meanbrlenmuts = 5
gammashapebrlen = 1000
gammashapemodel = 1000
uniqueswapbias = 0.1
distanceswapbias = 1.0
"""

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
