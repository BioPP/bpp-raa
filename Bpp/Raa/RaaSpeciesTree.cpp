
#include "RaaSpeciesTree.h"
#include <string>
using namespace std;
using namespace bpp;

string RaaSpeciesTree::getName(int rank)
{
	if(rank >= 2 && rank <= max_sp && sp_tree[rank] != NULL) {
		string name(sp_tree[rank]->name);
		return name;
		}
	else {
		string name("");
		return name;
		}
}


int RaaSpeciesTree::parent(int rank)
{
	if(!(rank > 2 && rank <= max_sp && sp_tree[rank] != NULL)) return 0;
	while(sp_tree[rank]->parent == NULL) rank = sp_tree[rank]->syno->rank;
	return sp_tree[rank]->parent->rank;
}


int RaaSpeciesTree::getTid(int rank)
{
	if(rank >= 2 && rank <= max_sp && sp_tree[rank] != NULL) return sp_tree[rank]->parent->tid;
	else return 0;
}


int RaaSpeciesTree::findNode(int tid)
{
	if(tid >= 0 && tid <= max_tid) return tid_to_rank[tid];
	else return 0;
}


int RaaSpeciesTree::findNode(const string &taxon, bool allowsynonym)
{
	if(taxon == string("ROOT")) return 2;
	int num = raa_iknum(raa_data, (char*)taxon.c_str(), raa_spec);
	if(num != 0 && !allowsynonym) while(sp_tree[num]->parent == NULL) num = sp_tree[num]->syno->rank;
	return num;
}


int RaaSpeciesTree::count(int rank)
{
	if(rank >= 2 && rank <= max_sp && sp_tree[rank] != NULL) return sp_tree[rank]->count;
	else return 0;
}


string RaaSpeciesTree::label(int rank)
{
	char *p;
	if(rank > 2 && rank <= max_sp && sp_tree[rank] != NULL && (p = sp_tree[rank]->libel) != NULL) {
		string retval(p);
		return retval;
		}
	else {
		string retval("");
		return retval;
		}
}


int RaaSpeciesTree::firstChild(int rank)
{
	if(rank >= 2 && rank <= max_sp && sp_tree[rank] != NULL) return sp_tree[rank]->list_desc->value->rank;
	else return 0;
}


int RaaSpeciesTree::nextChild(int rank, int child)
{
	if(!(rank >= 2 && rank <= max_sp && child > 2 && child <= max_sp && sp_tree[rank] != NULL && 
		 sp_tree[child] != NULL)) return 0;
	struct raa_pair *pair = sp_tree[rank]->list_desc;
	while(pair != NULL && pair->value->rank != child) {
		pair = pair->next;
		}
	return pair == NULL || pair->next == NULL ? 0 : pair->next->value->rank;
}


bool RaaSpeciesTree::isChild(int parent, int child)
{
	while(child != parent) {
		child = this->parent(child);
		if(child == 0) return false;
		}
	return true;
}


int RaaSpeciesTree::nextSynonym(int rank)
{
	if(!(rank >= 2 && rank <= max_sp && sp_tree[rank] != NULL)) return 0;
	struct raa_node *mynode =  sp_tree[rank]->syno;
	if(mynode == NULL) return 0;
	else return mynode->rank;
}


int RaaSpeciesTree::getMajor(int rank)
{
	if(!(rank >= 2 && rank <= max_sp && sp_tree[rank] != NULL)) return 0;
	while(sp_tree[rank]->parent == NULL) rank = sp_tree[rank]->syno->rank;
	return rank;
}
