#include <iostream>
#include "solve.h"
#include "cmdlineops.h"
#include "sloppy.h"
ull solutionsfound = 0 ;
ull solutionsneeded = 1 ;
int noearlysolutions ;
int onlyimprovements ;
int phase2 ;
int optmindepth ;
string lastsolution ;
int didprepass ;
solveworker solveworkers[MAXTHREADS] ;
static uchar *pt ;
static const ll PTSIZE = 1LL<<32 ;
static const ll PTMASK = PTSIZE - 1 ;
int ptlookup(const puzdef &pd, const setval &pos, setval &w) {
   slowmodm2(pd, pos, w) ;
   ull h = (fasthash(pd.totsize, w.dat) & PTMASK) ;
   if (pt[h] == 255)
      cout << "Error in hash" << endl ;
   return pt[h] ;
}
int (*callback)(setval &pos, const vector<int> &moves, int d, int id) ;
int (*flushback)(int d) ;
void setsolvecallback(int (*f)(setval &pos, const vector<int> &moves, int d, int id), 
                      int (*g)(int)) {
   callback = f ;
   flushback = g ;
}
void *threadworker(void *o) {
   workerparam *wp = (workerparam *)o ;
   solveworkers[wp->tid].dowork(wp->pd, wp->pt) ;
   return 0 ;
}
void solveworker::init(const puzdef &pd, int d_, int id_, const setval &p) {
   if (looktmp) {
      delete [] looktmp->dat ;
      delete looktmp ;
      looktmp = 0 ;
   }
   looktmp = new allocsetval(pd, pd.solved) ;
   // make the position table big to minimize false sharing.
   while (posns.size() <= 100 || (int)posns.size() <= d_+10) {
      posns.push_back(allocsetval(pd, pd.solved)) ;
      movehist.push_back(-1) ;
   }
   pd.assignpos(posns[0], p) ;
   lookups = 0 ;
   d = d_ ;
   id = id_ ;
}
int solveworker::solverecur(const puzdef &pd, prunetable &pt, int togo, int sp, int st) {
   lookups++ ;
   int v = ptlookup(pd, posns[sp], *looktmp) ;
   if (v > togo + 1)
      return -1 ;
   if (v > togo)
      return 0 ;
   if (v == 0) {
      if (togo == 1 && didprepass && pd.comparepos(posns[sp], pd.solved) == 0)
         return 0 ;
      if (togo > 0 && noearlysolutions &&
          pd.comparepos(posns[sp], pd.solved) == 0)
         return 0 ;
   }
   if (togo == 0) {
      if (callback) {
         return callback(posns[sp], movehist, d, id) ;
      }
      if (pd.comparepos(posns[sp], pd.solved) == 0) {
         int r = 1 ;
         get_global_lock() ;
         solutionsfound++ ;
         lastsolution.clear() ;
         if (d == 0) // allow null solution to trigger
            cout << " " ;
         for (int i=0; i<d; i++) {
            cout << " " << pd.moves[movehist[i]].name ;
            if (i > 0)
               lastsolution += " " ;
            lastsolution += pd.moves[movehist[i]].name ;
         }
         cout << endl << flush ;
         if (solutionsfound < solutionsneeded)
            r = 0 ;
         release_global_lock() ;
         return r ;
      } else
         return 0 ;
   }
   ull mask = canonmask[st] ;
   const vector<int> &ns = canonnext[st] ;
   int skip = unblocked(pd, posns[sp]) ;
   for (int m=0; m<(int)pd.moves.size(); m++) {
      const moove &mv = pd.moves[m] ;
      if ((mask >> mv.cs) & 1)
         continue ;
      if ((skip >> m) & 1)
         continue ;
      pd.mul(posns[sp], mv.pos, posns[sp+1]) ;
      if (!pd.legalstate(posns[sp+1]))
         continue ;
      movehist[sp] = m ;
      v = solverecur(pd, pt, togo-1, sp+1, ns[mv.cs]) ;
      if (v == 1)
         return 1 ;
      if (!quarter && v == -1) {
         // skip similar rotations
         while (m+1 < (int)pd.moves.size() && pd.moves[m].base == pd.moves[m+1].base)
            m++ ;
      }
   }
   return 0 ;
}
int solveworker::solvestart(const puzdef &pd, prunetable &pt, int w) {
   ull initmoves = workchunks[w] ;
   int nmoves = pd.moves.size() ;
   int sp = 0 ;
   int st = 0 ;
   int togo = d ;
   while (initmoves > 1) {
      int mv = initmoves % nmoves ;
      pd.mul(posns[sp], pd.moves[mv].pos, posns[sp+1]) ;
      if (!pd.legalstate(posns[sp+1]))
         return -1 ;
      movehist[sp] = mv ;
      st = canonnext[st][pd.moves[mv].cs] ;
      sp++ ;
      togo-- ;
      initmoves /= nmoves ;
   }
   return solverecur(pd, pt, togo, sp, st) ;
}
void solveworker::dowork(const puzdef &pd, prunetable &pt) {
   while (1) {
      int w = -1 ;
      int finished = 0 ;
      get_global_lock() ;
      finished = (solutionsfound >= solutionsneeded) ;
      if (workat < (int)workchunks.size())
         w = workat++ ;
      release_global_lock() ;
      if (finished || w < 0)
         return ;
      if (solvestart(pd, pt, w) == 1)
         return ;
   }
}
int maxdepth = 1000000000 ;
int solve(const puzdef &pd, prunetable &pt, const setval p, generatingset *gs) {
   if (::pt == 0) {
      ::pt = (uchar *)malloc(PTSIZE) ;
      FILE *f = fopen("sloppy.dat", "rb") ;
      if (f == 0 || fread(::pt, 1, PTSIZE, f) != PTSIZE)
         error("! could not load pruning table") ;
      fclose(f) ;
   }
   solutionsfound = solutionsneeded ;
   if (gs && !gs->resolve(p)) {
      if (!phase2)
         cout << "Ignoring unsolvable position." << endl ;
      return -1 ;
   }
   stacksetval looktmp(pd) ;
   double starttime = walltime() ;
   ull totlookups = 0 ;
   int initd = ptlookup(pd, p, looktmp) ;
   cout << "Initd is " << initd << endl ;
   solutionsfound = 0 ;
   int hid = 0 ;
 cout << "Outer loop" << endl ;
   for (int d=initd; d <= maxdepth; d++) {
 cout << "Working from depth " << d << endl ;
      if (onlyimprovements && d >= globalinputmovecount)
         break ;
      if (d < optmindepth)
         continue ;
      hid = d ;
      if (0 && d - initd > 3)
         makeworkchunks(pd, d, 0) ;
      else
         makeworkchunks(pd, 0, 0) ;
      int wthreads = setupthreads(pd, pt) ;
      for (int t=0; t<wthreads; t++)
         solveworkers[t].init(pd, d, t, p) ;
#ifdef USE_PTHREADS
      for (int i=1; i<wthreads; i++)
         spawn_thread(i, threadworker, &(workerparams[i])) ;
      threadworker((void*)&workerparams[0]) ;
      for (int i=1; i<wthreads; i++)
         join_thread(i) ;
#else
      threadworker((void*)&workerparams[0]) ;
#endif
      for (int i=0; i<wthreads; i++) {
         totlookups += solveworkers[i].lookups ;
         pt.addlookups(solveworkers[i].lookups) ;
      }
      if (solutionsfound >= solutionsneeded) {
         duration() ;
         double actualtime = start - starttime ;
         cout << "Found " << solutionsfound << " solution" <<
                 (solutionsfound != 1 ? "s" : "") << " at maximum depth " <<
                 d << " lookups " << totlookups << " in " << actualtime <<
                 " rate " << (totlookups/actualtime) << endl << flush ;
         return d ;
      }
      double dur = duration() ;
      if (verbose) {
         if (verbose > 1 || dur > 1)
            cout << "Depth " << d << " finished in " << dur << endl << flush ;
      }
      if (flushback)
         if (flushback(d))
            break ;
      if (d != maxdepth)
         pt.checkextend(pd) ; // fill table up a bit more if needed
   }
   if (!phase2 && callback == 0)
      cout << "No solution found in " << hid << endl << flush ;
   return -1 ;
}
