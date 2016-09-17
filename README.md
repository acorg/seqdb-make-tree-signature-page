# seqdb
Set of tools to build, query sequence database and to generate phylogenetic trees.

## Requirements:

- macOS: clang 7.3
- Linux: gcc 5.3 or 6.1 (gcc 4.9 is not supported by json-struct module)

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

- TODO

    + [http://www.libharu.org/](http://www.libharu.org/) (2015-06-28) (homebrew)
      HPDF\_Page\_CreateTextAnnot
    + [http://podofo.sourceforge.net](http://podofo.sourceforge.net) (0.9.4 - 2016-06-08, 0.9.3 - 2014)
      (homebrew and apt-get)

    + [http://www.jagpdf.org](http://www.jagpdf.org) (Oct 2009)
