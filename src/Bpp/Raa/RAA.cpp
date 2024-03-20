// SPDX-FileCopyrightText: The Bio++ Development Group
//
// SPDX-License-Identifier: CECILL-2.1

/*
 *  RAA.cpp
 *
 *
 */


#include "RAA.h"

extern "C" {
int get_ncbi_gc_number(int gc);
int sock_printf(raa_db_access* raa_current_db, const char* fmt, ...);
}

using namespace std;
using namespace bpp;

RAA::RAA(const string& dbname, int port, const string& server)
{
  kw_pattern = NULL;
  current_address.div = -1;
  int error = raa_acnucopen_alt((char*)server.c_str(), port, (char*)dbname.c_str(), (char*)"Bio++", &raa_data);
  if (error)
  {
    throw error;
  }
}


RAA::RAA(int port, const string& server)
{
  kw_pattern = NULL;
  int error = raa_open_socket((char*)server.c_str(), port, (char*)"Bio++", &raa_data);
  if (error)
  {
    throw error;
  }
}


RAA::~RAA()
{
  if (raa_data != NULL)
    raa_acnucclose(raa_data);
  if (kw_pattern)
    delete kw_pattern;
}


unique_ptr<Sequence> RAA::getSeq_both(const string& name_or_accno, int rank, int maxlength)
{
  int length;
  char* name, * description;
  if (rank == 0)
    name = raa_getattributes(raa_data, name_or_accno.c_str(), &rank, &length, NULL, NULL, NULL,
                             &description, NULL, NULL);
  else
    name = raa_seqrank_attributes(raa_data, rank, &length, NULL, NULL, NULL,
                                  &description, NULL, NULL);
  if (length > maxlength)
    return NULL;
  string sname = name;
  vector<string> comment(1, string(description) );
  string* cseq = new string(length + 1, ' ');
  if (cseq == NULL)
    return NULL;
  raa_gfrag(this->raa_data, rank, 1, length, (char*)cseq->data());
  cseq->resize(length);
  shared_ptr<const Alphabet> alphab;
  if (raa_data->swissprot || raa_data->nbrf)
    alphab = AlphabetTools::PROTEIN_ALPHABET;
  else
    alphab = AlphabetTools::DNA_ALPHABET;
  auto seq = make_unique<Sequence>(sname, *cseq, comment, alphab);
  delete cseq;
  return seq;
}


unique_ptr<Sequence> RAA::getSeq(const string& name_or_accno, int maxlength)
{
  return getSeq_both(name_or_accno, 0, maxlength);
}


unique_ptr<Sequence> RAA::getSeq(int seqrank, int maxlength)
{
  if (seqrank < 2 || seqrank > raa_data->nseq)
    return nullptr;
  return getSeq_both(string(""), seqrank, maxlength);
}


int RAA::getSeqFrag(int seqrank, int first, int length, string& sequence)
{
  if (seqrank < 2 || seqrank > raa_data->nseq)
    return 0;
  sequence = "";
  char* p = (char*)malloc(length + 1);
  if (p == NULL)
    return 0;
  int l = raa_gfrag(this->raa_data, seqrank, first, length, p);
  if (l > 0)
  {
    majuscules(p);
    sequence = p;
  }
  free(p);
  return l;
}


int RAA::getSeqFrag(const string& name_or_accno, int first, int length, string& sequence)
{
  int seqrank;
  raa_getattributes(raa_data, name_or_accno.c_str(), &seqrank, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  if (seqrank == 0)
    return 0;
  return getSeqFrag(seqrank, first, length, sequence);
}


unique_ptr<RaaSeqAttributes> RAA::getAttributes(const string& name_or_accno)
{
  char* description, * species, * access;
  int acnuc_gc, rank, length, frame;
  char* name = raa_getattributes(this->raa_data, name_or_accno.c_str(), &rank, &length,
                                 &frame, &acnuc_gc, &access, &description, &species, NULL);
  if (!name)
  {
    return nullptr;
  }
  auto myattr = make_unique<RaaSeqAttributes>();
  myattr->raa = this;
  myattr->rank = rank;
  myattr->length = length;
  myattr->frame = frame;
  myattr->name = name;
  myattr->description = description;
  myattr->accno = access;
  myattr->species = species;
  myattr->ncbi_gc = get_ncbi_gc_number(acnuc_gc);
  return myattr;
}


unique_ptr<RaaSeqAttributes> RAA::getAttributes(int seqrank)
{
  char* description, * species, * access;
  int acnuc_gc;
  if (seqrank < 2 || seqrank > raa_data->nseq)
    return nullptr;
  auto myattr = make_unique<RaaSeqAttributes>();
  char* name = raa_seqrank_attributes(this->raa_data, seqrank, &myattr->length,
                                      &myattr->frame, &acnuc_gc, &access, &description, &species, NULL);
  if (!name)
    return nullptr;
  myattr->rank = seqrank;
  myattr->raa = this;
  myattr->name =  name;
  myattr->description = description;
  myattr->accno = access;
  myattr->species = species;
  myattr->ncbi_gc = get_ncbi_gc_number(acnuc_gc);
  return myattr;
}


int RAA::knownDatabases(vector<string>& name, vector<string>& description)
{
  char** cname, ** cdescription;

  int count = raa_knowndbs(raa_data, &cname, &cdescription);
  name.resize(count);
  description.resize(count);
  for (int i = 0; i < count; i++)
  {
    name[i] = cname[i]; free(cname[i]);
    description[i] = cdescription[i]; free(cdescription[i]);
  }
  if (count > 0)
  {
    free(cname); free(cdescription);
  }
  return count;
}


int RAA::openDatabase(const string& dbname, char* (*getpasswordf)(void*), void* p)
{
  current_address.div = -1;
  return raa_opendb_pw(raa_data, (char*)dbname.c_str(), p, getpasswordf);
}


void RAA::closeDatabase()
{
  sock_fputs(this->raa_data, (char*)"acnucclose\n");
  read_sock(this->raa_data);
}


string RAA::getFirstAnnotLine(int seqrank)
{
  if (seqrank < 2 || seqrank > raa_data->nseq)
    return "";
  raa_seq_to_annots(raa_data, seqrank, &current_address.faddr, &current_address.div);
  char* p = raa_read_annots(raa_data, current_address.faddr, current_address.div);
  string retval(p);
  return retval;
}


string RAA::getNextAnnotLine()
{
  if (current_address.div == -1)
    return NULL;
  char* p = raa_next_annots(raa_data, &current_address.faddr);
  if (p != NULL)
  {
    string retval(p);
    return retval;
  }
  else
    return NULL;
}


RaaAddress RAA::getCurrentAnnotAddress()
{
  return current_address;
}


string RAA::getAnnotLineAtAddress(RaaAddress address)
{
  current_address = address;
  char* p = raa_read_annots(raa_data, current_address.faddr, current_address.div);
  string retval(p);
  return retval;
}


unique_ptr<Sequence> RAA::translateCDS(int seqrank)
{
  char* descript;
  if (seqrank < 2 || seqrank > raa_data->nseq)
    return nullptr;
  char* prot = raa_translate_cds(raa_data, seqrank);
  if (!prot)
    return nullptr;
  int l = strlen(prot) - 1;
  if (l >= 0 && prot[l] == '*')
    prot[l] = 0;
  char* name = raa_seqrank_attributes(raa_data, seqrank, NULL, NULL, NULL, NULL, &descript, NULL, NULL);
  string* sname = new string(name);
  string* pstring = new string(prot);
  if (!sname || !pstring)
    return nullptr;
  unique_ptr<Sequence> Sprot;
  try
  {
    auto alphaPtr = dynamic_pointer_cast<const Alphabet>(AlphabetTools::PROTEIN_ALPHABET);
    Sprot = make_unique<Sequence>(*sname, *pstring, alphaPtr);
  }
  catch (BadCharException& e)
  {
    delete sname;
    delete pstring;
    throw e;
  }
  delete sname;
  delete pstring;
  vector<string> comment(1, string(descript) );
  Sprot->setComments(comment);
  return Sprot;
}


unique_ptr<Sequence> RAA::translateCDS(const string& name)
{
  int rank;
  rank = raa_isenum(raa_data, (char*)name.c_str());
  if (rank == 0)
    return NULL;
  auto Sprot = translateCDS(rank);
  return Sprot;
}


char RAA::translateInitCodon(int seqrank)
{
  if (seqrank < 2 || seqrank > raa_data->nseq)
    return 0;
  return raa_translate_init_codon(raa_data, seqrank);
}


unique_ptr<RaaList> RAA::processQuery(const string& query, const string& listname)
{
  char* message;
  int type, rank;
  int err = raa_proc_query(raa_data, (char*)query.c_str(), &message, (char*)listname.c_str(), &rank, NULL,
                           NULL, &type);
  if (err)
  {
    string errmess = message;
    free(message);
    throw errmess;
  }
  unique_ptr<RaaList> mylist(new RaaList()); //Note: cannot use make_unique because of private constructor.
  mylist->myraa = this;
  mylist->rank = rank;
  mylist->name = listname;
  if (type == 'S')
    mylist->type = &RaaList::LIST_SEQUENCES;
  else if (type == 'K')
    mylist->type = &RaaList::LIST_KEYWORDS;
  else
    mylist->type = &RaaList::LIST_SPECIES;
  return mylist;
}


unique_ptr<RaaList> RAA::createEmptyList(const string& listname, const string& kind)
{
  int err, lrank;
  char type, * p = 0, * q = 0;
  sock_printf(raa_data, (char*)"getemptylist&name=%s\n", listname.c_str() );
  char* reponse = read_sock(raa_data);
  p = strchr(reponse, '=');
  if (p)
    q = strchr(p + 1, '=');
  sscanf(p + 1, "%d", &err);

  if (err != 0)
  {
    throw err;
  }
  sscanf(q + 1, "%d", &lrank);
  if (kind == RaaList::LIST_SEQUENCES)
    type = 'S';
  else if (kind == RaaList::LIST_KEYWORDS)
    type = 'K';
  else
    type = 'E';
  sock_printf(raa_data, "setliststate&lrank=%d&type=%c\n", lrank, type);
  read_sock(raa_data);
  unique_ptr<RaaList> mylist(new RaaList()); //Note: cannot use make_unique because of private constructor.
  mylist->myraa = this;
  mylist->rank = lrank;
  mylist->name = listname;
  mylist->type = &kind;
  return mylist;
}


void RAA::deleteList(RaaList* list)
{
  raa_releaselist(raa_data, list->rank);
  delete list;
}


int RAA::keywordPattern(const string& pattern)
{
  current_kw_match = 2;
  if (kw_pattern)
    delete kw_pattern;
  kw_pattern = new string(pattern);
  return raa_data->WIDTH_KW;
}


int RAA::nextMatchingKeyword(string& matching)
{
  char* keyword;
  if (current_kw_match == 2)
    current_kw_match = raa_nextmatchkey(raa_data, 2, (char*)kw_pattern->c_str(), &keyword);
  else
    current_kw_match = raa_nextmatchkey(raa_data, current_kw_match, NULL, &keyword);
  if (current_kw_match)
    matching = keyword;
  return current_kw_match;
}


static int treeloadprogress(int percent, void* data)
{
  bool* first = (bool*)data;
  if (*first)
  {
    cout << "Starting species tree download\n";
    *first = false;
  }
  cout << '.'; fflush(stdout);
  return false;
}


unique_ptr<RaaSpeciesTree> RAA::loadSpeciesTree(bool showprogress)
{
  bool init_load_mess = true;
  int err = raa_loadtaxonomy(raa_data, (char*)"ROOT", showprogress ? treeloadprogress : NULL, &init_load_mess, NULL, NULL);
  if (err)
    return nullptr;
  if (showprogress && (!init_load_mess) )
    cout << "\nSpecies tree download completed\n";
  auto tree = make_unique<RaaSpeciesTree>();
  tree->raa_data = raa_data;
  tree->sp_tree = raa_data->sp_tree;
  tree->tid_to_rank = raa_data->tid_to_rank;
  tree->max_tid = raa_data->max_tid;
  tree->max_sp = raa_read_first_rec(raa_data, raa_spec);
  return tree;
}


void RAA::freeSpeciesTree(RaaSpeciesTree* tree)
{
  int i;
  struct raa_pair* p, * q;
  for (i = 2; i <= tree->max_sp; i++)
  {
    if (raa_data->sp_tree[i] == NULL)
      continue;
    free(raa_data->sp_tree[i]->name);
    if (raa_data->sp_tree[i]->libel != NULL)
      free(raa_data->sp_tree[i]->libel);
    if (raa_data->sp_tree[i]->libel_upcase != NULL)
      free(raa_data->sp_tree[i]->libel_upcase);
    p = raa_data->sp_tree[i]->list_desc;
    while (p)
    {
      q = p->next;
      free(p);
      p = q;
    }
    free(raa_data->sp_tree[i]);
  }
  if (raa_data->tid_to_rank != NULL)
    free(raa_data->tid_to_rank);
  if (raa_data->sp_tree != NULL)
    free(raa_data->sp_tree);
  raa_data->sp_tree = NULL;
  raa_data->tid_to_rank = NULL;
  delete tree;
}


vector<string> RAA::listDirectFeatureKeys()
{
  int total, num;
  vector<string> ftkeys;

  total = raa_read_first_rec(raa_data, raa_smj);
  for (num = 2; num <= total; num++)
  {
    char* name = raa_readsmj(raa_data, num, NULL, NULL);
    if (strncmp(name, "04", 2) != 0)
      continue;
    if (strcmp(name, "04ID") == 0)
      continue;
    if (strcmp(name, "04LOCUS") == 0)
      continue;
    ftkeys.push_back(name + 2);
  }
  return ftkeys;
}


static void godownkey(raa_db_access* raa_data, unsigned int p3, vector<string>& ftkeys)
{
  unsigned next;
  int value;
  char* name;

  next = raa_readshrt(raa_data, p3, &value);
  name = raa_readkey(raa_data, abs(value), NULL, NULL, NULL, NULL);
  ftkeys.push_back(name);

  while (next != 0)
  {
    next = raa_readshrt(raa_data, next, &value);
    godownkey(raa_data, value, ftkeys);
  }
}


vector<string> RAA::listAllFeatureKeys()
{
  vector<string> ftkeys;
  int rank, pdesc;

  rank = raa_iknum(raa_data, (char*)"misc_feature", raa_key);
  raa_readkey(raa_data, rank, NULL, NULL, &pdesc, NULL);
  godownkey(raa_data, pdesc, ftkeys);
  return ftkeys;
}


unique_ptr<RaaList> RAA::getDirectFeature(const string& seqname, const string& featurekey, const string& listname, const string& matching)
{
  char query[80];
  unique_ptr<RaaList> list1;
  int matchinglist, err;
  snprintf(query, 80, "n=%s and t=%s", seqname.c_str(), featurekey.c_str());
  string squery = query;
  try
  {
    list1 = processQuery(squery, listname);
  }
  catch (string s)
  {
    return nullptr;
  }
  if (matching.empty() || list1->getCount() == 0)
    return list1;
  sock_printf(raa_data, "prep_getannots&nl=1\n%s|%s\n", raa_data->embl ? "FT" : "FEATURES", featurekey.c_str());
  char* p = read_sock(raa_data);
  if (strncmp(p, "code=0", 6) != 0)
    return nullptr;
  err = raa_modifylist(raa_data, list1->getRank(), (char*)"scan", (char*)matching.c_str(), &matchinglist, NULL, NULL);
  if (err != 0 || raa_bcount(raa_data, matchinglist) == 0)
  {
    if (err == 0)
      raa_releaselist(raa_data, matchinglist);
    return nullptr;
  }
  raa_setlistname(raa_data, matchinglist, (char*)listname.c_str());
  unique_ptr<RaaList> list2(new RaaList()); //Note: cannot use make_unique because of private constructor.
  list2->rank = matchinglist;
  list2->myraa = this;
  list2->name = listname;
  list2->type = &RaaList::LIST_SEQUENCES;
  return list2;
}


struct extract_data
{
  char line[100];
};


void* RAA::prepareGetAnyFeature(int seqrank, const string& featurekey)
{
  char* p, * line;
  int l;

  if (seqrank < 2 || seqrank > raa_data->nseq)
    throw "Incorrect first argument";
  struct extract_data* data = new struct extract_data;
  sock_printf(raa_data, "extractseqs&seqnum=%d&format=fasta&operation=feature&feature=%s&zlib=F\n", seqrank, featurekey.c_str());
  line = read_sock(raa_data);
  if (strcmp(line, "code=0") == 0)
  {
    p = read_sock(raa_data);
    strcpy(data->line, p);
    return (void*)data;
  }
  delete data;
  p = strstr(line, "message=");
  if (p == NULL)
    return NULL;
  p += 8;
  if (*p == '"')
    p++;
  l = strlen(p);
  if (p[l - 1] == '"')
    p[l - 1] = 0;
  string message = p;
  throw message;
}


unique_ptr<Sequence> RAA::getNextFeature(void* v)
{
  char* p;
  string name;
  struct extract_data* data = (struct extract_data*)v;

  while (data->line[0] == 27 /* esc */)
  {
    p = read_sock(raa_data);
    strcpy(data->line, p);
  }
  if (strcmp(data->line, "extractseqs END.") == 0)
  {
    delete data;
    return nullptr;
  }
  p = strchr(data->line, ' ');
  if (p)
    *p = 0;
  name = data->line + 1;
  p = read_sock(raa_data);
  string seqdata = "";
  while (p && strcmp(p, "extractseqs END.") != 0 && *p != 27 /* esc */  && *p != '>')
  {
    seqdata += p;
    p = read_sock(raa_data);
  }
  if (p == NULL)
    strcpy(data->line, "extractseqs END.");
  else
  {
    if (*p == 27 /* esc */)
      p = read_sock(raa_data);
    strcpy(data->line, p);
  }
  auto alphaPtr = dynamic_pointer_cast<const Alphabet>(AlphabetTools::DNA_ALPHABET);
  auto seq = make_unique<Sequence>(name, seqdata, alphaPtr);
  return seq;
}


void RAA::interruptGetAnyFeature(void* v)
{
  struct extract_data* data = (struct extract_data*)v;
  char* p;

  if (data == NULL)
    return;
  sock_fputs(raa_data, (char*)"\033" /* esc */);
  sock_flush(raa_data);
  p = data->line;
  while (strcmp(p, "extractseqs END.") != 0)
  {
    p = read_sock(raa_data);
  }
  delete data;
  /* just to consume ESC that may have arrived after extractseqs END. */
  sock_fputs(raa_data, (char*)"null_command\n");
  read_sock(raa_data);
}
