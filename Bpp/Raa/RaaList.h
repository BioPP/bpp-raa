#ifndef _RAALIST_H_
#define _RAALIST_H_
#include <string>

using namespace std;

namespace bpp {

class RAA;

/**
 * @brief List of sequences, keywords, or species returned by a database query.
 *
 * Instances of this class are created by database queries. Each instance contains
 * one or several elements that are often sequences, but can also be species or keywords.
 * Iteration through all elements of the list is possible.
 */
class RaaList {
	friend class RAA;
	RAA *myraa;
	int rank;
	string name;
	const string *type;
	int from;
	string elementname;
	int elementlength;
public:
	
	/**
	 * @brief Refers to a sequence list.
	 */
	static const string LIST_SEQUENCES;
	
	/**
	 * @brief Refers to a keyword list.
	 */
	static const string LIST_KEYWORDS;
	
	/**
	 * @brief Refers to a species list.
	 */
	static const string LIST_SPECIES;
	RaaList();
	~RaaList();
	
	/**
	 * @brief Gives the number of elements (often sequences) in the list.
	 */
	int getCount();
	
	/**
	 * @brief   Gives the database rank of the first element of the list.
	 */
	int firstElement();
	
	/**
	 * @brief   Gives the database rank of the list element following the last considered element.
	 */
	int nextElement();
	
	/**
	 * @brief   Sets an element of the list from which nextElement() will start.
	 *
	 * @param  element_rank   The database rank of an element of the list, typically returned by 
	 * a previous nextElement() call.
	 */
	void setFrom(int element_rank);
	
	/**
	 * @brief   Gives the name of the last considered list element.
	 */
	string elementName() {return elementname; };
	
	/**
	 * @brief   Gives the length of the last considered list element (meaningful only for sequence lists).
	 */
	int elementLength() {return elementlength; };
	
	/**
	 * @brief   Returns the total # of residues in all sequences of list (meaningful only for sequence lists).
	 *
	 * Because this count can exceed a long integer, it is returned as a string.
	 */
	string residueCount();
	
	/**
	 * @brief   Adds an element identified by its database rank to the list.
	 */
	void addElement(int rank);
	
	/**
	 * @brief   Removes an element identified by its database rank from the list.
	 */
	void removeElement(int rank);
	
	/**
	 * @brief   Tests whether an element identified by its database rank belongs to the list.
	 */
	bool isInList(int rank);
	
	/**
	 * @brief   Removes all elements from the list.
	 */
	void zeroList();
	
	/**
	 * @brief Gives the rank of the list.
	 */
	int getRank() {return rank; };
	
	/**
	 * @brief true means that list contains only parent sequences (does not contain any subsequence).
	 */
	bool parentsOnly();
	
	/**
	 * @brief Indicates whether the list contains sequences, species or keywords.
	 *
	 * @return  can be "sequence list", "species list", or "keyword list"
	 */
	string getType() {return *type; };
	
	/**
	 * @brief   Gives the list name.
	 */
	string getName() {return name; };
};

} //end of namespace bpp.

#endif  //_RAALIST_H_

