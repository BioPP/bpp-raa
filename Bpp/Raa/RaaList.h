#ifndef _RAALIST_H_
#define _RAALIST_H_
#include <string>

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
	std::string name;
	const std::string *type;
	int from;
	std::string elementname;
	int elementlength;
public:
	
	/**
	 * @brief Refers to a sequence list.
	 */
	static const std::string LIST_SEQUENCES;
	
	/**
	 * @brief Refers to a keyword list.
	 */
	static const std::string LIST_KEYWORDS;
	
	/**
	 * @brief Refers to a species list.
	 */
	static const std::string LIST_SPECIES;
	
	/**
	 * @brief Destructor. Should not be called after the corresponding RAA object was deleted.
     */
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
	std::string elementName() {return elementname; };
	
	/**
	 * @brief   Gives the length of the last considered list element (meaningful only for sequence lists).
	 */
	int elementLength() {return elementlength; };
	
	/**
	 * @brief   Returns the total # of residues in all sequences of list (meaningful only for sequence lists).
	 *
	 * Because this count can exceed a long integer, it is returned as a string.
	 */
	std::string residueCount();
	
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
	std::string getType() {return *type; };
	
	/**
	 * @brief   Gives the list name.
	 */
	std::string getName() {return name; };
	
	/**
	 * @brief   Modifies a sequence list by a length criterion.
	 *
	 * @param criterion		Length criterion such as "> 1000" or "<5000".
	 * @param listname		Name to be given to created list of sequences matching the length criterion.
	 * @return				A new list of sequences matching the length criterion, or NULL if error.
	 */
	RaaList *modifyByLength(const std::string &criterion, const std::string &listname);
	
	/**
	 * @brief   Modifies a sequence list by a database insertion date criterion.
	 *
	 * The database insertion date of each sequence is that of the last DT record for the embl/swissprot format, 
	 * or that of the LOCUS record for the GenBank format.
	 *
	 * @param criterion		Date criterion such as "> 1/jun/98" or "<10/DEC/2004". Year can be expressed with 2 or 4 digits.
	 * Case is not significant.
	 * @param listname		Name to be given to the created list of sequences matching the date criterion.
	 * @return				A new list of sequences matching the date criterion, or NULL if error.
	 */
	RaaList *modifyByDate(const std::string &criterion, const std::string &listname);
private:
	RaaList();
};

} //end of namespace bpp.

#endif  //_RAALIST_H_

