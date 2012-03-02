                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2012 Andrew Trevorrow and Tomas Rokicki.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Web site:  http://sourceforge.net/projects/golly
 Authors:   rokicki@gmail.com  andrew@trevorrow.com

                        / ***/

#include "ruletreealgo.h"
#include "util.h"          // for lifegetuserrules, lifegetrulesdir, lifewarning
#include <stdlib.h>
// for case-insensitive string comparison
#include <string.h>
#ifndef WIN32
   #define stricmp strcasecmp
#endif
#include <vector>
#include <cstdio>
#include <string>
using namespace std ;

int ruletreealgo::NumCellStates() {
   return num_states ;
}

const int MAXFILELEN = 4096 ;

/* provide the ability to load the default rule without requiring a file */
static const char *defaultRuleData[] = {
  "num_states=2", "num_neighbors=8", "num_nodes=32",
  "1 0 0", "2 0 0", "1 0 1", "2 0 2", "3 1 3", "1 1 1", "2 2 5", "3 3 6",
  "4 4 7", "2 5 0", "3 6 9", "4 7 10", "5 8 11", "3 9 1", "4 10 13",
  "5 11 14", "6 12 15", "3 1 1", "4 13 17", "5 14 18", "6 15 19",
  "7 16 20", "4 17 17", "5 18 22", "6 19 23", "7 20 24", "8 21 25",
  "5 22 22", "6 23 27", "7 24 28", "8 25 29", "9 26 30", 0 } ;

static FILE *OpenTreeFile(const char *rule, const char *dir, char *path)
{
   // look for rule.tree in given dir and set path
   if (strlen(dir) + strlen(rule) + 15 > (unsigned int)MAXFILELEN) {
      lifewarning("Path too long") ;
      return NULL ;
   }
   sprintf(path, "%s%s.tree", dir, rule) ;
   // change "dangerous" characters to underscores
   for (char *p=path + strlen(dir); *p; p++)
      if (*p == '/' || *p == '\\') *p = '_' ;
   return fopen(path, "r") ;
}

const char* ruletreealgo::setrule(const char* s) {

   const char *colonptr = strchr(s, ':');
   string rule_name(s);
   if (colonptr)
      rule_name.assign(s,colonptr);

   // nicer to check for different versions of default rule
   int isDefaultRule = (stricmp(rule_name.c_str(), "B3/S23") == 0 ||
                        stricmp(rule_name.c_str(), "B3S23") == 0 ||
                        strcmp(rule_name.c_str(), "23/3") == 0) ;
   char strbuf[MAXFILELEN+1] ;
   FILE *f = 0 ;
   linereader lr(0) ;
   if (!isDefaultRule) {
      if (strlen(rule_name.c_str()) >= (unsigned int)MAXRULESIZE) {
         return "Rule length too long" ;
      }
      // look for rule.tree in user's rules dir then in Golly's rules dir
      f = OpenTreeFile(rule_name.c_str(), lifegetuserrules(), strbuf);
      if (f == 0)
         f = OpenTreeFile(rule_name.c_str(), lifegetrulesdir(), strbuf);
      if (f == 0) {
         return "File not found" ;
      }
      lr.setfile(f) ;
      lr.setcloseonfree() ;
   }
   
   // check for rule suffix like ":T200,100" to specify a bounded universe
   if (colonptr) {
      const char* err = setgridsize(colonptr);
      if (err) return err;
   } else {
      // universe is unbounded
      gridwd = 0;
      gridht = 0;
   }
   
   int lineno = 0 ;
   int mnum_states=-1, mnum_neighbors=-1, mnum_nodes=-1 ;
   vector<int> dat ;
   vector<state> datb ;
   vector<int> noff ;
   int lev = 1000 ;
   for (;;) {
      if (isDefaultRule) {
         if (defaultRuleData[lineno] == 0)
            break ;
         strcpy(strbuf, defaultRuleData[lineno]) ;
      } else {
         if (lr.fgets(strbuf, MAXFILELEN) == 0)
            break ;
      }
      lineno++ ;
      if (strbuf[0] != '#' && strbuf[0] != 0 &&
          sscanf(strbuf, " num_states = %d", &mnum_states) != 1 &&
          sscanf(strbuf, " num_neighbors = %d", &mnum_neighbors) != 1 &&
          sscanf(strbuf, " num_nodes = %d", &mnum_nodes) != 1) {
         if (mnum_states < 2 || mnum_states > 256 ||
             (mnum_neighbors != 4 && mnum_neighbors != 8) ||
             mnum_nodes < mnum_neighbors || mnum_nodes > 100000000) {
            return "Bad basic values" ;
         }
         if (strbuf[0] < '1' || strbuf[0] > '0' + 1 + mnum_neighbors) {
            return "Bad line in ruletree file 1" ;
         }
         lev = strbuf[0] - '0' ;
         int vcnt = 0 ;
         char *p = strbuf + 1 ;
         if (lev == 1)
            noff.push_back(datb.size()) ;
         else
            noff.push_back(dat.size()) ;
         while (*p) {
            while (*p && *p <= ' ')
               p++ ;
            int v = 0 ;
            while (*p > ' ') {
               if (*p < '0' || *p > '9') {
                  return "Bad line in ruletree file 2" ;
               }
               v = v * 10 + *p++ - '0' ;
            }
            if (lev == 1) {
               if (v < 0 || v >= mnum_states) {
                  return "Bad state value in ruletree file" ;
               }
               datb.push_back((state)v) ;
            } else {
               if (v < 0 || ((unsigned int)v) >= noff.size()) {
                  return "Bad node value in ruletree file" ;
               }
               dat.push_back(noff[v]) ;
            }
            vcnt++ ;
         }
         if (vcnt != mnum_states) {
            return "Bad number of values on ruletree line" ;
         }
      }
   }
   // disabled to avoid crash on Linux
   // lr.close() ;
   if (dat.size() + datb.size() != (unsigned int)(mnum_nodes * mnum_states))
      return "Bad count of values in ruletree file" ;
   if (lev != mnum_neighbors + 1)
      return "Bad last node (wrong level)" ;
   int *na = (int*)calloc(sizeof(int), dat.size()) ;
   state *nb = (state*)calloc(sizeof(state), datb.size()) ;
   if (na == 0 || nb == 0)
      return "Out of memory in ruletree allocation" ;
   if (a)
      free(a) ;
   if (b)
      free(b) ;
   num_nodes = mnum_nodes ;
   num_states = mnum_states ;
   num_neighbors = mnum_neighbors ;
   for (unsigned int i=0; i<dat.size(); i++)
      na[i] = dat[i] ;
   for (unsigned int i=0; i<datb.size(); i++)
      nb[i] = datb[i] ;
   a = na ;
   b = nb ;
   base = noff[noff.size()-1] ;
   maxCellStates = num_states ;
   ghashbase::setrule(rule_name.c_str()) ;
   
   // set canonical rule string returned by getrule()
   strcpy(rule,rule_name.c_str()) ;
   if (gridwd > 0 || gridht > 0) {
      // setgridsize() was successfully called above, so append suffix
      int len = strlen(rule) ;
      const char* bounds = canonicalsuffix() ;
      int i = 0 ;
      while (bounds[i]) rule[len++] = bounds[i++] ;
      rule[len] = 0 ;
   }
   
   return 0 ;
}

const char* ruletreealgo::getrule() {
   return rule ;
}

const char* ruletreealgo::DefaultRule() {
   return "B3/S23" ;
}

ruletreealgo::ruletreealgo() : ghashbase(), a(0), base(0), b(0),
                               num_neighbors(0),
                               num_states(0), num_nodes(0) {
   rule[0] = 0 ;
}

ruletreealgo::~ruletreealgo() {
   if (a != 0) {
      free(a) ;
      a = 0 ;
   }
   if (b != 0) {
      free(b) ;
      b = 0 ;
   }
}

state ruletreealgo::slowcalc(state nw, state n, state ne, state w, state c, state e,
                        state sw, state s, state se) {
   if (num_neighbors == 4)
     return b[a[a[a[a[base+n]+w]+e]+s]+c] ;
   else
     return b[a[a[a[a[a[a[a[a[base+nw]+ne]+sw]+se]+n]+w]+e]+s]+c] ;
}

static lifealgo *creator() { return new ruletreealgo() ; }

void ruletreealgo::doInitializeAlgoInfo(staticAlgoInfo &ai) {
   ghashbase::doInitializeAlgoInfo(ai) ;
   ai.setAlgorithmName("RuleTree") ;
   ai.setAlgorithmCreator(&creator) ;
   ai.minstates = 2 ;
   ai.maxstates = 256 ;
   // init default color scheme
   ai.defgradient = true;              // use gradient
   ai.defr1 = 255;                     // start color = red
   ai.defg1 = 0;
   ai.defb1 = 0;
   ai.defr2 = 255;                     // end color = yellow
   ai.defg2 = 255;
   ai.defb2 = 0;
   // if not using gradient then set all states to white
   for (int i=0; i<256; i++)
      ai.defr[i] = ai.defg[i] = ai.defb[i] = 255;
}
