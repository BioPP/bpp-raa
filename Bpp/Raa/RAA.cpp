/*
 *  RAA.cpp
 *  
 *
 */


#include <Bpp/Raa/RAA.h>

extern "C" int get_ncbi_gc_number(int gc);

using namespace std;
using namespace bpp;

RAA::RAA(const string &dbname, int port, const string &server) throw (int)
{
	kw_pattern = NULL;
	current_address.div = -1;
	int error = raa_acnucopen_alt((char *)server.c_str(), port, (char *)dbname.c_str(), "Bio++", &raa_data);
	if(error) {
		throw error;
		}
}


RAA::RAA(int port, const string &server) throw(int)
{
	kw_pattern = NULL;
	int error = raa_open_socket((char *)server.c_str(), port, "Bio++", &raa_data);
	if(error) {
		throw error;
	}
}


RAA::~RAA()
{
	if(raa_data != NULL) raa_acnucclose(raa_data);
	if(kw_pattern) delete kw_pattern;
}


Sequence *RAA::getSeq_both(const string &name_or_accno, int rank, int maxlength)
{
	int length;
	char *name, *description;
	if(rank == 0) name = raa_getattributes(raa_data, name_or_accno.c_str(), &rank, &length, NULL, NULL, NULL, 
											  &description, NULL, NULL);
	else name = raa_seqrank_attributes(raa_data, rank, &length, NULL, NULL, NULL, 
								   &description, NULL, NULL);
	if(length > maxlength) return NULL;
	string sname = name;
	vector<string> comment(1, string(description) );
	string *cseq = new string(length + 1, ' ');
	if(cseq == NULL) return NULL;
	raa_gfrag(this->raa_data, rank, 1, length, (char *)cseq->data());
	cseq->resize(length);
	const Alphabet *alphab;
	if(raa_data->swissprot || raa_data->nbrf) alphab = &AlphabetTools::PROTEIN_ALPHABET;
	else alphab = &AlphabetTools::DNA_ALPHABET;
	Sequence *seq = new Sequence(sname, *cseq, comment, alphab);
	delete cseq;
	return seq;
}


Sequence *RAA::getSeq(const string &name_or_accno, int maxlength)
{
	return getSeq_both(name_or_accno, 0, maxlength);
}


Sequence *RAA::getSeq(int seqrank, int maxlength)
{
	return getSeq_both(string(""), seqrank, maxlength);
}


int RAA::getSeqFrag(int seqrank, int first, int length, string &sequence)
{
	sequence = "";
	char *p = (char *)malloc(length + 1);
	if(p == NULL) return 0;
	int l = raa_gfrag(this->raa_data, seqrank, first, length, p);
	if(l > 0) sequence = p;
	free(p);
	return l;
}


int RAA::getSeqFrag(const string &seqname, int first, int length, string &sequence)
{
	int seqrank = raa_isenum(this->raa_data, (char *)seqname.c_str());
	if(seqrank == 0) return 0;
	return this->getSeqFrag(seqrank, first, length, sequence);
}


RaaSeqAttributes *RAA::getAttributes(const string &name_or_accno)
{
	char *description, *species, *access;
	int acnuc_gc;
	RaaSeqAttributes *myattr = new RaaSeqAttributes();
	char *name = raa_getattributes(this->raa_data, name_or_accno.c_str(), &myattr->rank, &myattr->length, 
						&myattr->frame, &acnuc_gc, &access, &description, &species, NULL);
	myattr->raa = this;
	myattr->name = name;
	myattr->description = description;
	myattr->accno = access;
	myattr->species = species;
	myattr->ncbi_gc = get_ncbi_gc_number(acnuc_gc);
	return myattr;
}


RaaSeqAttributes *RAA::getAttributes(int seqrank)
{
	char *description, *species, *access;
	int acnuc_gc;
	RaaSeqAttributes *myattr = new RaaSeqAttributes();
	char *name = raa_seqrank_attributes(this->raa_data, seqrank, &myattr->length, 
								   &myattr->frame, &acnuc_gc, &access, &description, &species, NULL);
	if(name == NULL) return NULL;
	myattr->rank =seqrank;
	myattr->raa = this;
	myattr->name=  name;
	myattr->description = description;
	myattr->accno = access;
	myattr->species = species;
	myattr->ncbi_gc = get_ncbi_gc_number(acnuc_gc);
	return myattr;
}


int RAA::knownDatabases(vector<string> &name, vector<string> &description)
{
	char **cname, **cdescription;
	
	int count = raa_knowndbs(raa_data, &cname, &cdescription);
	name.resize(count);
	description.resize(count);
	for(int i = 0; i < count; i++) {
		name[i] = cname[i]; free(cname[i]);
		description[i] = cdescription[i]; free(cdescription[i]);
		}
	if(count > 0) { free(cname); free(cdescription); }
	return count;
}


int RAA::openDatabase(const string &dbname, char *(*getpasswordf)(void *), void *p)
{
	current_address.div = -1;
	return raa_opendb_pw(raa_data, (char *)dbname.c_str(), p, getpasswordf);
}


void RAA::closeDatabase()
{
	sock_fputs(this->raa_data, "acnucclose\n"); 
	read_sock(this->raa_data);
}


string RAA::getFirstAnnotLine(int seqrank)
{
	raa_seq_to_annots(raa_data, seqrank, &current_address.faddr, &current_address.div);
	char *p = raa_read_annots(raa_data, current_address.faddr, current_address.div);
	string retval(p);
	return retval;
}


string RAA::getNextAnnotLine()
{
	if(current_address.div == -1) return NULL;
	char *p = raa_next_annots(raa_data, &current_address.faddr);
	if(p != NULL) {
		string retval(p);
		return retval;
		}
	else return NULL;
}


RaaAddress RAA::getCurrentAnnotAddress()
{
	return current_address;
}


string RAA::getAnnotLineAtAddress(RaaAddress address)
{
	current_address = address;
	char *p = raa_read_annots(raa_data, current_address.faddr, current_address.div);
	string retval(p);
	return retval;
}


Sequence *RAA::translateCDS(int seqrank) throw(BadCharException)
{
	char *descript;
	char *prot = raa_translate_cds(raa_data, seqrank);
	if(prot == NULL) return NULL;
	int l = strlen(prot) - 1;
	if(l >= 0 && prot[l] == '*') prot[l] = 0;
	char *name = raa_seqrank_attributes(raa_data, seqrank, NULL, NULL, NULL, NULL, &descript, NULL, NULL);
	string *sname = new string(name);
	string *pstring = new string(prot);
	if(sname == NULL || pstring == NULL) return NULL;
	Sequence *Sprot;
	try {
		Sprot = new Sequence(*sname, *pstring, &AlphabetTools::PROTEIN_ALPHABET );
		}
	catch (BadCharException e){
		delete sname, pstring;
		throw e;
		}
	delete sname, pstring;
	vector<string> comment(1, string(descript) );
	Sprot->setComments(comment);
	return Sprot;
}


Sequence *RAA::translateCDS(const string &name) throw(BadCharException)
{
	int rank;
	rank = raa_isenum(raa_data, (char *)name.c_str());
	if(rank == 0) return NULL;
	Sequence *Sprot;
	try {
		Sprot = translateCDS(rank);
		}
	catch (BadCharException e){
		throw e;
		}
	return Sprot;
}


char RAA::translateInitCodon(int seqrank)
{
	return raa_translate_init_codon(raa_data, seqrank);
}


RaaList *RAA::processQuery(const string &query, const string &listname) throw(string *)
{
	char *message;
	int type;
	RaaList *mylist = new RaaList();
	int err = raa_proc_query(raa_data, (char *)query.c_str(), &message, (char *)listname.c_str(), &mylist->rank, NULL, 
							 NULL, &type);
	if(err) {
		delete mylist;
		string *errmess = new string(message);
		free(message);
		throw errmess;
		}
	mylist->myraa = this;
	mylist->name = listname;
	if(type=='S') mylist->type = &RaaList::LIST_SEQUENCES;
	else if(type=='K') mylist->type = &RaaList::LIST_KEYWORDS;
	else mylist->type = &RaaList::LIST_SPECIES;
	return mylist;
}


RaaList *RAA::createEmptyList(const string &listname, const string &kind) throw(int)
{
	int err, lrank;
	char line[100], type, *p, *q;
	sock_fputs(raa_data, "getemptylist&name=");
	sock_fputs(raa_data, (char *)listname.c_str());
	sock_fputs(raa_data, "\n");
	char *reponse = read_sock(raa_data);
	p = strchr(reponse, '=');
	if(p) q = strchr(p+1, '=');
	sscanf(p+1, "%d", &err);
	
	if(err != 0) {
		throw err;
		}
	sscanf(q+1, "%d", &lrank);
	if(kind == RaaList::LIST_SEQUENCES) type='S';
	else if(kind == RaaList::LIST_KEYWORDS) type='K';
	else type='E';
	sprintf(line, "setliststate&lrank=%d&type=%c\n", lrank, type);
	sock_fputs(raa_data, line);
	read_sock(raa_data);
	RaaList *mylist = new RaaList();
	mylist->myraa = this;
	mylist->rank = lrank;
	mylist->name = listname;
	mylist->type = &kind;
	return mylist;
}


int RAA::keywordPattern(const string &pattern)
{
	current_kw_match = 2;
	if(kw_pattern) delete kw_pattern;
	kw_pattern = new string(pattern);
	return raa_data->WIDTH_KW;
}


int RAA::nextMatchingKeyword(string &matching)
{
	char *keyword;
	if(current_kw_match==2) current_kw_match = raa_nextmatchkey(raa_data, 2, (char*)kw_pattern->c_str(), &keyword);
	else current_kw_match = raa_nextmatchkey(raa_data, current_kw_match, NULL, &keyword);
	if(current_kw_match) matching = keyword;
	return current_kw_match;
}


static int treeloadprogress(int percent, void *data)
{
	bool *first =(bool *)data;
	if(*first) {
		cout << "Starting species tree download\n"; 
		*first = false;
		}
	cout << '.'; fflush(stdout);
	return false;
}


RaaSpeciesTree *RAA::loadSpeciesTree(bool showprogress)
{
	bool init_load_mess = true;
	int err = raa_loadtaxonomy(raa_data, "ROOT", showprogress ? treeloadprogress : NULL, &init_load_mess, NULL, NULL); 
	if(err) return NULL;
	if( showprogress && (!init_load_mess) ) cout << "\nSpecies tree download completed\n";
	RaaSpeciesTree *tree = new RaaSpeciesTree();
	tree->raa_data = raa_data;
	tree->sp_tree = raa_data->sp_tree;
	tree->tid_to_rank = raa_data->tid_to_rank;
	tree->max_tid = raa_data->max_tid;
	tree->max_sp = raa_read_first_rec(raa_data, raa_spec); 
	return tree;
}