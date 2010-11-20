#!/usr/bin/env python

import sys, os
import copy
import afni_util as UTIL

g_mema_tests = [None, 'paired', 'unpaired']
g_ttpp_tests = ['-AminusB', '-BminusA']

class VarsObject(object):
   """a general class for holding variables, essentially treated as a
      struct to group variables"""

   def __init__(self, name='noname'):
      self.name = name

   def show(self, mesg='', prefix=None, pattern=None):
      print ("-- %s values in var '%s', prefix = '%s'"%(mesg,self.name,prefix))
      for key in sorted(self.__dict__.keys()):
         match = (prefix == None and pattern == None)
         if prefix:
            if key.startswith(prefix): match = 1
         if pattern:
            if key.find(pattern) >= 0: match = 1
         if match: print ("      %-15s : %s" % (key, self.__dict__[key]))

def subj_compare(subj0, subj1):
   """compare 2 Subject objects:        used for sorting a list of subjects
         if _sort_key is set, compare by key, then by sid
         else, compare by sid
   """

   cval = 0
   key = subj0._sort_key
   if key != None:
      if subj0.atrs.has_key(key): v0 = subj0.atrs[key]
      else: v0 = None
      if subj1.atrs.has_key(key): v1 = subj1.atrs[key]
      else: v1 = None
      cval = cmp(v0, v1)

   if cval != 0: return subj0._order*cval
   return subj0._order*cmp(subj0.sid, subj1.sid)
   
class Subject(object):
   """a simple subject object holding an ID, dataset name, and an
      attribute dictionary"""

   _sort_key = None             # attribute key used in compare()
   _order    = 1                # 1 for normal, -1 for reverse

   def __init__(self, sid='', dset='', atrs={}):

      self.sid   = sid          # subject ID (string)
      self.dset  = dset         # dset name (string: existing filename)
      self.atrs  = atrs         # attributes (dictionary: group, age, etc.)

      self.ddir  = '.'          # split dset name into directory and file
      self.dfile = ''

      dir, file = os.path.split(dset)   # and update them
      if dir: self.ddir = dir
      self.dfile = file

   def show(self):
      print("Subject %s, dset %s, atrs %s" % (self.sid, self.dset, self.atrs))

class SubjectList(object):
   """list of Subject elements, with attributes and command writing functions

        - can pass list of SIDs, dset names, attributes (dictionaries)
        - if any lists are past, the lengths must be equal

        - given a list of datasets (and no SIDs), try to extract SIDs
          by parsing into a regular expression

        - sort comparison should be by _sort_key (else sid), then by sid
   """

   def __init__(self, name='subject list', sid_l=None, dset_l=None, atr_l=None,
                verb=1):

      self.name      = name     # in case there are multiple lists in use
      self.subjects  = []       # list of Subject instances
      self.atrs      = []       # complete list of subject attributes
      self.disp_atrs = []       # attributes to display, in order
      self.verb      = verb     # verbose level 
      self.status    = 0        # non-zero is bad 

      if sid_l == None and dset_l == None and atr_l == None: return

      # set the length, and check for consistency
      llen = - 1
      errs = 0
      for ilist in [sid_l, dset_l, atr_l]:
         if ilist == None: continue
         if llen < 0: llen = len(ilist)         # set, or
         elif llen != len(ilist): errs += 1     # ... test

      if errs > 0:
         print '** SubjectList init requires equal lengths (or None)'
         print '   sid_l  = %s' % sid_l
         print '   dset_l = %s' % dset_l
         print '   atr_l  = %s' % atr_l
         self.status = 1
         return

      if llen == 0: return      # empty lists?

      # okay, fill the subjects and atrs fields
      sid = ''
      dname = ''
      atr = {}
      for ind in range(llen):
         if sid_l  != None: sid  = sid_l[ind]
         if dset_l != None: dset = dset_l[ind]
         if atr_l  != None: atr  = atr_l[ind]
         self.add(Subject(sid=sid, dset=dset, atrs=atr))

   def copy(self, sid_l=[], atr='', atrval=''):
      """make a full copy of the SubjectList element

         start with a complete copy
         if len(sid_l)>0, remove subjects not in list
         if atr is not '', remove subjects for which atrs[atr]!=atrval
      """
      # start with everything and then delete
      olen = len(newSL.subjects)
      newSL = copy.deepcopy(self)
      if len(sid_l) > 0:
         skeep = []
         snuke = []
         for subj in self.subjects:
            if subj.sid in sid_l: skeep.append(subj)
            else                : snuke.append(subj)
         if self.verb > 2:
            print '++ SL copy: nuking %d subjs not in sid list' % len(snuke)
         for subj in snuke: del(subj)
         newSL.subjects = skeep
      if atr != '':
         skeep = []
         snuke = []
         for subj in self.subjects:
            if subj.atrs[atr] == atrval: skeep.append(subj)
            else                       : snuke.append(subj)
         if self.verb>2: print '++ SL copy: nuking %d subjs with atr[%s] != %s'\
                               % (atr, atrval)
         for subj in snuke: del(subj)
         newSL.subjects = skeep
      if self.verb > 1: print '++ SL copy: keeping %d of %d subjects' \
                              % (len(newSL.subjects), olen)
      return newSL

   def show(self, mesg=''):
      if mesg: mstr = " (%s)" % mesg
      else:    mstr = ''
      print("SubjectList: %s%s" % (self.name, mstr))
      print("  nsubj = %d, natrs = %d, ndisp_atrs = %d" % \
               (len(self.subjects), len(self.atrs), len(self.disp_atrs)))
      print("  atrs      = %s" % self.atrs)
      print("  disp_atrs = %s" % self.disp_atrs)

      if len(self.subjects) == 0: return

      print("  subject sort key: %s" % self.subjects[0]._sort_key)
      for subj in self.subjects:
         subj.show()

   def add(self, subj):
      """add the subject to the list and update the atrs list"""

      for key in subj.atrs.keys():
         if not key in self.atrs:
            self.atrs.append(key)

      self.subjects.append(subj)

   def set_ids_from_dsets(self, prefix='', suffix='', hpad=0, tpad=0):
      """use the varying part of the dataset names for subject IDs

         If hpad > 0 or tpad > 0, expand into the head or tail of the dsets.
         If prefix or suffix is passed, apply them.
      """

      if hpad < 0 or tpad < 0:
         print '** set_ids_from_dsets: will not apply negative padding'
         return
      dlist = [s.dset.split('/')[-1] for s in self.subjects]
      slist = UTIL.list_minus_glob_form(dlist, hpad, tpad)

      if len(slist) != len(self.subjects):
         print '** failed to set SIDs from dset names\n'        \
               '   dsets = %s\n'                                \
               '   slist = %s' % (dlist, slist)
         return

      if not UTIL.vals_are_unique(slist):
         print '** cannot set IDs from dsets, labels not unique: %s' % slist
         return

      for ind, subj in enumerate(self.subjects):
         subj.sid = '%s%s%s' % (prefix, slist[ind], suffix)

   def sort(self, key=None, order=1):
      if len(self.subjects) == 0: return
      Subject._sort_key = key     # None or otherwise
      Subject._order = order      # 1 for small first, -1 for reverse
      self.subjects.sort(cmp=subj_compare)

   def make_ttestpp_command(self, set_labs=None, bsubs=None, subjlist2=None,
                             prefix=None, comp_dir=None, options=None, verb=1):
      """create a basic 3dttest++ command

         Note: this is almost identical to make_mema_command, except for use 
               of tsubs.

         ** labs, bsubs should be lists of strings, even if they are integral
            sub-bricks

         if set_labs=None, use defaults depending on # of subject lists
         else, set_labs must have 1 or 2 elements, for 1 or 2 sets of subjects
         bsubs can be of length 1 even with 2 set_labs, in that case:
            - length 1: must have subjlist2 set, and apply to it
            - length 2: no subjlist2, use with subjlist 1
         attach options after subject lists

            set_labs       - set labels (None, 1 or 2 labels)
            bsubs          - beta sub-bricks (length 1 or 2, matching labels)
            subjlist2      - second subject list for 2-sample test (want if the
                             datasets differ across sets)
            prefix         - prefix for 3dtest++ output
            comp_dir       - comparison direction, either -AminusB or -BminusA
                             (if 2 sets)
            options        - other options added to the 3dtest++ command
            verb           - verbose level

         return None on failure, command on success
      """

      if prefix == '' or prefix == None: prefix = 'ttest++_result'
      if verb > 1: print '-- make_ttest++_command: have prefix %s' % prefix

      if set_labs == None:
         if subjlist2 == None: set_labs = ['setA']
         else:                 set_labs = ['setA', 'setB']
         if verb > 2: print '-- tt++_cmd: adding default set labels'
      if bsubs == None: bsubs, tsubs = ['0'], ['1']

      indent = 10  # minimum indent: spaces to following -set option

      # want any '-AminusB' option at top of command
      if len(set_labs) > 1: copt = '%*s%s \\\n' % (indent, '', comp_dir)
      else:                 copt = ''

      # command and first set of subject files
      cmd = '3dttest++ -prefix %s \\\n'    \
            '%s'                           \
            '          -setA %s \\\n%s' %  (prefix, copt, set_labs[0],
                                self.make_ttpp_set_list(bsubs[0], indent+3))

      # maybe add second set of subject files
      if len(set_labs) > 1:
         if verb > 2: print '-- tt++_cmd: have labels for second subject set'

         # separate tests for bad test types
         if comp_dir == None:
            print '** make_tt++_cmd: missing test type, should be in %s' \
                  % g_ttpp_tests
            return      # failure
         if comp_dir not in g_ttpp_tests:
            print '** make_tt++_cmd: comp_dir (%s) must be in the list %s' \
                  % (comp_dir, g_ttpp_tests)
            return      # failure

         # note subject list and sub-brick labels
         if subjlist2 != None:
            S = subjlist2
            if verb > 2: print '-- second subject list was passed'
         else:
            S = self
            if verb > 2: print '-- no second subject list, using same list'
         if len(bsubs) > 1: b = bsubs[1]
         else:
            if S != subjlist2:
               print '** make_tt++_cmd: same subject list in comparison'
            b = bsubs[0]
         cmd += '%*s-setB %s \\\n%s' % \
                (indent, ' ', set_labs[1], S.make_ttpp_set_list(b, indent+3))

      if len(options) > 0: cmd += '%*s%s' % (indent, '', ' '.join(options))
      cmd += '\n\n'

      return cmd

   def make_mema_command(self, set_labs=None, bsubs=None, tsubs=None,
                         subjlist2=None, prefix=None, ttype=None, options=None,
                         verb=1):
      """create a basic 3dMEMA command

         ** labs, bsubs, tsubs should be lists of strings, even if they are
            integral sub-bricks

         if set_labs=None, use defaults depending on # of subject lists
         else, set_labs must have 1 or 2 elements, for 1 or 2 sets of subjects
         bsubs and tsubs can be of length 1 even with 2 set_labs, in that case:
            - length 1: must have subjlist2 set, and apply to it
            - length 2: no subjlist2, use with subjlist 1
         attach options after subject lists

            set_labs       - set labels (None, 1 or 2 labels)
            bsubs          - beta sub-bricks (length 1 or 2, matching labels)
            tsubs          - t-stat sub-bricks (as with bsubs)
            subjlist2      - second subject list for 2-sample test (want if the
                             datasets differ across sets)
            prefix         - prefix for 3dMEMA output
            'ttype'        - used for a 2-sample test, to distinguish between
                             paired and unpaired  (results is using either
                             -conditions (paired) or -group (un-))
            options        - other options added to the 3dMEMA command
            verb           - verbose level

         return None on failure, command on success
      """

      if prefix == '' or prefix == None: prefix = 'mema_result'
      if verb > 1: print '++ make_mema_cmd: have prefix %s' % prefix

      if set_labs == None:
         if subjlist2 == None: set_labs = ['setA']
         else:                 set_labs = ['setA', 'setB']
         if verb > 2: print '++ mema_cmd: adding default set labels'
      if bsubs == None: bsubs, tsubs = ['0'], ['1']

      # command and first set of subject files
      cmd = '3dMEMA -prefix %s \\\n'    \
            '       -set %s \\\n%s' %   \
             (prefix,set_labs[0],self.make_mema_set_list(bsubs[0],tsubs[0],10))

      # maybe add second set of subject files
      if len(set_labs) > 1:
         if verb > 2: print '-- mema_cmd: have labels for second subject set'
         # note subject list and sub-brick labels
         if subjlist2 != None:
            S = subjlist2
            if verb > 2: print '-- second subject list was passed'
         else:
            S = self
            if verb > 2: print '-- no second subject list, using same list'
         if len(bsubs) > 1: b, t = bsubs[1], tsubs[1]
         else:
            if S != subjlist2:
               print '** make_mema_cmd: same subject list in comparison'
            b, t = bsubs[0], tsubs[0]
         cmd += '%7s-set %s \\\n%s' % \
                (' ', set_labs[1], S.make_mema_set_list(b, t, 10))

         # either 2-sample or paired
         if ttype not in g_mema_tests:
            print "** invalid 3dMEMA test %s, not in %s" % (ttype,g_mema_tests)
            return None
         if ttype == 'paired': opt = '-conditions'
         else:                 opt = '-groups'
         cmd += '%7s%s %s %s \\\n' % ('', opt, set_labs[0], set_labs[1])

      if len(options) > 0: cmd += '%7s%s' % ('', ' '.join(options))
      cmd += '\n\n'

      return cmd

   def make_mema_set_list(self, bsub, tsub, indent=0):
      """return a multi-line string of the form:
                SID1 "dset1[bsub]"
                     "dset1[tsub]"
                SID2 "dset2[bsub]"
                     "dset2[tsub]"
                ...
         indent is the initial indentation
      """
      # note the max subject ID length
      ml = 0
      for subj in self.subjects:
         if len(subj.sid) > ml: ml = len(subj.sid)

      sstr = ''
      for subj in self.subjects:
         sstr += '%*s%s "%s[%s]" \\\n%*s "%s[%s]" \\\n' % \
                 (indent,    '', subj.sid, subj.dset, bsub,
                  indent+ml, '',           subj.dset, tsub)
      return sstr

   def make_ttpp_set_list(self, bsub, indent=0):
      """return a multi-line string of the form:
                SID1 "dset1[bsub]"
                SID2 "dset2[bsub]"
                ...
         indent is the initial indentation
      """
      # note the max subject ID length
      ml = 0
      for subj in self.subjects:
         if len(subj.sid) > ml: ml = len(subj.sid)

      sstr = ''
      for subj in self.subjects:
         sstr += '%*s%s "%s[%s]" \\\n' % (indent, '', subj.sid, subj.dset, bsub)
      return sstr

if __name__ == '__main__':
   print '** this is not a main program'
