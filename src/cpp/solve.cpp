#include "solve.h"
#include "cmdlineops.h"
#include <iostream>
ull solutionsfound = 0;
ull solutionsneeded = 1;
int noearlysolutions;
int onlyimprovements;
int phase2;
int optmindepth;
int randomstart;
string lastsolution;
int didprepass;
int requesteduthreading = 4;
int workinguthreading = 0;
solveworker solveworkers[MAXTHREADS];
int (*callback)(setval pos, const vector<int> &moves, int d, int id);
int (*flushback)(int d);
static vector<vector<int>> randomized;
void setsolvecallback(int (*f)(setval pos, const vector<int> &moves, int d,
                               int id),
                      int (*g)(int)) {
  callback = f;
  flushback = g;
}
void *threadworker(void *o) {
  workerparam *wp = (workerparam *)o;
  solveworkers[wp->tid].solveiter(wp->pd, wp->pt, solveworkers[wp->tid].p);
  return 0;
}
void microthread::init(const puzdef &pd, int d_, int tid_, const setval p_) {
  if (looktmp) {
    delete[] looktmp->dat;
    delete looktmp;
    looktmp = 0;
  }
  looktmp = new allocsetval(pd, pd.solved);
  // make the position table big to minimize false sharing.
  while (posns.size() <= 100 || (int)posns.size() <= d_ + 10) {
    posns.push_back(allocsetval(pd, pd.solved));
    movehist.push_back(-1);
  }
  pd.assignpos(posns[0], p_);
  d = d_;
  tid = tid_;
}
void solveworker::init(int d_, int tid_, const setval p_) {
  lookups = 0;
  checkincrement = 10000 + myrand(10000);
  checktarget = lookups + checkincrement;
  d = d_;
  tid = tid_;
  p = p_;
  rover = 0;
}
int microthread::possibsolution(const puzdef &pd) {
  if (callback) {
    return callback(posns[sp], movehist, d, tid);
  }
  if (pd.comparepos(posns[sp], pd.solved) == 0) {
    int r = 1;
    get_global_lock();
    solutionsfound++;
    lastsolution.clear();
    if (d == 0) // allow null solution to trigger
      cout << " ";
    for (int i = 0; i < d; i++) {
      cout << " " << pd.moves[movehist[i]].name;
      if (i > 0)
        lastsolution += " ";
      lastsolution += pd.moves[movehist[i]].name;
    }
    cout << endl << flush;
    if (solutionsfound < solutionsneeded)
      r = 0;
    release_global_lock();
    return r;
  } else
    return 0;
}
int microthread::getwork(const puzdef &pd, prunetable &pt) {
  while (1) {
    int w = -1;
    int finished = 0;
    get_global_lock();
    finished = (solutionsfound >= solutionsneeded);
    if (workat < (int)workchunks.size())
      w = workat++;
    release_global_lock();
    if (finished || w < 0) {
      this->finished = 1;
      return 0;
    }
    if (solvestart(pd, pt, w))
      return 1;
  }
}
int solveworker::solveiter(const puzdef &pd, prunetable &pt, const setval p) {
  int active = 0;
  for (int uid = 0; uid < workinguthreading; uid++) {
    uthr[uid].init(pd, d, tid, p);
    uthr[uid].finished = 0;
    if (uthr[uid].getwork(pd, pt)) {
      active++;
      lookups++;
    }
  }
  while (active) {
    int uid = rover++;
    if (rover >= workinguthreading)
      rover = 0;
    if (uthr[uid].finished)
      continue;
    int v = uthr[uid].innerfetch(pd, pt);
    if (v == 0) {
      if (uthr[uid].getwork(pd, pt)) {
        lookups++;
        v = 3;
      } else {
        active--;
        continue; // this one is done; go to next one
      }
    }
    if (v != 3)
      return v;
    if (lookups > checktarget) {
      int finished = 0;
      get_global_lock();
      finished = (solutionsfound >= solutionsneeded);
      checkincrement += checkincrement / 30;
      checktarget = lookups + checkincrement;
      release_global_lock();
      if (finished)
        return 0;
    }
    uthr[uid].innersetup(pt);
    lookups++;
  }
  return 0;
}
void microthread::innersetup(prunetable &pt) {
  h = pt.prefetchindexed(pt.gethashforlookup(posns[sp], looktmp));
}
int microthread::innerfetch(const puzdef &pd, prunetable &pt) {
  int v = pt.lookuphindexed(h);
  int m, mi;
  ull mask, skipbase;
  if (v > togo) {
    v = togo - v + 1;
  } else if (v == 0 && togo == 1 && didprepass &&
             pd.comparepos(posns[sp], pd.solved) == 0) {
    v = 0;
  } else if (v == 0 && togo > 0 && noearlysolutions &&
             pd.comparepos(posns[sp], pd.solved) == 0) {
    v = 0;
  } else if (togo == 0) {
    v = possibsolution(pd);
  } else {
    mask = canonmask[st];
    skipbase = 0;
    mi = -1;
    goto downstack;
  }
upstack:
  if (solvestates.size() == 0)
    return v;
  {
    auto &ss = solvestates[solvestates.size() - 1];
    togo++;
    sp--;
    st = ss.st;
    mi = ss.mi;
    mask = ss.mask;
    skipbase = ss.skipbase;
  }
  solvestates.pop_back();
  if (v == 1)
    goto upstack;
  if (v < 0) {
    if (!quarter && v == -1) {
      m = randomstart ? randomized[togo][mi] : mi;
      if (pd.moves[m].base < 64)
        skipbase |= 1LL << pd.moves[m].base;
    } else {
      skipbase = -1; // skip *all* remaining moves!
    }
  }
downstack:
  mi++;
  if (mi >= (int)pd.moves.size()) {
    v = 0;
    goto upstack;
  }
  m = randomstart ? randomized[togo][mi] : mi;
  const moove &mv = pd.moves[m];
  if (!quarter && mv.base < 64 && ((skipbase >> mv.base) & 1))
    goto downstack;
  if ((mask >> mv.cs) & 1)
    goto downstack;
  pd.mul(posns[sp], mv.pos, posns[sp + 1]);
  if (!pd.legalstate(posns[sp + 1]))
    goto downstack;
  movehist[sp] = m;
  solvestates.push_back({st, mi, mask, skipbase});
  togo--;
  sp++;
  st = canonnext[st][mv.cs];
  return 3;
}
int microthread::solvestart(const puzdef &pd, prunetable &pt, int w) {
  ull initmoves = workchunks[w];
  int nmoves = pd.moves.size();
  sp = 0;
  st = 0;
  togo = d;
  while (initmoves > 1) {
    int mv = initmoves % nmoves;
    pd.mul(posns[sp], pd.moves[mv].pos, posns[sp + 1]);
    if (!pd.legalstate(posns[sp + 1])) {
      return 0;
    }
    movehist[sp] = mv;
    st = canonnext[st][pd.moves[mv].cs];
    sp++;
    togo--;
    initmoves /= nmoves;
  }
  solvestates.clear();
  innersetup(pt);
  return 1;
}
int maxdepth = 1000000000;
int solve(const puzdef &pd, prunetable &pt, const setval p, generatingset *gs) {
  solutionsfound = solutionsneeded;
  if (gs && !gs->resolve(p)) {
    if (!phase2)
      cout << "Ignoring unsolvable position." << endl;
    return -1;
  }
  stacksetval looktmp(pd);
  double starttime = walltime();
  ull totlookups = 0;
  int initd = pt.lookup(p, &looktmp);
  solutionsfound = 0;
  int hid = 0;
  randomized.clear();
  for (int d = initd; d <= maxdepth; d++) {
    ll olookups = pt.lookupcnt;
    if (randomstart) {
      while ((int)randomized.size() <= d) {
        randomized.push_back({});
        vector<int> &r = randomized[randomized.size() - 1];
        for (int i = 0; i < (int)pd.moves.size(); i++)
          r.push_back(i);
        for (int i = 0; i < (int)r.size(); i++) {
          int j = i + myrand(r.size() - i);
          swap(r[i], r[j]);
        }
      }
    }
    if (onlyimprovements && d >= globalinputmovecount)
      break;
    if (d < optmindepth)
      continue;
    hid = d;
    if (d - initd > 3)
      makeworkchunks(pd, d, p, requesteduthreading);
    else
      makeworkchunks(pd, 0, p, requesteduthreading);
    int wthreads = setupthreads(pd, pt);
    workinguthreading =
        min(requesteduthreading,
            (int)(workchunks.size() + numthreads - 1) / numthreads);
    for (int t = 0; t < wthreads; t++)
      solveworkers[t].init(d, t, p);
#ifdef USE_PTHREADS
    for (int i = 1; i < wthreads; i++)
      spawn_thread(i, threadworker, &(workerparams[i]));
    threadworker((void *)&workerparams[0]);
    for (int i = 1; i < wthreads; i++)
      join_thread(i);
#else
    threadworker((void *)&workerparams[0]);
#endif
    for (int i = 0; i < wthreads; i++) {
      totlookups += solveworkers[i].lookups;
      pt.addlookups(solveworkers[i].lookups);
    }
    if (solutionsfound >= solutionsneeded) {
      duration();
      double actualtime = start - starttime;
      cout << "Found " << solutionsfound << " solution"
           << (solutionsfound != 1 ? "s" : "") << " max depth " << d
           << " lookups " << totlookups << " in " << actualtime << " rate "
           << (totlookups / actualtime / 1e6) << endl
           << flush;
      return d;
    }
    double dur = duration();
    ll lookups = pt.lookupcnt - olookups;
    double rate = lookups / dur / 1e6;
    if (verbose) {
      if (verbose > 1 || dur > 1)
        cout << "Depth " << d << " in " << dur << " lookups " << lookups
             << " rate " << rate << endl
             << flush;
    }
    if (flushback)
      if (flushback(d))
        break;
    if (d != maxdepth)
      pt.checkextend(pd); // fill table up a bit more if needed
  }
  if (!phase2 && callback == 0)
    cout << "No solution found in " << hid << endl << flush;
  return -1;
}
