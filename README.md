# seqdb
Set of tools to build, query sequence database and to generate phylogenetic trees.

## Processing

- Make database from the source fasta files.

        seqdb-create --db ~/WHO/seqdb.json.xz *.fasta

- Generate phylogenetic tree on albertine

        whocc-make-tree --flu-lineage BYAM --raxml-num-runs 128 --garli-num-runs 128
  Various output files will be under /syn/eu/ac/results/whocc-tree


- Export some sequences into fasta (or phylip)

        seqdb-export -h

- Importing tree from newick format

        tree-to-tree -h

- Drawing tree

        tree-draw -h
