#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <strings.h>
#include <math.h>
#include <cstdio>
#include <functional>
#include "city.h"
#include "util.h"
#include "threads.h"
#include "puzdef.h"
#include "generatingset.h"
#include "orderedgs.h"
#include "readksolve.h"
#include "index.h"
#include "antipode.h"
#include "canon.h"
#include "god.h"
#include "findalgo.h"
#include "workchunks.h"
#include "prunetable.h"
#include "solve.h"
#include "test.h"
#include "parsemoves.h"
#include "filtermoves.h"
#include "parsemoves.h"
#include "cmdlineops.h"
#include "rotations.h"
#include "twsearch.h"
#include "coset.h"
#include "descsets.h"
#include "ordertree.h"
using namespace std ;
int checkbeforesolve ;
generatingset *gs ;
int bestsolve = 1000000 ;
int optmaxdepth = 0 ;
void dophase2(const puzdef &pd, setval scr, setval p1sol, prunetable &pt,
              const char *p1str) {
   stacksetval p2(pd) ;
   if (optmaxdepth == 0)
      optmaxdepth = maxdepth ;
   pd.mul(scr, p1sol, p2) ;
   maxdepth = min(optmaxdepth - globalinputmovecount,
                  bestsolve - globalinputmovecount - 1) ;
   int r = solve(pd, pt, p2, gs) ;
   if (r >= 0) {
      cout << "Phase one was " << p1str << endl ;
      bestsolve = r + globalinputmovecount ;
      cout << "Found a solution totaling " << bestsolve << " moves." << endl ;
   }
}
int dogod, docanon, doalgo, dosolvetest, dotimingtest, douniq, doinv,
    dosolvelines, doorder, doshowmoves, doshowpositions, genrand,
    checksolvable, doss, doorderedgs, dosyms, usehashenc, docancelseqs,
    domergeseqs, dounrotateseqs, doshortenseqs, docoset, douniqsymm,
    dodescsets, doordertree, dowrong ;
const char *scramblealgo = 0 ;
const char *legalmovelist = 0 ;
static int initialized = 0 ;
int seed = 0 ;
void doinit() {
   if (!initialized) {
// disable saving pruning tables when running under WASM
#ifdef WASM
      nowrite = 1 ;
#endif
      init_util() ;
      init_threads() ;
      if (seed)
         srand48(seed) ;
      else
         srand48(time(0)) ;
      initialized = 1 ;
   }
}
int forcearray = 0 ;
/*
 *   Can be called multiple times at the start.
 */
void processargs(int &argc, argvtype &argv) {
   while (argc > 1 && argv[1][0] == '-') {
      argc-- ;
      argv++ ;
      if (argv[0][1] == '-') {
         if (strcmp(argv[0], "--moves") == 0) {
            legalmovelist = argv[1] ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--showmoves") == 0) {
            doshowmoves++ ;
         } else if (strcmp(argv[0], "--showpositions") == 0) {
            doshowpositions++ ;
         } else if (strcmp(argv[0], "--newcanon") == 0) {
            ccount = atol(argv[1]) ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--nocorners") == 0) {
            nocorners++ ;
         } else if (strcmp(argv[0], "--nocenters") == 0) {
            nocenters++ ;
         } else if (strcmp(argv[0], "--noorientation") == 0) {
            ignoreori = 1 ;
         } else if (strcmp(argv[0], "--omit") == 0) {
            omitsets.insert(argv[1]) ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--distinguishall") == 0) {
            distinguishall = 1 ;
         } else if (strcmp(argv[0], "--noearlysolutions") == 0) {
            noearlysolutions = 1 ;
         } else if (strcmp(argv[0], "--checkbeforesolve") == 0) {
            checkbeforesolve = 1 ;
         } else if (strcmp(argv[0], "--orientationgroup") == 0) {
            origroup = atol(argv[1]) ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--noedges") == 0) {
            noedges++ ;
         } else if (strcmp(argv[0], "--scramblealg") == 0) {
            scramblealgo = argv[1] ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--schreiersims") == 0) {
            doss = 1 ;
         } else if (strcmp(argv[0], "--orderedgs") == 0) {
            doorderedgs = 1 ;
         } else if (strcmp(argv[0], "--showsymmetry") == 0) {
            dosyms = 1 ;
         } else if (strcmp(argv[0], "--nowrite") == 0) {
            nowrite++ ;
         } else if (strcmp(argv[0], "--mergeseqs") == 0) {
            domergeseqs++ ;
         } else if (strcmp(argv[0], "--unrotateseqs") == 0) {
            dounrotateseqs++ ;
         } else if (strcmp(argv[0], "--shortenseqs") == 0) {
            doshortenseqs++ ;
         } else if (strcmp(argv[0], "--cancelseqs") == 0) {
            docancelseqs++ ;
         } else if (strcmp(argv[0], "--mindepth") == 0) {
            optmindepth = atol(argv[1]) ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--maxdepth") == 0) {
            maxdepth = atol(argv[1]) ;
            argc-- ;
            argv++ ;
         } else if (strcmp(argv[0], "--coset") == 0) {
            cosetmovelist = argv[1] ;
            cosetmoveseq = argv[2] ;
            docoset++ ;
            argc -= 2 ;
            argv += 2 ;
         } else if (strcmp(argv[0], "--listcosets") == 0) {
            listcosets++ ;
         } else if (strcmp(argv[0], "--relaxcosets") == 0) {
            relaxcosets++ ;
         } else if (strcmp(argv[0], "--compact") == 0) {
            compact++ ;
         } else if (strcmp(argv[0], "--describesets") == 0) {
            dodescsets++ ;
         } else if (strcmp(argv[0], "--ordertree") == 0) {
            doordertree++ ;
         } else if (strcmp(argv[0], "--maxwrong") == 0) {
            dowrong++ ;
            maxwrong = atol(argv[1]) ;
            argc-- ;
            argv++ ;
         } else {
            error("! Argument not understood ", argv[0]) ;
         }
      } else {
         switch (argv[0][1]) {
case 'q':
            quarter++ ;
            break ;
case 'v':
            verbose++ ;
            if (argv[0][2] != 0)
               verbose = argv[0][2] - '0' ;
            break ;
case 'm':
case 'd':
            maxdepth = atol(argv[1]) ;
            argc-- ;
            argv++ ;
            break ;
case 'r':
            genrand = 1 ;
            if (argv[0][2] != 0)
               genrand = atol(argv[0]+2) ;
            break ;
case 'R':
            seed = atol(argv[1]) ;
            argc-- ;
            argv++ ;
            break ;
case 'H':
            usehashenc++ ;
            break ;
case 'M':
            maxmem = 1048576 * atoll(argv[1]) ;
            argc-- ;
            argv++ ;
            break ;
case 'y':
            symcoordgoal = atoll(argv[1]) ;
            if (symcoordgoal <= 0)
               symcoordgoal = 1 ;
            argc-- ;
            argv++ ;
            break ;
case 'c':
            solutionsneeded = atoll(argv[1]) ;
            argc-- ;
            argv++ ;
            break ;
case 'g':
            dogod++ ;
            break ;
case 'o':
            doorder++ ;
            break ;
case 'U':
            douniqsymm++ ;
            if (argv[0][2] >= '0')
               proclim = atoll(argv[0]+2) ;
            break ;
case 'u':
            douniq++ ;
            if (argv[0][2] >= '0')
               proclim = atoll(argv[0]+2) ;
            break ;
case 'i':
            doinv++ ;
            break ;
case 's':
            dosolvelines++ ;
            if (argv[0][2] == 'i')
               onlyimprovements = 1 ;
            break ;
case 'C':
            docanon++ ;
            if (argv[0][2] >= '0')
               canonlim = atoll(argv[0]+2) ;
            break ;
case 'F':
            forcearray++ ;
            break ;
case 'a':
            antipodecount = atoll(argv[1]) ;
            argc-- ;
            argv++ ;
            break ;
case 'A':
            doalgo = -1 ;
            for (int pp=2; argv[0][pp]; pp++)
               if (argv[0][pp] == '1')
                  doalgo = 1 ;
               else if (argv[0][pp] == '2')
                  doalgo = 2 ;
               else if (argv[0][pp] == '3')
                  doalgo = 3 ;
               else if (argv[0][pp] == 's')
                  algostrict = 1 ;
            break ;
case 'T':
            dotimingtest++ ;
            break ;
case 'S':
            dosolvetest++ ;
            if (argv[0][2])
               scramblemoves = atol(argv[0]+2) ;
            break ;
case 't':
            numthreads = atol(argv[1]) ;
            if (numthreads > MAXTHREADS)
               error("Numthreads cannot be more than ", to_string(MAXTHREADS)) ;
            argc-- ;
            argv++ ;
            break ;
case '2':
            phase2 = 1 ;
            break ;
default:
            error("! did not argument ", argv[0]) ;
         }
      }
   }
}
puzdef makepuzdef(istream *f) {
   doinit() ;
   puzdef pd = readdef(f) ;
   addmovepowers(pd) ;
   if (legalmovelist)
      filtermovelist(pd, legalmovelist) ;
   if (nocorners)
      pd.addoptionssum("nocorners") ;
   if (nocenters)
      pd.addoptionssum("nocenters") ;
   if (noedges)
      pd.addoptionssum("noedges") ;
   if (ignoreori)
      pd.addoptionssum("noorientation") ;
   if (omitsets.size()) {
      pd.addoptionssum("omit") ;
      for (auto s: omitsets)
         pd.addoptionssum(s.c_str()) ;
   }
   if (distinguishall)
      pd.addoptionssum("distinguishall") ;
   if (doss || checkbeforesolve)
      gs = new generatingset(pd) ;
   if (pd.rotations.size())
      calcrotations(pd) ;
   calculatesizes(pd) ;
   calclooseper(pd) ;
   if (ccount == 0)
      makecanonstates(pd) ;
   else
      makecanonstates2(pd) ;
   cout << "Calculated canonical states in " << duration() << endl << flush ;
   showcanon(pd, docanon) ;
//   gensymm(pd) ;
   return pd ;
}
puzdef makepuzdef(string s) {
   stringstream is(s) ;
   return makepuzdef(&is) ;
}
#ifndef ASLIBRARY
int main(int argc, const char **argv) {
   cout << "# This is twsearch 0.2 (C) 2020 Tomas Rokicki." << endl ;
   cout << "#" ;
   for (int i=0; i<argc; i++)
      cout << " " << argv[i] ;
   cout << endl << flush ;
   processargs(argc, argv) ;
   if (argc <= 1)
      error("! please provide a twsearch file name on the command line") ;
   ifstream f ;
   f.open(argv[1], ifstream::in) ;
   if (f.fail())
      error("! could not open file ", argv[1]) ;
   int sawdot = 0 ;
   inputbasename.clear() ;
   for (int i=0; argv[1][i]; i++) {
      if (argv[1][i] == '.')
         sawdot = 1 ;
      else if (argv[1][i] == '/' || argv[1][i] == '\\') {
         sawdot = 0 ;
         inputbasename.clear() ;
      } else if (!sawdot)
         inputbasename.push_back(argv[1][i]) ;
   }
   puzdef pd = makepuzdef(&f) ;
   if (doorderedgs)
      runorderedgs(pd) ;
   if (dodescsets) {
      descsets(pd) ;
   }
   if (doordertree) {
      ordertree(pd) ;
   }
   if (genrand) {
      for (int i=0; i<genrand; i++)
         showrandompos(pd) ;
      return 0 ;
   }
   if (dogod) {
      int statesfit2 = pd.logstates <= 50 && ((ll)(pd.llstates >> 2)) <= maxmem ;
      int statesfitsa = forcearray ||
          (pd.logstates <= 50 &&
             ((ll)(pd.llstates * sizeof(loosetype) * looseper) <= maxmem)) ;
      if (!forcearray && statesfit2 && pd.canpackdense() && pd.rotations.size() == 0) {
         cout << "Using twobit arrays." << endl ;
         dotwobitgod2(pd) ;
      } else if (statesfitsa) {
         if (pd.rotations.size()) {
            cout << "Using sorting bfs symm and arrays." << endl ;
            doarraygodsymm(pd) ;
         } else {
            cout << "Using sorting bfs and arrays." << endl ;
            doarraygod(pd) ;
         }
      } else {
         cout << "Using canonical sequences and arrays." << endl ;
         doarraygod2(pd) ;
      }
   }
   if (doalgo)
      findalgos(pd, doalgo) ;
   if (dosolvetest)
      solvetest(pd, gs) ;
   if (dotimingtest)
      timingtest(pd) ;
   if (! phase2 && scramblealgo)
      solvecmdline(pd, scramblealgo, gs) ;
   if (douniq)
      processlines(pd, uniqit) ;
   if (dowrong)
      processlines(pd, wrongit) ;
   if (douniqsymm)
      processlines2(pd, uniqitsymm) ;
   if (doinv)
      processlines3(pd, invertit) ;
   if (domergeseqs)
      processlines3(pd, mergeit) ;
   if (dounrotateseqs)
      processlines4(pd, unrotateit) ;
   if (doshortenseqs)
      processlines3(pd, shortenit) ;
   if (docancelseqs)
      processlines3(pd, cancelit) ;
   if (dosyms)
      processlines(pd, symsit) ;
   if (doorder)
      processlines2(pd, orderit) ;
   if (doshowmoves)
      processlines2(pd, emitmove) ;
   if (doshowpositions)
      processlines(pd, emitposition) ;
   if (dosolvelines) {
      prunetable pt(pd, maxmem) ;
      string emptys ;
      processlines(pd, [&](const puzdef &pd, setval p, const char *) {
                          solveit(pd, pt, emptys, p) ;
                       }) ;
   }
   if (docoset) {
      runcoset(pd) ;
   }
   if (phase2) {
      if (argc <= 2 && !scramblealgo)
         error("! need a scramble file for phase 2") ;
      stacksetval scr(pd) ;
      if (scramblealgo) {
         pd.assignpos(scr, pd.solved) ;
         vector<setval> movelist = parsemovelist_generously(pd, scramblealgo) ;
         for (int i=0; i<(int)movelist.size(); i++)
            domove(pd, scr, movelist[i]) ;
      } else {
         ifstream scrambles ;
         scrambles.open(argv[2], ifstream::in) ;
         if (scrambles.fail())
            error("! could not open scramble file ", argv[2]) ;
         readfirstscramble(&scrambles, pd, scr) ;
         scrambles.close() ;
      }
      prunetable pt(pd, maxmem) ;
      processlines2(pd, [&](const puzdef &pd, setval p1sol, const char *p1str) {
                               dophase2(pd, scr, p1sol, pt, p1str); }) ;
   } else if (argc > 2) {
      ifstream scrambles ;
      scrambles.open(argv[2], ifstream::in) ;
      if (scrambles.fail())
         error("! could not open scramble file ", argv[2]) ;
      processscrambles(&scrambles, pd, gs) ;
      scrambles.close() ;
   }
   double s = 0 ;
   stacksetval p1(pd), p2(pd) ;
   pd.assignpos(p1, pd.id) ;
   ll cnts[2521] ;
   for (int i=0; i<2521; i++)
      cnts[i] = 0 ;
   for (ll trial=1; ; trial++) {
      int rmv = myrand(pd.moves.size()) ;
      pd.mul(p1, pd.moves[rmv].pos, p2) ;
      pd.assignpos(p1, p2) ;
      vector<int> cc = pd.cyccnts(p1) ;
      ll o = puzdef::order(cc) ;
      s += o ;
      if (o <= 2520)
         cnts[o]++ ;
      if ((trial & (trial - 1)) == 0) {
         double avg = s / trial ;
         cout << trial << " " << avg << endl << flush ;
         for (int i=0; i<=2520; i++)
            if (cnts[i])
               cout << i << " " << cnts[i] << " " << cnts[i]/(double)trial << endl ;
      }
   }
}
#endif
