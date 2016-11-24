#include "tree-import.hh"

#include "acmacs-base/read-file.hh"
#include "newick.hh"

// ----------------------------------------------------------------------

Tree* import_tree(std::string buffer)
{
    Tree* tree = nullptr;
    if (buffer == "-")
        buffer = acmacs_base::read_stdin();
    else // if (file_exists(buffer))
        buffer = acmacs_base::read_file(buffer);
    if (buffer[0] == '(') {
        tree = parse_newick(buffer);
        tree->preprocess_upon_importing_from_external_format();
    }
    else if (buffer[0] == '{') {
        tree = Tree::from_json(buffer);
    }
    else
        throw std::runtime_error("cannot import tree: unrecognized source format");
    return tree;
}

// ----------------------------------------------------------------------
