#ifndef _RAASPECIESTREE_H_
#define _RAASPECIESTREE_H_

extern "C" {
#include "RAA_acnuc.h"
}

#include <string>

namespace bpp
{
/**
 * @brief   To work with the species tree classification of the database.
 *
 * The tree can be walked up or down starting from a string, a database species rank,
 * or an NCBI taxonID (TID) value. The species tree also contains synonymous names.
 * The root of the tree is named "ROOT" and has database rank 2.
 */


class RaaSpeciesTree
{
  friend class RAA;

public:
  /**
   * @brief Returns the database rank of a taxon identified by its name.
   *
   * @param  taxon    A taxon name. Case is not significant.
   * @param allowsynonym  If true, the return value will give the synonym's rank rather than the rank of its
   * major taxon.
   * @return The database rank of this taxon, or 0 if no such taxon exists in tree.
   */
  int findNode(const std::string& taxon, bool allowsynonym = false);

  /**
   * @brief Returns the database rank of a taxon identified by a TID.
   *
   * @param  tid     A TID value.
   * @return The database rank of this taxon, or 0 if no such taxon exists in tree.
   */
  int findNode(int tid);

  /**
   * @brief Returns the name of a taxon identified by its database rank.
   *
   * @param  rank  The database rank of a taxon.
   * @return The name of this taxon.
   */
  std::string getName(int rank);

  /**
   * @brief Returns the database rank of the parent of a taxon in the tree.
   *
   * @param  rank  The database rank of a taxon.
   * @return The database rank of the parent of this taxon in the tree, or 0 if no such taxon exists in tree.
   */
  int parent(int rank);

  /**
   * @brief Returns the TID of a taxon.
   *
   * @param  rank  The database rank of a taxon.
   * @return The TID of this taxon, or 0 if no such taxon exists in tree.
   */
  int getTid(int rank);

  /**
   * @brief Returns the number of sequences attached to a taxon or to taxa below it in the species tree.
   *
   * @param  rank  The database rank of a taxon.
   * @return The number of sequences attached to this taxon or to taxa below it, or 0 if no such taxon exists in tree.
   */
  int count(int rank);

  /**
   * @brief Returns the label of a taxon of given rank.
   *
   * Labels may contain taxon common names, genetic code information, TID values, and taxonomic
   * level information (e.g., genus, order).
   * @param  rank  The database rank of a taxon.
   * @return The label of this taxon, or "" if no such taxon or label exists in tree.
   */
  std::string label(int rank);

  /**
   * @brief Returns the rank of the first child taxon of a given taxon.
   *
   * @param  rank  The database rank of a taxon.
   * @return The rank of its first child taxon, or 0 if no child taxon exists in tree.
   */
  int firstChild(int rank);

  /**
   * @brief Allows to loop around all child taxa of a given taxon.
   *
   * @param  parent  The database rank of a taxon.
   * @param  child  The database rank of a child of taxon of rank parent.
   * @return The rank of the next child taxon of parent after taxon child, or 0 if no more child taxon exists in tree.
   */
  int nextChild(int parent, int child);

  /**
   * @brief Tells whether there is a downward path from taxon parent to taxon child in the species tree.
   *
   * @param  parent  The database rank of a taxon.
   * @param  child  The database rank of another taxon.
   * @return true iff child is below parent (possibly with intermediate nodes) in the species tree.
   */
  bool isChild(int parent, int child);

  /**
   * @brief Allows to loop around all synonymous taxa of a given taxon.
   *
   * Synonymous taxa of a taxon are chained in a closed loop, among which only one, the major taxon,
   * has a parent taxon.
   *
   * @param  rank  The database rank of a taxon.
   * @return  The rank of the next synonymous taxon, or 0 if taxon rank has no synonymous taxon.
   */
  int nextSynonym(int rank);

  /**
   * @brief Gives the major taxon among all synonyms of a given taxon.
   *
   * @param  rank  The database rank of a taxon.
   * @return  The rank of the major taxon of taxon rank (can be itself).
   */
  int getMajor(int rank);

private:
  raa_db_access* raa_data;
  raa_node** sp_tree;
  int* tid_to_rank;
  int max_tid;
  int max_sp;
};
} // namespace bpp.

#endif// _RAASPECIESTREE_H_
