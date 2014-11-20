#ifndef _RAASEQATTRIBUTES_H_
#define _RAASEQATTRIBUTES_H_

#include <string>

namespace bpp
{
	
	
/**
 * @brief Contains various attributes of a sequence (length, name, species name, genetic code, etc...).
 *
 */


class RaaSeqAttributes {
	friend class RAA;
public:
	/**
	 * @brief    Returns the sequence name.
	 */
	std::string getName() {return name; };
	
	/**
	 * @brief    Returns the sequence length.
	 */
	int getLength() {return length; };
	
	/**
	 * @brief    Returns the sequence database rank.
	 */
	int getRank() {return rank; };
	
	/**
	 * @brief    Returns the sequence's NCBI genetic code number (1 is universal). 
   
   Meaningful only for CDS sequences. 
   See <a href=http://doua.prabi.fr/databases/acnuc_data/glossary#GCODE> a description of NCBI-defined genetic codes</a>.
	 */
	int getGeneticCode() {return ncbi_gc; };
	
	/**
	 * @brief    Returns the sequence reading frame (0, 1, or 2). Meaningful only for CDS sequences.
	 */
	int getReadingFrame() {return frame; };
	
	/**
	 * @brief    Returns the sequence species name.
	 */
	std::string getSpeciesName() {return species; };
	
	/**
	 * @brief    Returns the sequence primary accession number.
	 */
	std::string getAccessionNumber() {return accno; };
	
	/**
	 * @brief    Returns a one-line description of the sequence.
	 */
	std::string getDescription() {return description; };
protected:
	
	/**
	 * @brief    The database connection of this sequence.
	 */
	RAA *raa;
private:
	std::string name;
	int rank;
	int length;
	int ncbi_gc;
	int frame;
	std::string species;
	std::string accno;
	std::string description;
	};
	
}  // namespace bpp

#endif   //_RAASEQATTRIBUTES_H_
