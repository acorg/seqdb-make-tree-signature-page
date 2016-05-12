#include "tree-import.hh"

#include "read-file.hh"
#include "newick.hh"
#include "xz.hh"

// ----------------------------------------------------------------------

Tree import_tree(std::string buffer)
{
    Tree tree;
    if (buffer == "-")
        buffer = read_stdin();
    else // if (file_exists(buffer))
        buffer = read_file(buffer);
    if (xz_compressed(buffer))
        buffer = xz_decompress(buffer);
    if (buffer[0] == '(')
        tree = parse_newick(buffer);
    else if (buffer[0] == '{')
        tree = Tree::from_json(buffer);
    else
        throw std::runtime_error("cannot import tree: unrecognized source format");
    return tree;
}

// ----------------------------------------------------------------------
