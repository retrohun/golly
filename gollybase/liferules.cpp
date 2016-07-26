                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2013 Andrew Trevorrow and Tomas Rokicki.

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
#include "liferules.h"
#include "util.h"
#include <cstdlib>
#include <string.h>
#include <stdlib.h>

liferules::liferules() {
   canonrule[0] = 0 ;
   alternate_rules = 0 ;
   neighbormask = MOORE ;
   neighbors = 8 ;
   wolfram = 0 ;
   totalistic = true ;
   rulebits = 0 ;
   survival_offset = 9 ;
   memset(letter_bits, 0, sizeof(letter_bits)) ;
   memset(neg_letter_bits, 0, sizeof(neg_letter_bits)) ;
   memset(rule0, 0, sizeof(rule0)) ;
   memset(rule1, 0, sizeof(rule1)) ;
   memset(rule3x3, 0, sizeof(rule3x3)) ;
   valid_rule_letters = "012345678ceaiknjqrytwz-" ;
   rule_letters[0] = "ce" ;
   rule_letters[1] = "ceaikn" ;
   rule_letters[2] = "ceaiknjqry" ;
   rule_letters[3] = "ceaiknjqrytwz" ;
   static int entry0[2] = { 1, 2 } ;
   static int entry1[6] = { 5, 10, 3, 40, 33, 68 } ;
   static int entry2[10] = { 69, 42, 11, 7, 98, 13, 14, 70, 41, 97 } ;
   static int entry3[13] = { 325, 170, 15, 45, 99, 71, 106, 102, 43, 101, 105, 78, 108 } ;
   rule_neighborhoods[0] = entry0 ;
   rule_neighborhoods[1] = entry1 ;
   rule_neighborhoods[2] = entry2 ;
   rule_neighborhoods[3] = entry3 ;
}

liferules::~liferules() {
}

// set 3x3 grid based on totalistic value
void liferules::setTotalistic(int value, bool survival) {
   int mask = 0 ;
   int nbrs = 0 ;
   int nhood = 0 ;
   int i = 0 ;
   int j = 0 ;
   int offset = 0 ;

   // update the rulebits
   if (survival) {
      offset = survival_offset ;
   }
   rulebits |= 1 << (value + offset) ;

   // update the mask if survival
   if (survival) {
      mask = 0x10 ;
   }

   // fill the array based on totalistic value
   for (i = 0; i < ALL3X3; i += 32) {
      for (j = 0; j < 16; j++) {
         nbrs = 0 ;
         nhood = (i+j) & neighbormask ;
         while (nhood > 0) {
            nbrs += (nhood & 1) ;
            nhood >>= 1 ;
         }
         if (value == nbrs) {
            rule3x3[i+j+mask] = 1 ;
         }
      }
   }
}

// flip bits
int liferules::flipBits(int x) {
   return ((x & 0x07) << 6) | ((x & 0x1c0) >> 6) | (x & 0x38) ;
}

// rotate 90
int liferules::rotateBits90Clockwise(int x) {
   return ((x & 0x4) << 6) | ((x & 0x20) << 2) | ((x & 0x100) >> 2)
                        | ((x & 0x2) << 4) | (x & 0x10) | ((x & 0x80) >> 4)
                        | ((x & 0x1) << 2) | ((x & 0x8) >> 2) | ((x & 0x40) >> 6) ;
}

// set symmetrical neighborhood into 3x3 map
void liferules::setSymmetrical512(int x, int b) {
   int y = x ;
   int i = 0 ;

   // process each of the 4 rotations
   for (i=0; i<4; i++) {
      rule3x3[y] = b ;
      y = rotateBits90Clockwise(y) ;
   }

   // flip
   y = flipBits(y) ;

   // process each of the 4 rotations
   for (i=0; i<4; i++) {
      rule3x3[y] = b ;
      y = rotateBits90Clockwise(y) ;
   }
}

// set symmetrical neighborhood
void liferules::setSymmetrical(int value, bool survival, int lindex, int normal) {
   int xorbit = 0 ;
   int nindex = value - 1 ;
   int x = 0 ;
   int offset = 0 ;

   // check for homogeneous bits
   if (value == 0 || value == 8) {
      setTotalistic(value, survival) ;
   }
   else {
      // update the rulebits
      if (survival) {
         offset = survival_offset ;
      }
      rulebits |= 1 << (value + offset) ;

      // reflect the index if in second half
      if (nindex > 3) {
         nindex = 6 - nindex ;
         xorbit = 0x1ef ;
      }

      // update the letterbits
      if (normal) {
         letter_bits[value + offset] |= 1 << lindex ;
      }
      else {
         neg_letter_bits[value + offset] |= 1 << lindex ;
      }

      // lookup the neighborhood
      x = rule_neighborhoods[nindex][lindex] ^ xorbit ;
      if (survival) {
         x |= 0x10 ;
      }
      setSymmetrical512(x, normal) ;
   }
}

// set totalistic birth or survival rule from a string
void liferules::setTotalisticRuleFromString(const char *rule, bool survival) {
   char current ;

   // process each character in the rule string
   while ( (current = *rule) ) {
      rule++ ;

      // convert the digit to an integer
      current -= '0' ;

      // set totalistic
      setTotalistic(current, survival) ;
   }
}

// set rule from birth or survival string
void liferules::setRuleFromString(const char *rule, bool survival) {
   // current and nexr character
   char current ;
   char next ;

   // whether character normal or inverted
   bool normal = true ;

   // letter index
   char *letterindex = 0 ;
   int lindex = 0 ;
   int nindex = 0 ;

   // process each character
   while ( (current = *rule) ) {
      rule++ ;

      // find the index in the valid character list
      letterindex = strchr(valid_rule_letters, current) ;
      lindex = letterindex - valid_rule_letters ;

      // check if it is a digit
      if ((lindex > 0 && lindex <= 8) || (lindex == 0 && survival)) {
         // determine what follows the digit
         next = *rule ;
         nindex = 0 ;
         if (next) {
            letterindex = strchr(valid_rule_letters, next) ;
            nindex = letterindex - valid_rule_letters ;
         }

         // is the next character another digit?
         if (nindex >= 0 && nindex <= 8) {
            // yes so set totalitic with the current digit
            setTotalistic(lindex, survival) ;
         }

         // check for inversion
         normal = true ;
         if (next == '-') {
            rule++ ;
            next = *rule ;

            // invert following character meanings
            normal = false ;
         }

         // process non-totalistic characters
         if (next) {
            letterindex = strchr(rule_letters[3], next) ;
            nindex = -1 ;
            if (letterindex) {
               nindex = letterindex - rule_letters[3] ;
            }
            while (nindex >= 0) {
               // set symmetrical
               setSymmetrical(lindex, survival, nindex, normal) ;

               // get next character
               rule++ ;
               next = *rule ;
               nindex = -1 ;
               if (next) {
                  letterindex = strchr(rule_letters[3], next) ;
                  if (letterindex) {
                     nindex = letterindex - rule_letters[3] ;
                  }
               }
            }
         }
      }
   }
}

// create the rule map from Wolfram number
void liferules::createWolframMap() {
   int i = 0 ;

   // clear the rule array
   memset(rule3x3, 0, ALL3X3) ;

   // set in the 3x3 map
   for (i = 0; i < ALL3X3; i+= 1) {
      if ((wolfram & (1 << (i & 7))) || (i & 16)) {
         rule3x3[i] = 1 ;
      }
   }
}

// create the rule map from birth and survival strings
void liferules::createRuleMap(const char *birth, const char *survival) {
   // clear the rule array
   memset(rule3x3, 0, ALL3X3) ;

   // check for totalistic rule
   if (totalistic) {
      // set the totalistic birth rule
      setTotalisticRuleFromString(birth, false) ;

      // set the totalistic surivival rule
      setTotalisticRuleFromString(survival, true) ;
   }
   else {
      // set the non-totalistic birth rule
      setRuleFromString(birth, false) ;

      // set the non-totalistic survival rule
      setRuleFromString(survival, true) ;
   }
}

// AKT: store valid rule in canonical format for getrule()
void liferules::createCanonicalName(lifealgo *algo) {
   int p = 0 ;
   int i = 0 ;
   int j = 0 ;

   // check for wolfram rule
   if (wolfram >= 0) {
      sprintf(canonrule, "W%d", wolfram) ;
      while (canonrule[p]) p++ ;
   } else {
      // output birth part
      canonrule[p++] = 'B' ;
      for (i=0; i<=neighbors; i++) {
         if (rulebits & (1 << i)) {
            canonrule[p++] = '0' + (char)i ;

            // check for non-totalistic
            if (letter_bits[i]) {
               for (j=0; j<13; j++) {
                  if (letter_bits[i] & (1 << j)) {
                     canonrule[p++] = rule_letters[3][j] ;
                  }
               }
            }

            if (neg_letter_bits[i]) {
               canonrule[p++] = '-' ;
               for (j=0; j<13; j++) {
                  if (neg_letter_bits[i] & (1 << j)) {
                     canonrule[p++] = rule_letters[3][j] ;
                  }
               }
            }
         }
      }

      // add slash
      canonrule[p++] = '/' ;

      // output survival part
      canonrule[p++] = 'S' ;
      for (i=0; i<=neighbors; i++) {
         if (rulebits & (1 << (survival_offset+i))) {
            canonrule[p++] = '0' + (char)i ;

            // check for non-totalistic
            if (letter_bits[survival_offset+i]) {
               for (j=0; j<13; j++) {
                  if (letter_bits[survival_offset+i] & (1 << j)) {
                     canonrule[p++] = rule_letters[3][j] ;
                  }
               }
            }

            if (neg_letter_bits[survival_offset+i]) {
               canonrule[p++] = '-' ;
               for (j=0; j<13; j++) {
                  if (neg_letter_bits[survival_offset+i] & (1 << j)) {
                     canonrule[p++] = rule_letters[3][j] ;
                  }
               }
            }
         }
      }

      // add neighborhood
      if (neighbormask == HEXAGONAL) canonrule[p++] = 'H' ;
      if (neighbormask == VON_NEUMANN) canonrule[p++] = 'V' ;
   }

   // check for bounded grid
   if (algo->gridwd > 0 || algo->gridht > 0) {
      // algo->setgridsize() was successfully called above, so append suffix
      const char* bounds = algo->canonicalsuffix() ;
      int i = 0 ;
      while (bounds[i]) canonrule[p++] = bounds[i++] ;
   }

   // null terminate
   canonrule[p] = 0 ;
}

// convert the 3x3 map to the 4x4 map
void liferules::convertTo4x4Map(char *which) {
   int i = 0 ;
   int v = 0 ;

   // create every possible cell combination for 4x4
   for (i = 0; i < 65536; i += 1) {
      // perform 4 lookups in the 3x3 map to create the 4x4 entry
      // 15 14 13  x       7  6  5
      // 11 10  9  x  ->  11 10  9  ->  10' x 0 0 x x
      //  7  6  5  x      15 14 13
      //  x  x  x  x
      v = rule3x3[((i & 57344) >> 13) | ((i & 3584) >> 6) | ((i & 224) << 1)] << 5 ;
     
      //  x 14 13 12       6  5  4
      //  x 10  9  8  ->  10  9  8  ->  x 9' 0 0 x x
      //  x  6  5  4      14 13 12
      //  x  x  x  x
      v |= rule3x3[((i & 28672) >> 12) | ((i & 1792) >> 5) | ((i & 112) << 2)] << 4 ;

      //  x  x  x  x
      // 11 10  9  x  ->   3  2  1  ->  x x 0 0 6' x
      //  7  6  5  x       7  6  5
      //  3  2  1  x      11 10  9
      v |= rule3x3[((i & 3584) >> 9) | ((i & 224) >> 2) | ((i & 14) << 5)] << 1 ;

      //  x  x  x  x
      //  x 10  9  8  ->   2  1  0  ->  x x 0 0 x 5' 
      //  x  6  5  4       6  5  4
      //  x  2  1  0      10  9  8
      v |= rule3x3[((i & 1792) >> 8) | ((i & 112) >> 1) | ((i & 7) << 6)] ;

      // save the entry
      which[i] = v ;
   }
}

/*
    Following routines emulate B0 rules using David Eppstein's idea to change
    the current rule depending on generation parity.
    
    If original rule has B0 but not S8:

    For even generations, whenever the original rule has a Bx or Sx, omit that 
    bit from the modified rule, and whenever the original rule is missing a
    Bx or Sx, add that bit to the modified rule.
    eg. B03/S23 => B1245678/S0145678.
    
    For odd generations, use Bx if and only if the original rule has S(8-x)
    and use Sx if and only if the original rule has B(8-x).
    eg. B03/S23 => B56/S58.
    
    If original rule has B0 and S8:
    
    Such rules don't strobe, so we just want to invert all the cells.
    The trick is to do both changes: invert the bits, and swap Bx for S(8-x).
    eg. B03/S238 => B123478/S0123467 (for ALL gens).
*/

// create the B0 (no Smax) even generation rule map
void liferules::createB0SmaxRuleMap(const char *birth, const char *survival) {
   int b = 0 ;
   int s = 0 ;
   int i = 0 ;
   char newbirth[10] ;
   char newsurvival[10] ;

   // invert neighbor counts and Bx->S(max-x) and Sx->B(max-x)
   for (i = 0 ; i <= neighbors; i++) {
      // check if the digit is in the birth part
      if (strchr(birth, '0' + i) == 0) {
         // compute Smax-x and add to survival part
         newsurvival[s++] = '0' + (neighbors - i) ;
      }

      // check if the digit is in the survival part
      if (strchr(survival, '0' + i) == 0) {
         // compute Smax-x and add to birth part
         newbirth[b++] = '0' + (neighbors - i) ;
      }
   }

   // terminate strings
   newbirth[b] = 0 ;
   newsurvival[s] = 0 ;

   // create the rule map
   createRuleMap(newbirth, newsurvival) ;
}

// create the B0 (no Smax) odd generation rule map
void liferules::createB0OddRuleMap(const char *birth, const char *survival) {
   int b = 0 ;
   int s = 0 ;
   int i = 0 ;
   char newbirth[10] ;
   char newsurvival[10] ;

   // Bx->S(max-x) and Sx->B(max-x)
   for (i = 0 ; i <= neighbors; i++) {
      // check if the digit is in the birth part
      if (strchr(birth, '0' + i) != 0) {
         // Sx->B(max-x)
         newsurvival[s++] = '0' + (neighbors - i) ;
      }

      // check if the digit is in the survival part
      if (strchr(survival, '0' + i) != 0) {
         // Bx->S(max-x)
         newbirth[b++] = '0' + (neighbors - i) ;
      }
   }

   // terminate strings
   newbirth[b] = 0 ;
   newsurvival[s] = 0 ;

   // create the rule map
   createRuleMap(newbirth, newsurvival) ;
}

// create the B0 Smax rule map
void liferules::createB0EvenRuleMap(const char *birth, const char *survival) {
   int b = 0 ;
   int s = 0 ;
   int i = 0 ;
   char newbirth[10] ;
   char newsurvival[10] ;

   // invert neighbor counts
   for (i = 0 ; i <= neighbors; i++) {
      // check if the digit is in the birth part
      if (strchr(birth, '0' + i) == 0) {
         // add to birth part
         newbirth[b++] = '0' + i ;
      }

      // check if the digit is in the survival part
      if (strchr(survival, '0' + i) == 0) {
         // add to survival part
         newsurvival[s++] = '0' + i ;
      }
   }

   // terminate strings
   newbirth[b] = 0 ;
   newsurvival[s] = 0 ;

   // create the rule map
   createRuleMap(newbirth, newsurvival) ;
}

// set rule
const char *liferules::setrule(const char *rulestring, lifealgo *algo) {
   char *r = (char *)rulestring ;
   char tidystring[MAXRULESIZE] ;  // tidy version of rule string
   char *t = (char *)tidystring ;
   char *end = r + strlen(r) ;     // end of rule string
   char len = 0 ;                  // length of tidy string
   char c ;
   char *charpos = 0 ;
   int digit ;
   int maxdigit = 0 ;              // maximum digit value found
   char *colonpos = 0 ;            // position of colon
   char *slashpos = 0 ;            // position of slash
   char *underscorepos = 0 ;       // position of underscore
   char *bpos = 0 ;                // position of b
   char *spos = 0 ;                // position of s
   int src, dst ;

   // initialise rule type
   wolfram = -1 ;
   rulebits = 0 ;
   neighbormask = MOORE ;
   neighbors = 8 ;
   totalistic = true ;
   memset(letter_bits, 0, sizeof(letter_bits)) ;
   memset(neg_letter_bits, 0, sizeof(neg_letter_bits)) ;

   // we might need to emulate B0 rule by using two different rules for odd/even gens
   alternate_rules = false ;

   // check for colon
   colonpos = strchr(r, ':') ;
   if (colonpos) {
      // only process up to the colon
      end = colonpos ;
   }

   // create lower case version of rule name without spaces
   while (r < end) {
      // get the next character and convert to lowercase
      c = tolower(*r) ;

      // process the character
      switch (c) {
      // birth
      case 'b':
         if (bpos) {
            // multiple b found
            return "Only one B allowed." ;
         }
         bpos = t ;
         *t = c ;
         t++ ;
         break ;

      // survival
      case 's':
         if (spos) {
            // multiple s found
            return "Only one S allowed." ;
         }
         spos = t ;
         *t = c ;
         t++ ;
         break ;

      // slash
      case '/':
         if (slashpos) {
            // multiple slashes found
            return "Only one slash allowed." ;
         }
         slashpos = t ;
         *t = c ;
         t++ ;
         break ;
        
      // underscore
      case '_':
         if (underscorepos) {
            // multiple underscores found
            return "Only one underscore allowed." ;
         }
         underscorepos = t ;
         *t = c ;
         t++ ;
         break ;

      // hex
      case 'h':
         if (neighbormask != MOORE || wolfram != -1) {
            // multiple neighborhoods specified
            return "Only one neighborhood allowed." ;
         }
         neighbormask = HEXAGONAL ;
         neighbors = 6 ;
         *t = c ;
         t++ ;
         break ;

      // von neumann
      case 'v':
         if (neighbormask != MOORE || wolfram != -1) {
            // multiple neighborhoods specified
            return "Only one neighborhood allowed." ;
         }
         neighbormask = VON_NEUMANN ;
         neighbors = 4 ;
         *t = c ;
         t++ ;
         break ;

      // wolfram
      case 'w':
         // check if at beginning of string
         if (t == tidystring) {
            if (neighbormask != MOORE || wolfram != -1) {
               // multiple neighborhoods specified
               return "Only one neighborhood allowed." ;
            }
            wolfram = 0 ;
         }
         else {
            // copy character
            *t = c ;
            t++ ;
            totalistic = false ;
         }
         break ;

      // other characters
      default:
         // ignore space
         if (c != ' ') {
            // check character is valid
            charpos = strchr(valid_rule_letters, c) ;
            if (charpos) {
               // copy character
               *t = c ; 
               t++ ;

               // check if totalistic (i.e. found a valid non-digit)
               digit = charpos - valid_rule_letters ;
               if (digit > 8) {
                  totalistic = false ;
               }
               else {
                  // update maximum digit found
                  if (digit > maxdigit) {
                     maxdigit = digit ;
                  }
               }
            }
            else {
               return "Bad character found." ;
            }
         }
         break ;
      }

      // next character
      r++ ;
   }

   // ensure null terminated
   *t = 0 ;

   // don't allow empty rule string
   t = tidystring ;
   if (*t == 0) {
      return "Rule cannot be empty string." ;
   }

   // can't have slash and underscore
   if (underscorepos && slashpos) {
      return "Can't have slash and underscore." ;
   }

   // underscore only valid for non-totalistic rules
   if (underscorepos && totalistic) {
      return "Underscore not valid for totalistic rules, use slash." ;
   }

   // if underscore defined then set the slash position
   if (underscorepos) {
      slashpos = underscorepos ;
   }

   // check for Wolfram
   if (wolfram == 0) {
      // parse Wolfram 1D rule
      while (*t >= '0' && *t <= '9') {
         wolfram = 10 * wolfram + *t - '0' ;
         t++ ;
      }
      if (wolfram < 0 || wolfram > 254 || wolfram & 1) {
         return "Wolfram rule must be an even number from 0 to 254." ;
      }
      if (*t) {
         return "Bad character in Wolfram rule." ;
      }
   }
   else {
      // if neighborhood specified then must be last character
      if (neighbormask != MOORE) {
         len = strlen(t) ;
         if (len) {
            c = t[len - 1] ;
            if (!((c == 'h') || (c == 'v'))) {
               return "Neighborhood must be at end of rule." ;
            }
            // remove character
            t[len - 1] = 0 ;
         }
      }

      //AKT!!! we need to allow B234 as valid (canonical is B234/S)
      
      // slash must be present, or both b and s with one at the start
      if (!(slashpos || ((bpos && spos) && (bpos == t || spos == t)))) {
         return "Rule must contain a slash or both B and S." ;
      }
   
      // digits can not be greater than the number of neighbors for the defined neighborhood
      if (maxdigit > neighbors) {
         return "Digit greater than neighborhood allows." ;
      }

      // if slash present and both b and s then one must be each side of the slash
      if (slashpos && bpos && spos) {
         if ((bpos < slashpos && spos < slashpos) || (bpos > slashpos && spos > slashpos)) {
            return "B and S must be either side of slash." ;
         }
      }

      // check if there was a slash to divide birth from survival
      if (!slashpos) {
         // determine whether b or s is first
         if (bpos < spos) {
            // skip b and cut the string using s
            bpos++ ;
            *spos = 0 ;
            spos++ ;
         }
         else {
            // skip s and cut the string using b
            spos++ ;
            *bpos = 0 ;
            bpos++ ;
         }
      }
      else {
         // slash exists so set determine which part is b and which is s
         *slashpos = 0 ;

         // check if b or s are defined
         if (bpos || spos) {
            // check for birth first
            if ((bpos && bpos < slashpos) || (spos && spos > slashpos)) {
               // birth then survival
               bpos = t ;
               spos = slashpos + 1 ;
            }
            else {
               // survival then birth
               bpos = slashpos + 1 ;
               spos = t ;
            }

            // remove b from string
            if (bpos) {
               src = 0 ;
               dst = 0 ;
               while ( (c = bpos[src++]) ) {
                  if (c != 'b') {
                     bpos[dst++] = c ;
                  }
               }
               bpos[dst] = 0 ;
            }

            // remove s from string
            if (spos) {
               src = 0 ;
               dst = 0 ;
               while ( (c = spos[src++]) ) {
                  if (c != 's') {
                     spos[dst++] = c ;
                  }
               }
               spos[dst] = 0 ;
            }
         }
         else {
            // no b or s so survival first
            spos = t ;
            bpos = slashpos + 1 ;
         }
      }

      // if not totalistic and a part exists it must start with a digit
      if (!totalistic) {
         // check birth
         c = *bpos ;
         if (c && (c <= '0' || c >= '8')) {
            return "Non-totalistic birth must start with a digit." ;
         }
         // check survival
         c = *spos ;
         if (c && (c <= '0' || c >= '8')) {
            return "Non-totalistic survival must start with a digit." ;
         }
      }
   
      // if not totalistic then neighborhood must be Moore
      if (!totalistic && neighbormask != MOORE) {
         return "Non-totalistic only supported with Moore neighborhood." ;
      }
   }

   // AKT: check for rule suffix like ":T200,100" to specify a bounded universe
   if (colonpos) {
      const char* err = algo->setgridsize(colonpos) ;
      if (err) return err ;
   } else {
      // universe is unbounded
      algo->gridwd = 0 ;
      algo->gridht = 0 ;
   }

   // create the rule map
   if (wolfram >= 0) {
      // create the 3x3 map
      createWolframMap() ;

      // save the canonical rule name
      createCanonicalName(algo) ;

      // convert to the 4x4 map
      convertTo4x4Map(rule0) ;
   }
   else {
      // generate the 3x3 map (which also creates the data for the canonical format)
      createRuleMap(bpos, spos) ;

      // save the canonical rule name
      createCanonicalName(algo) ;

      // check for B0 rules
      if (totalistic && strchr(bpos, '0')) {
          // check for Smax
          if (strchr(spos, '0' + neighbors)) {
             // B0 Smax
             createB0SmaxRuleMap(bpos, spos) ;

             // convert to the even 4x4 map
             convertTo4x4Map(rule0) ;
          }
          else {
             // set alternate rules needed
             alternate_rules = true ;

             // B0 without Smax even generation
             createB0EvenRuleMap(bpos, spos) ;

             // convert to the even 4x4 map
             convertTo4x4Map(rule0) ;

             // B0 without Smax odd generation
             createB0OddRuleMap(bpos, spos) ;

             // convert to the odd 4x4 map
             convertTo4x4Map(rule1) ;
          }
      }
      else {
         // non-B0 rule so convert 3x3 to 4x4 map
         convertTo4x4Map(rule0) ;
      }
   }

   // exit with success
   return 0 ;
}

const char* liferules::getrule() {
   return canonrule ;
}

// B3/S23 -> (1 << 3) + (1 << (9 + 2)) + (1 << (9 + 3)) = 0x1808
bool liferules::isRegularLife() {
   return (neighbormask == MOORE && totalistic && rulebits == 0x1808 && wolfram < 0) ;
}
