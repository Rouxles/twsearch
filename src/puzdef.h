#ifndef PUZDEF_H
#include <vector>
#include <math.h>
#include "util.h"
using namespace std ;
/*
 *   This is the core code, where we store a puzzle definition and
 *   the values for a puzzle.  The puzzle definition is a sequence
 *   of sets.  Each set has a permutation and orientation component.
 *   Values are stored as a sequence of uchars; first comes the
 *   permutation component and then the orientation component.
 *
 *   Right now the code is simple and general; it is likely we can
 *   gain a fair bit by specializing specific cases.
 */
extern double dllstates ;
/*
 *   gmoda is used to calculate orientations for a given count of
 *   orientations.  Let's say we're working on a case where there
 *   are o orientations.  The first 2o values are simply mod o.
 *   To support don't care orientations without impacting branch
 *   prediction or instruction count, values 2o..4o-1 are simply 2o.
 *   Setting the orientation *value* then to 2o means that any
 *   twist leaves it at 2o.  This is a bit of a hack but lets us do
 *   don't care orientations without changing any of the normal
 *   straight line code.  Note that this affects indexing, so we
 *   make the indexing functions check and blow up if needed.
 */
extern uchar *gmoda[256] ;
struct setdef {
   int size, off ;
   const char *name ;
   uchar omod ;
   int pbits, obits, pibits, psum ;
   bool uniq, pparity, oparity, wildo ;
   double logstates ;
   unsigned long long llperms, llords, llstates ;
   vector<int> cnts ; // only not empty when not unique.
   setdef() : size(0), off(0), name(0), omod(0), pbits(0), obits(0), pibits(0),
              psum(0), uniq(0), pparity(0), oparity(0), wildo(0), logstates(0),
              llperms(0), llords(0), llstates(0), cnts() {}
   void mulp(const uchar *ap, const uchar *bp, uchar *cp) const {
      for (int j=0; j<size; j++)
         cp[j] = ap[bp[j]] ;
   }
   // the right side must be a move so we can access the permutation part
   void mulo(const uchar *ap, const uchar *bp, uchar *cp) const {
      if (omod > 1) {
         uchar *moda = gmoda[omod] ;
         for (int j=0; j<size; j++)
            cp[j] = moda[ap[bp[j-size]]+bp[j]] ;
      } else {
         for (int j=0; j<size; j++)
            cp[j] = 0 ;
      }
   }
} ;
typedef vector<setdef> setdefs_t ;
struct setval {
   setval() : dat(0) {}
   setval(uchar *dat_) : dat(dat_) {}
   uchar *dat ;
} ;
struct illegal_t {
   int pos ;
   ull mask ;
} ;
struct moove {
   moove() : name(0), pos(0), cost(1) {}
   const char *name ;
   setval pos ;
   int cost, base, twist, cs ;
} ;
extern int origroup ;
struct movealias {
   const char *src, *dst ;
} ;
struct puzdef {
   puzdef() : name(0), setdefs(), solved(0), totsize(0), id(0),
              logstates(0), llstates(0), checksum(0), haveillegal(0), wildo(0)
              {}
   const char *name ;
   setdefs_t setdefs ;
   setval solved ;
   vector<moove> basemoves, moves, parsemoves, rotations, rotgroup ;
   vector<movealias> aliases ;
   vector<movealias> moveseqs ;
   vector<const char *> swizzlenames ;
   vector<setval> rotinvmap ;
   vector<int> basemoveorders ;
   vector<int> rotinv ;
   vector<ull> commutes ;
   int totsize ;
   int ncs ;
   setval id ;
   double logstates ;
   unsigned long long llstates ;
   ull checksum ;
   ull optionssum ;
   vector<illegal_t> illegal ;
   char haveillegal, wildo ;
   int comparepos(const setval a, const setval b) const {
      return memcmp(a.dat, b.dat, totsize) ;
   }
   int canpackdense() const {
      for (int i=0; i<(int)setdefs.size(); i++)
         if (!setdefs[i].uniq)
            return 0 ;
      return 1 ;
   }
   int invertible() const {
      return canpackdense() ;
   }
   void assignpos(setval a, const setval b) const {
      memcpy(a.dat, b.dat, totsize) ;
   }
   void addoptionssum(const char *p) {
      while (*p)
         optionssum = 37 * optionssum + *p++ ;
   }
   int numwrongsolved(const setval a, const setval b, ull mask=-1) const ;
   int numwrong(const setval a, const setval b, ull mask=-1) const ;
   int permwrongsolved(const setval a, const setval b, ull mask=-1) const ;
   int permwrong(const setval a, const setval b, ull mask=-1) const ;
   vector<int> cyccnts(const setval a, ull sets=-1) const ;
   void cyccnts(vector<int> &r, const setval a, ull sets=-1) const ;
   static ll order(const vector<int> cc) ;
   void mul(const setval a, const setval b, setval c) const {
      const uchar *ap = a.dat ;
      const uchar *bp = b.dat ;
      uchar *cp = c.dat ;
      memset(cp, 0, totsize) ;
      for (int i=0; i<(int)setdefs.size(); i++) {
         const setdef &sd = setdefs[i] ;
         int n = sd.size ;
         for (int j=0; j<n; j++)
            cp[j] = ap[bp[j]] ;
         ap += n ;
         bp += n ;
         cp += n ;
         if (sd.omod > 1) {
            uchar *moda = gmoda[sd.omod] ;
            for (int j=0; j<n; j++)
               cp[j] = moda[ap[bp[j-n]]+bp[j]] ;
         } else {
            for (int j=0; j<n; j++)
               cp[j] = 0 ;
         }
         ap += n ;
         bp += n ;
         cp += n ;
      }
   }
   void mul3(const setval a, const setval b, const setval c, setval d) const {
      const uchar *ap = a.dat ;
      const uchar *bp = b.dat ;
      const uchar *cp = c.dat ;
      uchar *dp = d.dat ;
      memset(dp, 0, totsize) ;
      for (int i=0; i<(int)setdefs.size(); i++) {
         const setdef &sd = setdefs[i] ;
         int n = sd.size ;
         for (int j=0; j<n; j++)
            dp[j] = ap[bp[cp[j]]] ;
         ap += n ;
         bp += n ;
         cp += n ;
         dp += n ;
         if (sd.omod > 1) {
            uchar *moda = gmoda[sd.omod] ;
            for (int j=0; j<n; j++)
               dp[j] = moda[ap[bp[cp[j-n]-n]]+moda[bp[cp[j-n]]+cp[j]]] ;
         } else {
            for (int j=0; j<n; j++)
               dp[j] = 0 ;
         }
         ap += n ;
         bp += n ;
         cp += n ;
         dp += n ;
      }
   }
   // does a multiplication and a comparison at the same time.
   // c must be initialized already.
   int mulcmp3(const setval a, const setval b, const setval c, setval d) const {
      const uchar *ap = a.dat ;
      const uchar *bp = b.dat ;
      const uchar *cp = c.dat ;
      uchar *dp = d.dat ;
      int r = 0 ;
      for (int i=0; i<(int)setdefs.size(); i++) {
         const setdef &sd = setdefs[i] ;
         int n = sd.size ;
         for (int j=0; j<n; j++) {
            int nv = ap[bp[cp[j]]] ;
            if (r > 0)
               dp[j] = nv ;
            else if (nv > dp[j])
               return 1 ;
            else if (nv < dp[j]) {
               r = 1 ;
               dp[j] = nv ;
            }
         }
         ap += n ;
         bp += n ;
         cp += n ;
         dp += n ;
         if (sd.omod > 1) {
            uchar *moda = gmoda[sd.omod] ;
            for (int j=0; j<n; j++) {
               int nv = moda[ap[bp[cp[j-n]-n]]+moda[bp[cp[j-n]]+cp[j]]] ;
               if (r > 0)
                  dp[j] = nv ;
               else if (nv > dp[j])
                  return 1 ;
               else if (nv < dp[j]) {
                  r = 1 ;
                  dp[j] = nv ;
               }
            }
         }
         ap += n ;
         bp += n ;
         cp += n ;
         dp += n ;
      }
      return -r ;
   }
   int mulcmp(const setval a, const setval b, setval c) const {
      const uchar *ap = a.dat ;
      const uchar *bp = b.dat ;
      uchar *cp = c.dat ;
      int r = 0 ;
      for (int i=0; i<(int)setdefs.size(); i++) {
         const setdef &sd = setdefs[i] ;
         int n = sd.size ;
         for (int j=0; j<n; j++) {
            int nv = ap[bp[j]] ;
            if (r > 0)
               cp[j] = nv ;
            else if (nv > cp[j])
               return 1 ;
            else if (nv < cp[j]) {
               r = 1 ;
               cp[j] = nv ;
            }
         }
         ap += n ;
         bp += n ;
         cp += n ;
         if (sd.omod > 1) {
            uchar *moda = gmoda[sd.omod] ;
            for (int j=0; j<n; j++) {
               int nv = moda[ap[bp[j-n]]+bp[j]] ;
               if (r > 0)
                  cp[j] = nv ;
               else if (nv > cp[j])
                  return 1 ;
               else if (nv < cp[j]) {
                  r = 1 ;
                  cp[j] = nv ;
               }
            }
         }
         ap += n ;
         bp += n ;
         cp += n ;
      }
      return -r ;
   }
   int legalstate(const setval a) const {
      if (!haveillegal)
         return 1 ;
      for (auto i : illegal) {
         if ((i.mask >> a.dat[i.pos]) & 1)
            return 0 ;
      }
      return 1 ;
   }
   int invmove(int mvind) const {
      const moove &mv = moves[mvind] ;
      int b = mv.base ;
      int o = basemoveorders[b] ; 
      int twist = (o - mv.twist) % o ;
      return mvind-mv.twist+twist ;
   }
   void addillegal(const char *setname, int pos, int val) ;
   void pow(const setval a, setval b, ll cnt) const ;
   void inv(const setval a, setval b) const ;
} ;
struct stacksetval : setval {
   stacksetval(const puzdef &pd) : setval(new uchar[pd.totsize]) {
      memcpy(dat, pd.id.dat, pd.totsize) ;
   }
   stacksetval(const puzdef &pd, const setval iv) : setval(new uchar[pd.totsize]) {
      memcpy(dat, iv.dat, pd.totsize) ;
   }
   ~stacksetval() { delete [] dat ; }
} ;
struct allocsetval : setval {
   allocsetval(const puzdef &pd, const setval &iv) : setval(new uchar[pd.totsize]) {
      memcpy(dat, iv.dat, pd.totsize) ;
   }
   ~allocsetval() {
      // we drop memory here; need fix
   }
} ;
extern vector<allocsetval> posns ;
extern vector<int> movehist ;
void calculatesizes(puzdef &pd) ;
void domove(const puzdef &pd, setval p, setval pos, setval pt) ;
void domove(const puzdef &pd, setval p, setval pos) ;
void domove(const puzdef &pd, setval p, int mv) ;
void domove(const puzdef &pd, setval p, int mv, setval pt) ;
#define PUZDEF_H
#endif
