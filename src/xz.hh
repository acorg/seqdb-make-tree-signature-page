#pragma once

#include <string>

// ----------------------------------------------------------------------

bool xz_compressed(std::string input);
std::string xz_compress(std::string input);
std::string xz_decompress(std::string input);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
