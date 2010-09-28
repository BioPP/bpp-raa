
#include "RAA.h"

using namespace std;
using namespace bpp;

RaaList::RaaList()
{
	myraa = NULL;
	from = 1;
}


int RaaList::getCount(void)
{
	return raa_bcount(myraa->raa_data, rank);
}

const string RaaList::LIST_SEQUENCES = "sequence list";
const string RaaList::LIST_KEYWORDS = "keyword list";
const string RaaList::LIST_SPECIES = "species list";


int RaaList::firstElement()
{
	from = 1;
	return nextElement();
}


int RaaList::nextElement()
{
	char *sname;
	int seqrank = raa_nexteltinlist(myraa->raa_data, from, rank, &sname, &elementlength);
	if(seqrank) {
		elementname = sname;
		from = seqrank;
	}
	return seqrank;
}


void RaaList::setFrom(int element_rank)
{
	from = element_rank;
}


string RaaList::residueCount()
{
	char *count = raa_residuecount(myraa->raa_data, rank);
	string retval(count);
	return retval;
}


void RaaList::addElement(int elt_rank)
{
	raa_bit1(myraa->raa_data, rank, elt_rank);
}


bool RaaList::parentsOnly(void)
{
	if(*type == RaaList::LIST_SEQUENCES) return true;
	int isloc;
	raa_getliststate(myraa->raa_data, rank, &isloc, NULL, NULL); 
	return (bool)isloc;
}


void RaaList::removeElement(int elt_rank)
{
	raa_bit0(myraa->raa_data, rank, elt_rank);
}


void RaaList::zeroList(void)
{
	raa_zerolist(myraa->raa_data, rank);
}


bool RaaList::isInList(int elt_rank)
{
	return (bool)raa_btest(myraa->raa_data, rank, elt_rank);
}


RaaList *RaaList::modifyByLength(const string &criterion, const string &listname)
{
	int err, newlistrank;
	if(getType() != RaaList::LIST_SEQUENCES) return NULL;
	err = raa_modifylist(myraa->raa_data, rank, (char *)"length", (char *)criterion.c_str(), &newlistrank, NULL, NULL); 
	if(err) return NULL;
	raa_setlistname(myraa->raa_data, newlistrank, (char *)listname.c_str());
	RaaList *list = new RaaList();
	list->rank = newlistrank;
	list->myraa = myraa;
	list->name = listname;
	list->type = &RaaList::LIST_SEQUENCES;
	return list;
}


RaaList *RaaList::modifyByDate(const string &criterion, const string &listname)
{
	int err, newlistrank;
	if(getType() != RaaList::LIST_SEQUENCES) return NULL;
	err = raa_modifylist(myraa->raa_data, rank, (char *)"date", (char *)criterion.c_str(), &newlistrank, NULL, NULL); 
	if(err) return NULL;
	raa_setlistname(myraa->raa_data, newlistrank, (char *)listname.c_str());
	RaaList *list = new RaaList();
	list->rank = newlistrank;
	list->myraa = myraa;
	list->name = listname;
	list->type = &RaaList::LIST_SEQUENCES;
	return list;
}
