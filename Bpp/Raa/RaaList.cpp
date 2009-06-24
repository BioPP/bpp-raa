
#include "RAA.h"

using namespace bpp;

RaaList::RaaList()
{
	myraa = NULL;
	from = 1;
}


RaaList::~RaaList()
{
	if(myraa && myraa->raa_data) raa_releaselist(myraa->raa_data, rank);
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


