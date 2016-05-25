#include <iostream>
#include <map>
#include <regex>

#include "string.hh"
#include "amino-acids.hh"

// ----------------------------------------------------------------------

// Some sequences from CNIC (and perhaps from other labs) have initial
// part of nucleotides with stop codons inside. To figure out correct
// translation we have first to translate with all possible offsets
// (0, 1, 2) and not stoppoing at stop codons, then try to align all
// of them. Most probably just one offset leads to finding correct
// align shift.
AlignAminoAcidsData translate_and_align(std::string aNucleotides, Messages& aMessages)
{
    std::vector<AlignAminoAcidsData> r;
    AlignAminoAcidsData not_aligned;
    for (int offset = 0; offset < 3; ++offset) {
        auto amino_acids = translate_nucleotides_to_amino_acids(aNucleotides, static_cast<size_t>(offset), aMessages);
        auto aa_parts = string::split(amino_acids, "*");
        size_t prefix_len = 0;
        for (const auto& part: aa_parts) {
            if (part.size() >= MINIMUM_SEQUENCE_AA_LENGTH) {
                auto align_data = align(part, aMessages);
                if (!align_data.shift.alignment_failed()) {
                    if (align_data.shift.aligned() && prefix_len > 0) {
                        align_data.shift -= prefix_len;
                    }
                    r.push_back(AlignAminoAcidsData(align_data, amino_acids, offset));
                    break;
                }
                else {
                    if (not_aligned.amino_acids.size() < part.size()) {
                        not_aligned.amino_acids = part;
                        not_aligned.offset = offset;
                    }
                }
            }
            prefix_len += 1 + part.size();
        }
    }
    if (r.empty()) {
        return not_aligned;
    }
    if (r.size() > 1)
        aMessages.warning() << "Multiple translations and alignment for: " << aNucleotides << std::endl;
    return r[0];

} // translate_and_align

// ----------------------------------------------------------------------

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

// ----------------------------------------------------------------------

static const std::map<std::string, char> CODON_TO_PROTEIN = {
    {"UGC", 'C'}, {"GTA", 'V'}, {"GTG", 'V'}, {"CCT", 'P'}, {"CUG", 'L'}, {"AGG", 'R'}, {"CTT", 'L'}, {"CUU", 'L'},
    {"CTG", 'L'}, {"GCU", 'A'}, {"CCG", 'P'}, {"AUG", 'M'}, {"GGC", 'G'}, {"UUA", 'L'}, {"GAG", 'E'}, {"UGG", 'W'},
    {"UUU", 'F'}, {"UUG", 'L'}, {"ACU", 'T'}, {"TTA", 'L'}, {"AAT", 'N'}, {"CGU", 'R'}, {"CCA", 'P'}, {"GCC", 'A'},
    {"GCG", 'A'}, {"TTG", 'L'}, {"CAT", 'H'}, {"AAC", 'N'}, {"GCA", 'A'}, {"GAU", 'D'}, {"UAU", 'Y'}, {"CAC", 'H'},
    {"AUA", 'I'}, {"GUC", 'V'}, {"TCG", 'S'}, {"GGG", 'G'}, {"AGC", 'S'}, {"CTA", 'L'}, {"GCT", 'A'}, {"CCC", 'P'},
    {"ACC", 'T'}, {"GAT", 'D'}, {"TCC", 'S'}, {"UAC", 'Y'}, {"CAU", 'H'}, {"UCG", 'S'}, {"CAA", 'Q'}, {"UCC", 'S'},
    {"AGU", 'S'}, {"TTT", 'F'}, {"ACA", 'T'}, {"ACG", 'T'}, {"CGC", 'R'}, {"TGT", 'C'}, {"CAG", 'Q'}, {"GUA", 'V'},
    {"GGU", 'G'}, {"AAG", 'K'}, {"AGA", 'R'}, {"ATA", 'I'}, {"TAT", 'Y'}, {"UCU", 'S'}, {"TCA", 'S'}, {"GAA", 'E'},
    {"AGT", 'S'}, {"TCT", 'S'}, {"ACT", 'T'}, {"CGA", 'R'}, {"GGT", 'G'}, {"TGC", 'C'}, {"UGU", 'C'}, {"CUC", 'L'},
    {"GAC", 'D'}, {"UUC", 'F'}, {"GTC", 'V'}, {"ATT", 'I'}, {"TAC", 'Y'}, {"CUA", 'L'}, {"TTC", 'F'}, {"GTT", 'V'},
    {"UCA", 'S'}, {"AUC", 'I'}, {"GGA", 'G'}, {"GUG", 'V'}, {"GUU", 'V'}, {"AUU", 'I'}, {"CGT", 'R'}, {"CCU", 'P'},
    {"ATG", 'M'}, {"AAA", 'K'}, {"TGG", 'W'}, {"CGG", 'R'}, {"AAU", 'N'}, {"CTC", 'L'}, {"ATC", 'I'},
    {"TAA", '*'}, {"UAA", '*'}, {"TAG", '*'}, {"UAG", '*'}, {"TGA", '*'}, {"UGA", '*'}, {"TAR", '*'}, {"TRA", '*'}, {"UAR", '*'}, {"URA", '*'},
};

std::string translate_nucleotides_to_amino_acids(std::string aNucleotides, size_t aOffset, Messages& /*aMessages*/)
{
    typedef decltype(CODON_TO_PROTEIN)::difference_type Diff;
    std::string result;
    result.resize((aNucleotides.size() - aOffset) / 3 + 1, '-');
    auto result_p = result.begin();
    for (auto offset = aOffset; offset < aNucleotides.size(); offset += 3, ++result_p) {
        auto const it = CODON_TO_PROTEIN.find(std::string(aNucleotides.begin() + static_cast<Diff>(offset), aNucleotides.begin() + static_cast<Diff>(offset) + 3));
        if (it != CODON_TO_PROTEIN.end())
            *result_p = it->second;
        else
            *result_p = 'X';
    }
    result.resize(static_cast<size_t>(result_p - result.begin()));
    return result;

} // translate_nucleotides_to_amino_acids

// ----------------------------------------------------------------------

struct AlignEntry : public AlignData
{
    inline AlignEntry() = default;
    inline AlignEntry(const AlignEntry&) = default;
    inline AlignEntry(std::string aSubtype, std::string aLineage, std::string aGene, Shift aShift, const std::regex& aRe, size_t aEndpos, bool aSignalpeptide, std::string aName)
        : AlignData(aSubtype, aLineage, aGene, aShift), re(aRe), endpos(aEndpos), signalpeptide(aSignalpeptide), name(aName) {}

    std::regex re;
    size_t endpos;
    bool signalpeptide;
    std::string name;           // for debugging
};

    // std::string subtype;
    // std::string lineage;
    // std::string gene;
    // Shift shift;

static AlignEntry ALIGN_RAW_DATA[] = {
    {"A(H3N2)", "", "HA", Shift(),   std::regex("MKTIIAL[CS][HY]I[FLS]C[LQ][AV][FL][AG]"),  20,  true, "h3-MKT-1"},
    {"A(H3N2)", "", "HA", Shift(),   std::regex("MKTIIVLSCFFCLAFS"),                        20,  true, "h3-MKT-12"},
    {"A(H3N2)", "", "HA", Shift(),   std::regex("MKTLIALSYIFCLVLG"),                        20,  true, "h3-MKT-13"},
    {"A(H3N2)", "", "HA", Shift(),   std::regex("MKTTTILILLTHWVHS"),                        20,  true, "h3-MKT-14"},
    {"A(H3N2)", "", "HA", 10,        std::regex("ATLCLGHHAV"),                             100, false, "h3-ATL"},
    {"A(H3N2)", "", "HA", 36,        std::regex("TNATELVQ"),                               100, false, "h3-TNA"},
    {"A(H3N2)", "", "HA", 87,        std::regex("VERSKAYSN"),                              100, false, "h3-VER"},

    {"A(H3N2)", "", "NA",  0,        std::regex("MNP[NS]QKI[IM]TIGS[IVX]SL[IT][ILV]"),      20, false, "h3-NA-1"}, // Kim, http://www.ncbi.nlm.nih.gov/nuccore/DQ415347.1


    {"A(H1N1)", "",         "HA", Shift(), std::regex("MKVK[LY]LVLLCTFTATYA"),                           20,  true, "h1-MKV-1"},
    {"A(H1N1)", "SEASONAL", "HA", Shift(), std::regex("MKVKLLVLLCTFSATYA"),                              20,  true, "h1-MKV-2"},
    {"A(H1N1)", "2009PDM",  "HA", Shift(), std::regex("M[EK]AIL[VX][VX][LM]L[CHY]T[FL][AT]T[AT][NS]A"),  20,  true, "h1-MKA-2"},
    {"A(H1N1)", "",         "HA",       0, std::regex("DTLCI[GX][YX]HA"),                               100, false, "h1-DTL-1"},
    {"A(H1N1)", "",         "HA",       0, std::regex("DT[IL]C[IM]G[HXY]H[AX]NN"),                      100, false, "h1-DTL-2"},
    {"A(H1N1)", "",         "HA",       5, std::regex("GYHANNS[AT]DTV"),                                100, false, "h1-GYH"},
    {"A(H1N1)", "",         "HA",      96, std::regex("[DN]YEELREQL"),                                  120, false, "h1-DYE"},
    {"A(H1N1)", "",         "HA",     162, std::regex("[KQ]SY[AI]N[ND]K[EG]KEVLVLWG[IV]HHP"),           220, false, "h1-KSY"},
    {"A(H1N1)", "",         "HA",     105, std::regex("SSISSFER"),                                      200, false, "h1-SSI"},

    {"A(H1N1)", "",         "NA",       0, std::regex("MNPNQKIITIGSVCMTI"),                              20, false, "h1-NA-1"},
    {"A(H1N1)", "",         "NA", Shift(), std::regex("FAAGQSVVSVKLAGNSSLCPVSGWAIYSK"),                 200, false, "h1-NA-2"},
    {"A(H1N1)", "",         "NA", Shift(), std::regex("QASYKIFRIEKGKI"),                                300, false, "h1-NA-3"},

    {"A(H1N1)", "",         "M1", Shift(), std::regex("MSLLTEVETYVLSIIPSGPLKAEIAQRLESVFAGKNTDLEAL"),    100, false, "h1-M1-1"},
    {"A(H1N1)", "",         "M1", Shift(), std::regex("MGLIYNRMGTVTTEAAFGLVCA"),                        200, false, "h1-M1-2"},
    {"A(H1N1)", "",         "M1", Shift(), std::regex("QRLESVFAGKNTDLEALMEWL"),                         200, false, "h1-M1-3"},

    {"A(H5N1)", "", "HA", Shift(),   std::regex("MEKIVLL[FL]AI[IV]SLVKS"),  20,  true, "h5-MEK-1"}, // http://signalpeptide.com
    {"A(H5)",   "", "HA", Shift(),   std::regex("MEKIVLLLAVVSLVRS"),        20,  true, "h5-MEK-2"}, // http://signalpeptide.com H5N6, H5N2
    {"A(H5)",   "", "HA",       0,   std::regex("DQICIGYHANNSTEQV"),        40, false, "h5-DQI-1"},

    {"B", "", "HA", Shift(), std::regex("M[EKT][AGT][AIL][ICX]V[IL]L[IMT][AEILVX][AIVX][AMT]S[DHKNSTX][APX]"), 30,  true, "B-MKT"}, // http://repository.kulib.kyoto-u.ac.jp/dspace/bitstream/2433/49327/1/8_1.pdf, inferred by Eu for B/INDONESIA/NIHRD-JBI152/2015, B/CAMEROON/14V-8639/2014
    {"B", "", "HA",       0, std::regex("DR[ISV]C[AST][GX][ITV][IT][SWX]S[DKNX]SP[HXY][ILTVX][VX][KX]T[APT]T[QX][GV][EK][IV]NVTG[AV][IX][LPS]LT[AITX][AIST][LP][AIT][KRX]"), 50, false, "B-DRICT"},
    {"B", "", "HA",       3, std::regex("CTG[IVX]TS[AS]NSPHVVKTATQGEVNVTGVIPLTTTP"),                           50, false, "B-CTG"},
    {"B", "", "HA",      23, std::regex("[XV]NVTGVIPLTTTPTK"),                                                 50, false, "B-VNV"},
    {"B", "", "HA",      59, std::regex("CTDLDVALGRP"),                                                       150, false, "B-CTD"},
    {"B", "", "HA", Shift(), std::regex("MVVTSNA"),                                                            20,  true, "B-MVV"},

    {"B", "", "NA",  Shift(), std::regex("MLPSTIQ[MT]LTL[FY][IL]TSGGVLLSLY[AV]S[AV][LS]LSYLLY[SX]DIL[LX][KR]F"), 45, false, "B-NA"},
    {"B", "", "NS1", Shift(), std::regex("MA[DN]NMTT[AT]QIEVGPGATNAT[IM]NFEAGILECYERLSWQ[KR]AL"),                45, false, "B-NS1-1"},
    {"B", "", "NS1", Shift(), std::regex("MA[NX][DN][NX]MTTTQIEVGPGATNATINFEAGILECYERLSWQR"),                    45, false, "B-NS1-2"}, // has insertion at 2 or 3 compared to the above
    {"B", "", "",    Shift(), std::regex("GNFLWLLHV"),                                                           45, false, "B-CNIC"}, // Only CNIC sequences 2008-2009 have it, perhaps not HA
};

AlignData align(std::string aAminoAcids, Messages& /*aMessages*/)
{
    for (auto raw_data = std::begin(ALIGN_RAW_DATA); raw_data != std::end(ALIGN_RAW_DATA); ++raw_data) {
        std::smatch m;
        if (std::regex_search(aAminoAcids.cbegin(), aAminoAcids.cbegin() + static_cast<std::string::difference_type>(std::min(aAminoAcids.size(), raw_data->endpos)), m, raw_data->re)) {
            AlignData r(*raw_data);
            if (raw_data->signalpeptide) {
                r.shift = - (m[0].second - aAminoAcids.cbegin());
            }
            else if (r.shift.aligned()) {
                r.shift -= m[0].first - aAminoAcids.cbegin();
            }
            return r;
        }
    }
    std::cerr << "Not aligned: " << aAminoAcids << std::endl;
    return AlignData();

} // align

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
