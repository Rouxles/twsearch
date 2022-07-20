#include <map>
#include <set>
#include <iostream>
#include "canon.h"
#include "ordertree.h"
#include "findalgo.h"
#include "sloppy.h"
#include "prunetable.h"
static ll levcnts = 0 ;
static set<ull> world ;
void recurorder(const puzdef &pd, int togo, int sp, int st) {
   if (togo == 0) {
      if (pd.comparepos(pd.solved, posns[sp]) != 0 &&
          unblocked(pd, posns[sp]) == 0) {
         ull h = fasthash(pd.totsize, posns[sp].dat) ;
         if (world.find(h) == world.end()) {
            world.insert(h) ;
            for (int i=0; i<sp; i++)
               cout << " " << pd.moves[movehist[i]].name ;
            cout << endl ;
         }
      }
      levcnts++ ;
      return ;
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
      movehist[sp] = m ;
      pd.mul(posns[sp], mv.pos, posns[sp+1]) ;
      if (pd.legalstate(posns[sp+1]))
         recurorder(pd, togo-1, sp+1, ns[mv.cs]) ;
   }
}
void ordertree(const puzdef &pd) {
   for (int d=0; ; d++) {
      levcnts = 0 ;
      posns.clear() ;
      movehist.clear() ;
      while ((int)posns.size() <= d + 1) {
         posns.push_back(allocsetval(pd, pd.id)) ;
         movehist.push_back(-1) ;
      }
      recurorder(pd, d, 0, 0) ;
      cout << "At depth " << d << " levcnts " << levcnts << " world " << world.size() << endl << flush ;
   }
}
