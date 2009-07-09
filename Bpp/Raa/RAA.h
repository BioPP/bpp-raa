/*
 *  RAA.H
 */
#ifndef _RAA_H_
#define _RAA_H_

extern "C" {
#include "RAA_acnuc.h"
	}

#include <string>
#include <Seq/Sequence.h>
#include <Seq/alphabets>

#include "RaaList.h"
#include "RaaSpeciesTree.h"
#include "RaaSeqAttributes.h"

namespace bpp
{
	
	
/**
 * @brief Identifyies an annotation line in a database.
 */
class RaaAddress {
	friend class RAA;
	int div;
	raa_long faddr;
public:
	/**
	 * @brief Gives the rank of the database file containing this annotation line.
	 */
	int getDiv() {return div; };
	/**
	 * @brief Gives, as a 64-bit int value, the offset of this annotation line within its database file.
	 */
	raa_long getAddr() {return faddr; };
};


/**
 * @brief Network access to sequence databases (embl, genbank, swissprot, and others).
 *
 * This class provides network access to several nucleotide and protein sequence databases
 * structured for multi-criteria retrieval under the ACNUC system as described in
 * <a href=http://dx.doi.org/10.1016/j.biochi.2007.07.003>Remote access to ACNUC nucleotide 
 * and protein sequence databases at PBIL</a>.
 *
 * The list of available databases is <a href=http://pbil.univ-lyon1.fr/databases/acnuc/banques_raa.php>here</a>.
 * EMBL and GenBank are daily updated; SwissProt (it is in fact UniProt and includes SwissProt and trEMBL)
 * is updated at each partial release; EMBLwgs is updated at each full release (that is, quarterly).
 *
 * Access can be done to single sequences from their name or accession number
 * or to lists of sequences matching a query combining several retrieval criteria.
 * Any fragment of any sequence defined by coordinates or by feature table entries
 * can be retrieved. Access to sequence annotations is possible. Concurrent access 
 * to several databases is possible.
 *
 * Access is possible to database entries and also to subsequences, i.e., one or more fragments 
 * of one or more parent sequences defined by a feature table entry.
 * Subsequences are named by adding an extension (e.g., .PE1) to the name of their parent sequence.
 *
 */
class RAA {
	friend class RaaList;
public:
	
	/**
	 * @name Opening/closing database connections.
	 *
	 * @{
	 */
	
	/**
	 * @brief Direct constructor: opens a network connection to a database.
	 *
	 * @param dbname     The database name (e.g., "embl", "genbank", "swissprot").
	 * @param port       The IP port number of the server (the default value is a safe choice; make sure that no
	 * firewall blocks outbound connections on this port).
	 * @param server     The IP name of the server (the default value is a safe choice).
	 * @throw int    An error code as follows:\n
	 *               1: incorrect server name\n
	 *               2: cannot create connection with server\n
	 *               3: unknown database name\n
	 *               4: database is currently not available for remote connection\n
	 *               7: not enough memory
	 */
	RAA(const std::string &dbname, int port = 5558, const std::string &server = "pbil.univ-lyon1.fr") throw(int);

	/**
	 * @brief Direct constructor: opens a network connection to a database server, without specifying a database.
	 *
	 * Typical usage is to ask with knownDatabases() for the list of served databases, and then to open the chosen
	 * database with openDatabase().
	 *
	 * @param port       The IP port number of the server (the default value is a safe choice; make sure that no
	 * firewall blocks outbound connections on this port).
	 * @param server     The IP name of the server (the default value is a safe choice).
	 * @throw int    An error code as follows:\n
	 *               1: incorrect server name\n
	 *               2: cannot create connection with server\n
	 *               7: not enough memory
	 */
	RAA(int port=5558, const std::string &server = "pbil.univ-lyon1.fr") throw(int);

	/**
	 * @brief Destructor: closes both the database access, if any, and the network connection.
     */
	~RAA();
	
	/**
	 * @brief Opens a database from its name.
	 *
	 * @param dbname         The database name (e.g., "embl", "genbank", "swissprot").
	 * @param getpasswordf   NULL, or, for a password-protected database,  pointer to a password-providing function
	 * that returns the password as a writable static char string.
	 * @param p              NULL, or pointer to data transmitted as argument of getpasswordf.
	 * @return       0 if OK, or an error code as follows:\n
	 *               3: unknown database name\n
	 *               4: database is currently not available for remote connection\n
	 *               5: a database was previously opened on this RAA object and not closed\n
	 *               6: incorrect password for password-protected database\n
	 *               7: not enough memory
	 */
	int openDatabase(const std::string &dbname, char *(*getpasswordf)(void *) = NULL, void *p = NULL);
	
	/**
	 * @brief Closes a database connection.
	 
	 * Allows to later open another database with openDatabase() using the same RAA object.
	 */
	void closeDatabase();
	
	/**
	 * @brief Computes the list of names and descriptions of databases served by the server.
	 
	 * Typically used after creation of an RAA object without database and before openDatabase() call.
	 *
	 * @return              The number of served databases.
	 * @param name          Vector of database names. Any of these names can be used 
	 * in openDatabase() calls.
	 * @param description   Vector of database descriptions. A description can
	 * begin with "(offline)" to mean the database is currently not available.
	 */
	int knownDatabases(vector<std::string> &name, vector<std::string> &description);

	/** @} */

	/**
	 * @name Access to sequence data and annotations.
	 *
	 * @{
	 */
		
	/**
	 * @brief Returns several attributes of a sequence from its name or accession number.
	 *
	 * @param  name_or_accno   A sequence name or accession number. Case is not significant.
	 * @return  Several attributes (length, species, etc..., see: RaaSeqAttributes) of a sequence.
	 */
	RaaSeqAttributes *getAttributes(const std::string &name_or_accno);
	
	/**
	 * @brief Returns several attributes of a sequence from its database rank.
	 *
	 * @param  seqrank  The database rank of a sequence.
	 * @return  Several attributes (length, species, etc..., see: RaaSeqAttributes) of a sequence,
	 * or NULL if seqrank is not a valid database sequence rank.
	 */
	RaaSeqAttributes *getAttributes(int seqrank);
	
	/**
	 * @brief Returns a database sequence identified by name or accession number.
	 *
	 * Because nucleotide database sequences can be several megabases in length, the maxlength argument
	 * avoids unexpected huge sequence downloads.
	 *
	 * @param name_or_accno   A sequence name or accession number. Case is not significant.
	 * @param maxlength    The maximum sequence length beyond which the function returns NULL.
	 * @return          The database sequence including a one-line comment, or NULL if name_or_accno 
	 * does not match any sequence or if the sequence length exceeds maxlength.
	 */
	Sequence *getSeq(const std::string &name_or_accno, int maxlength=100000);
	
	/**
	 * @brief Returns a sequence identified by its database rank.
	 *
	 * Because nucleotide database sequences can be several megabases in length, the maxlength argument
	 * avoids unexpected huge sequence downloads.
	 *
	 * @param seqrank   The database rank of a sequence.
	 * @param maxlength    The maximum sequence length beyond which the function returns NULL.
	 * @return          The database sequence including a one-line comment, or NULL if seqrank 
	 * does not match any sequence or if the sequence length exceeds maxlength.
	 */
	Sequence *getSeq(int seqrank, int maxlength=100000);
	
	/**
	 * @brief Returns any part of a sequence identified by its database rank.
	 *
	 * @param seqrank   The database rank of a sequence.
	 * @param first     The first desired position within the sequence (1 is the smallest valid value).
	 * @param length    The desired number of residues (can be larger than what exists in the sequence).
	 * @param sequence  Filled upon return with requested sequence data.
	 * @return          The length of returned sequence data, or 0 if impossible.
	 */
	int getSeqFrag(int seqrank, int first, int length, std::string &sequence);
	
	/**
	 * @brief Returns any part of a sequence identified by its name or accession number.
	 *
	 * @param name_or_accno   The name or accession number of a sequence. Case is not significant.
	 * @param first     The first desired position within the sequence (1 is the smallest valid value).
	 * @param length    The desired number of residues (can be larger than what exists in the sequence).
	 * @param sequence  Filled upon return with requested sequence data.
	 * @return          The length of returned sequence data, or 0 if impossible.
	 */
	int getSeqFrag(const std::string &name_or_accno, int first, int length, std::string &sequence);
	
	/**
	 * @brief Returns the first annotation line of the sequence of given database rank.
	 *
	 * @param seqrank        Database rank of a sequence.
	 * @return               The first annotation line of this sequence (without terminal \\n).
	 */
	std::string getFirstAnnotLine(int seqrank);
	
	/**
	 * @brief Returns the next annotation line after that previously read, or NULL if the end of the
	 * database file was reached.
	 *
	 * @return     The next annotation line after that previously read (without terminal \\n).
	 */
	std::string getNextAnnotLine();
	
	/**
	 * @brief Returns information identifying the position of the last read annotation line.
	 *
	 * @return     Information identifying the position of the last read annotation line.
	 */
	RaaAddress getCurrentAnnotAddress();
	
	/**
	 * @brief Returns the annotation line at the given address.
	 *
	 * @param address    Information identifying the position of an annotation line 
	 * typically obtained from a previous call to getCurrentAnnotAddress().
	 * @return           The annotation line at that position (in static memory, without terminal \\n).
	 */
	std::string getAnnotLineAtAddress(RaaAddress address);
	
	/**
	 * @brief Returns the full protein translation of a protein-coding nucleotide database (sub)sequence.
	 *
	 * @param seqrank     The database rank of a protein-coding sequence. It can be either a subsequence
	 * corresponding to a CDS feature table entry, or a sequence if all of it belongs to the CDS.
	 * @return            The complete protein translation of this CDS, using the genetic code suggested
	 * by the sequence annotations and with a one-line comment, or NULL if seqrank does not match a CDS 
	 * or if not enough memory.
	 * @throw BadCharException In rare cases, the CDS may contain an internal stop codon that raises an 
	 * exception when translated to protein.
	 */
	Sequence *translateCDS(int seqrank) throw(BadCharException);//TODO add comment to Sequence
	
	/**
	 * @brief Returns the full protein translation of a protein-coding nucleotide database (sub)sequence.
	 *
	 * @param name     The name of a protein-coding sequence. It can be either a subsequence
	 * corresponding to a CDS feature table entry, or a sequence if all of it belongs to the CDS.
	 * @return            The complete protein translation of this CDS, using the genetic code suggested
	 * by the sequence annotations and with a one-line comment, or NULL if name does not match a CDS or 
	 * if not enough memory.
	 * @throw BadCharException In rare cases, the CDS may contain an internal stop codon that raises an 
	 * exception when translated to protein.
	 */
	Sequence *translateCDS(const std::string &name) throw(BadCharException);
	
	/**
	 * @brief Returns the amino acid translation of the first codon of a protein-coding (sub)sequence.
	 *
	 * @param seqrank     The database rank of a protein-coding sequence. It can be either a subsequence
	 * corresponding to a CDS feature table entry, or a sequence if all of it belongs to the CDS.
	 * @return            The amino acid corresponding to the start codon of this sequence, using the 
	 * adequate initiation-codon-specific genetic code.
	 */
	char translateInitCodon(int seqrank);
	
	/** @} */

	/**
	 * @name Creation of lists of sequences, species or keywords.
	 *
	 * @{
	 */
	
	/**
	 * @brief Returns the list of database elements (often sequences) matching a query.
	 *
	 * Query examples:\n k=ribosomal protein L14 \n   sp=felis catus and t=cds              
	 * @param query     A retrieval query following the syntax described 
	 * <a href=http://pbil.univ-lyon1.fr/databases/acnuc/cfonctions.html#QUERYLANGUAGE>here</a>.
	 * @param listname  A name to be given to the resulting list. Case is not significant. If a list with same 
	 * name already exists, it is replaced by the new list.
	 * @return          The resulting list of matching database elements.
	 * @throw string    If error, the string is a message describing the error cause.
	 */
	RaaList *processQuery(const std::string &query, const std::string &listname) throw(std::string);

	/**
	 * @brief Creates an empty list with specified name.
	 *
	 * @param listname  A name to be given to the resulting list. Case is not significant.
	 * @param kind      Nature of the resulting list. One of RaaList::LIST_SEQUENCES, RaaList::LIST_KEYWORDS,
	 * RaaList::LIST_SPECIES.
	 * @return          The resulting list, unless an exception was raised.
	 * @throw int    3: a list with same name already existed; it is left unchanged.\n
	 * 4: the server cannot create more lists.
	 */
	RaaList *createEmptyList(const std::string &listname, const std::string &kind=RaaList::LIST_SEQUENCES) throw(int);
	
	/** @} */

	
	/**
	 * @name Access to feature table-defined sequences (nucleotide databases only).
	 *
	 * @{
	 */
	
	/**
	 * @brief    Computes the list of subsequences of a given sequence corresponding to a given feature key with 
	 * optional annotation string matching.
	 *
	 * This function allows to retrieve all features of the given sequence corresponding to a given feature key
	 * and whose annotation optionally contains a given string. \n
	 * Example:\n
	 * getDirectFeature("AE005174", "tRNA", "mytrnas", "anticodon: TTG")\n
	 * retrieves all tRNA features present in the feature table of sequence AE005174 that contain the string 
	 * "anticodon: TTG" in their annotations, and puts that in a sequence list called "mytrnas". This function is
	 * meaningful with nucleotide sequence databases only (not with protein databases).
	 *
	 * @param seqname		The name of a database sequence. Case is not significant.
	 * @param featurekey	A feature key (e.g., CDS, tRNA, ncRNA) that must be directly accessible, that is, one of those 
	 * returned by listDirectFeatureKeys(). Case is not significant.
	 * @param listname		The name to give to the resulting sequence list.
	 * @param matching		An optional string required to be present in the feature's annotations. Case is not significant.
	 * @return    The list of subsequences of <i>seqname</i> that correspond to the specified feature key and, optionally, whose
	 * annotation contains the matching string, or NULL if no matching sequence exists.
	 */
	RaaList *getDirectFeature(const std::string &seqname, const std::string &featurekey, const std::string &listname, 
							  const std::string &matching = "");
	
	/**
	 * @brief    Gives all feature keys of the database that can be directly accessed.
	 *
	 * These feature keys (e.g., CDS, rRNA, tRNA) can be used with function getDirectFeature(). This function is
	 * meaningful with nucleotide sequence databases only (not with protein databases).
	 *
	 * @return    A string vector listing all feature keys of the database that can be directly accessed.
	 */
	vector<std::string> listDirectFeatureKeys();
	
	/**
	 * @brief    Gives all feature keys of the database.
	 *
	 * These feature keys (e.g., CDS, conflict, misc_feature) can be used with function prepareGetAnyFeature(). This function is
	 * meaningful with nucleotide sequence databases only (not with protein databases).
	 *
	 * @return    A string vector listing all feature keys of the database.
	 */
	vector<std::string> listAllFeatureKeys();

	
	/**
	 * @brief	Starts extraction of all features of a specified key present in the feature table of a database sequence.
	 *
	 * A database sequence can contain many instances of a given feature key in its feature table. Thus, feature extraction
	 * is done by first preparing the desired feature extraction, and by then successively extracting features 
	 * by getNextFeature() calls until no more exist in the feature table or until a call to interruptGetAnyFeature() is done.
	 * Any successful prepareGetAnyFeature() call must be followed by getNextFeature() calls until
	 * it returns NULL or by a call to interruptGetAnyFeature(); any call to other RAA member functions in between is prohibited.
	 *
	 * @param seqrank	The database rank of a sequence.
	 * @param featurekey	Any feature key (direct or not) defined in 
	 * <a href=http://www.ebi.ac.uk/embl/WebFeat/>EMBL/GenBank/DDBJ feature tables</a>. These are also returned by listAllFeatureKeys().
	 * @return	An opaque pointer to be transmitted to functions getNextFeature() or interruptGetAnyFeature().
	 * @throw string	A message indicating the cause of the error.
	 */
	void *prepareGetAnyFeature(int seqrank, const std::string &featurekey) throw(std::string);
	
	/**
	 * @brief	Successively returns features specified in a previous prepareGetAnyFeature() call.
	 *
	 * This function must be called repetitively until it returns NULL or until function interruptGetAnyFeature() is called.
	 * Features are processed in their order of appearance in the feature table.
	 * @param opaque	A pointer returned by a previous prepareGetAnyFeature() call.
	 * @return	A sequence corresponding to one of the features specified in the prepareGetAnyFeature() call, or NULL
	 * if no more such feature exists.
	 */
	Sequence *getNextFeature(void *opaque);
	
	/**
	 * @brief	Terminates a features extraction session initiated by a prepareGetAnyFeature() call before getNextFeature() call
	 * returned NULL.
	 *
	 * @param opaque	A pointer returned by a previous prepareGetAnyFeature() call.
	 */
	void interruptGetAnyFeature(void *opaque);

	/** @} */

	
	
	/**
	 * @name Browsing database species and keywords.
	 *
	 * @{
	 */
	
	/**
	 * @brief    Initializes pattern-matching in database keywords. Matching keywords are then returned by successive nextMatchingKeyword() calls.
	 *
	 * @param pattern    A pattern-matching string using @ as wildcard (example: RNA\@polymerase\@). 
	 * Case is not significant.
	 * @return   The maximum length of any database keyword.
	 */
	int keywordPattern(const std::string &pattern);
	
	/**
	 * @brief    Finds next matching keyword in database.
	 *
	 * @param matching    Set to the next matching keyword upon return.
	 * @return    The database rank of the next matching keyword, or 0 if no more matching keyword.
	 */
	int nextMatchingKeyword(std::string &matching);
	
	/**
	 * @brief Loads the database's full species tree classification.
	 *
	 * This call takes a few seconds to run on large databases because much data get downloaded from the server.
	 *
	 * @param showprogress    If true, progress information gets sent to stdout.
	 * @return   An object allowing work with the full species tree (see RaaSpeciesTree), or NULL if error.
	 */
	RaaSpeciesTree *loadSpeciesTree(bool showprogress=true);
	
	
	/** @} */

	
protected:
	raa_db_access *raa_data;
private:
	RaaAddress current_address;
	int current_kw_match;
	std::string *kw_pattern;
	Sequence *getSeq_both(const std::string &name_or_accno, int rank, int maxlength);
};

} //end of namespace bpp.

#endif // _RAA_H_

