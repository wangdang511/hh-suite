/*
 * HHblits.h
 *
 *  Created on: Apr 1, 2014
 *      Author: meiermark
 */

#ifndef HHBLITS_H_
#define HHBLITS_H_

#include <iostream>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <cassert>
#include <stdexcept>
#include <map>
#include <omp.h>

#include <emmintrin.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::vector;
using std::pair;

#include <sys/time.h>

extern "C" {
#include <ffindex.h>
}

#include "cs.h"
#include "context_library.h"
#include "library_pseudocounts-inl.h"
#include "crf_pseudocounts-inl.h"
#include "abstract_state_matrix.h"

#include "hhdecl.h"
#include "list.h"
#include "hash.h"
#include "util.h"
#include "hhutil.h"

#include "hhhmm.h"
#include "hhhit.h"
#include "hhalignment.h"
#include "hhhalfalignment.h"
#include "hhfullalignment.h"
#include "hhhitlist.h"

#include "hhmatrices.h"
#include "hhfunc.h"

#include "hhdatabase.h"

#include "hhprefilter.h"

const char HHBLITS_REFERENCE[] =
		"Remmert M., Biegert A., Hauser A., and Soding J.\nHHblits: Lightning-fast iterative protein sequence searching by HMM-HMM alignment.\nNat. Methods 9:173-175 (2011)\n";

class HHblits {
public:
	HHblits(Parameters& parameters);
	virtual ~HHblits();

	void Reset();
	static void ProcessAllArguments(int argc, char** argv, Parameters& par);

    //writer for non-mpi version
    void writeHHRFile(char* hhrFile);
    void writeAlisFile(char* basename);
    void writeScoresFile(char* scoresFile);
    void writePairwiseAlisFile(char* pairwieseAlisFile, char outformat);
    void writeAlitabFile(char* alitabFile);
    void writeReducedHHRFile(char* reduceHHRFile);
    void writePsiFile(char* psiFile);
    void writeHMMFile(char* HMMFile);
    void writeA3MFile(char* A3MFile);

    //output writer for mpi version
    std::map<int, Alignment>& getAlis();
    void writeHHRFile(std::stringstream& out);
    void writeScoresFile(std::stringstream& out);
    void writePairwiseAlisFile(char outformat, std::stringstream& out);
    void writeAlitabFile(std::stringstream& out);
    void writeReducedHHRFile(std::stringstream& out);
    void writePsiFile(std::stringstream& out);
    void writeHMMFile(std::stringstream& out);
    void writeA3MFile(std::stringstream& out);

	void run(FILE* query_fh, char* query_path);

private:
	// substitution matrix flavours
	float __attribute__((aligned(16))) P[20][20];
	float __attribute__((aligned(16))) R[20][20];
	float __attribute__((aligned(16))) Sim[20][20];
	float __attribute__((aligned(16))) S[20][20];
	float __attribute__((aligned(16))) pb[21];
	float __attribute__((aligned(16))) qav[21];

	// secondary structure matrices
	float S73[NDSSP][NSSPRED][MAXCF];
	float S33[NSSPRED][MAXCF][NSSPRED][MAXCF];

	Parameters par;

	cs::ContextLibrary<cs::AA>* context_lib = NULL;
	cs::Crf<cs::AA>* crf = NULL;
	cs::Pseudocounts<cs::AA>* pc_hhm_context_engine = NULL;
	cs::Admix* pc_hhm_context_mode = NULL;
	cs::Pseudocounts<cs::AA>* pc_prefilter_context_engine = NULL;
	cs::Admix* pc_prefilter_context_mode = NULL;

	// HHblits variables
    // verbose mode
	int v1 = v;

    // set to true in last iteration
	bool last_round = false;

	char input_format = 0; // Set to 1 if input in HMMER format (has already pseudocounts)

	char config_file[NAMELEN];

	//database filenames
	std::vector<HHblitsDatabase*> dbs;

	// Create query HMM with maximum of par.maxres match states
	HMM* q = NULL;
	// Create query HMM with maximum of par.maxres match states (needed for prefiltering)
	HMM* q_tmp = NULL;

	// output A3M generated by merging A3M alignments for significant hits to the query alignment
	Alignment Qali;
	// output A3M alignment with no sequence filtered out (only active with -all option)
	Alignment Qali_allseqs;

	// Each bin has a template HMM allocated that was read from the database file
	HMM* t[MAXBINS];
	// Each bin has an object of type Hit allocated with a separate dynamic programming matrix (memory!!)
	Hit* hit[MAXBINS];
	// format[bin] = 0 if in HHsearch format => add pcs; format[bin] = 1 if in HMMER format => no pcs
	int* format;

	HitList hitlist; // list of hits with one Hit object for each pairwise comparison done
	HitList reducedHitlist;
	int N_searched;           // Number of HMMs searched
	std::map<int, Alignment> alis;

	static void help(Parameters& par, char all = 0);
	static void ProcessArguments(int argc, char** argv, Parameters& par);
    HHblitsDatabase* getHHblitsDatabase(HHDatabaseEntry& entry, std::vector<HHblitsDatabase*>& dbs);

	void getTemplateA3M(HHblitsDatabase* db, char* entry_name, long& ftellpos, Alignment& tali);
	void getTemplateHMM(HHDatabaseEntry& entry, char use_global_weights, long& ftellpos, int& format, HMM* t);

	void DoViterbiSearch(std::vector<HHDatabaseEntry*>& prefiltered_hits, Hash<Hit>* previous_hits, bool alignByWorker = true);
	void ViterbiSearch(std::vector<HHDatabaseEntry*>& prefiltered_hits, Hash<Hit>* previous_hits, int db_size);
	void RescoreWithViterbiKeepAlignment(int db_size, Hash<Hit>* previous_hits);

	void perform_realign(std::vector<HHDatabaseEntry*>& hits_to_realign, Hash<char>* premerged_hits);

	//redundancy filter
    void wiggleQSC(HitList& hitlist, int n_redundancy, Alignment& Qali,
            char inputformat, float* qsc, size_t nqsc,
            HitList& reducedFinalHitList);
    void wiggleQSC(int n_redundancy, float* qsc, size_t nqsc, HitList& reducedFinalHitList);
	void reduceRedundancyOfHitList(int n_redundancy, int query_length,
			HitList& hitlist, HitList& reducedHitList);
	void recalculateAlignmentsForDifferentQSC(HitList& hitlist, Alignment& Qali,
			char inputformat, float* qsc, size_t nqsc,
			HitList& recalculated_hitlist);
	void uniqueHitlist(HitList& hitlist);
};

#endif /* HHBLITS_H_ */
