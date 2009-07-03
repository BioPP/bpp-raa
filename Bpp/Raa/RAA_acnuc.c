#include "RAA_acnuc.h"
#include "parser.h"

#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#if defined(unix) || defined(__APPLE__)
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#endif


#define SERVER_UPDATE_MESSAGE "acnuc stop for update\n"
/* seules fctions utilisables hors de ce fichier pour ecrire sur socket */
int sock_fputs(raa_db_access *f, char *l);
int sock_flush(raa_db_access *f);

/* some prototypes */
void raa_acnucclose(raa_db_access *raa_current_db);
void list_open_dbs_remove(raa_db_access *raa_current_db);

/* needed functions */
extern char init_codon_to_aa(char *codon, int gc);
char codaa(char *codon, int code);
void *prepare_sock_gz_r(FILE *sockr);
char *z_read_sock(void *v);
int close_sock_gz_r(void *v);
void *mycalloc(int nbr, size_t taille);
char *unprotect_quotes(char *name);


/* global variables */
void (*raa_error_mess_proc)(raa_db_access *, char *) = NULL;

#define MAX_RDSHRT 50 /* max short list length read in one time */



static raa_db_access *init_raa_db_access(void);
static struct chain_void *raa_list_open_dbs = NULL;

#if defined(WIN32)

static int socket_getc(raa_db_access *raa_current_db, SOCKET f)
{
int q;

if(raa_current_db->sock_input_pos < raa_current_db->sock_input_end) {
	return *(raa_current_db->sock_input_pos++);
	}
q = recv(f, raa_current_db->sock_input, SOCKBUFS, 0);
if(q == 0 || q == SOCKET_ERROR) return EOF;
raa_current_db->sock_input_end = raa_current_db->sock_input + q;
raa_current_db->sock_input_pos = raa_current_db->sock_input;
return *(raa_current_db->sock_input_pos++);
}


static char *sock_fgets(raa_db_access  *raa_current_db, char *line, int len)
{
int c;
char *p;

p = line;
while(len > 1) {
	c = socket_getc(raa_current_db,  (SOCKET)(raa_current_db->raa_sockfdr) );
	if(c == EOF) {
		if(p == line) return NULL;
		break;
		}
	*(p++) = c;
	if(c == '\n') break;
	len--;
	}
*p = 0;
return line;
}


static int mswin_sock_flush(raa_db_access  *raa_current_db)
{
int w;
char *p;

p = raa_current_db->sock_output;
while(raa_current_db->sock_output_lbuf > 0) {
	w = send((SOCKET)raa_current_db->raa_sockfdw, p, raa_current_db->sock_output_lbuf, 0);
	raa_current_db->sock_output_lbuf -= w;
	p += w;
	}
return 0;
}


int sock_flush(raa_db_access  *raa_current_db)
{
if(raa_current_db == NULL) return 0;
return mswin_sock_flush(raa_current_db);
}


int sock_fputs(raa_db_access  *raa_current_db, char *s)
{
int l, r;

if(raa_current_db == NULL) return 0;
l = strlen(s);
while(raa_current_db->sock_output_lbuf + l > SOCKBUFS) {
	r = SOCKBUFS - raa_current_db->sock_output_lbuf;
	memcpy(raa_current_db->sock_output + raa_current_db->sock_output_lbuf, s, r);
	raa_current_db->sock_output_lbuf += r;
	l -= r;
	s += r;
	mswin_sock_flush(raa_current_db);
	}
if(l > 0) {
	memcpy(raa_current_db->sock_output + raa_current_db->sock_output_lbuf, s, l);
	raa_current_db->sock_output_lbuf += l;
	}
return 0;
}



#else

int sock_fputs(raa_db_access  *raa_current_db, char *s)
{
if(raa_current_db == NULL) return EOF;
return fputs(s, raa_current_db->raa_sockfdw);
}

int sock_flush(raa_db_access  *raa_current_db)
{
if(raa_current_db == NULL) return EOF;
return fflush(raa_current_db->raa_sockfdw);
}

#endif /* WIN32 */


int sock_printf(raa_db_access  *raa_current_db, const char *fmt, ...)
{
va_list ap;
int retval;
static char buffer[1000];

va_start(ap, fmt);
vsprintf(buffer, fmt, ap);
retval = sock_fputs(raa_current_db, buffer);
va_end(ap);
return retval;
}



/******************************************************************/
/* lit une ligne au plus de la socket et transfere le resultat dans une chaine char *   */

static char *read_sock_tell(raa_db_access *raa_current_db, int *wascompleteline) {
#define RSOCKBUFS 5000
  int lnbuf, isfull;  
  char *p ;
  static char buffer[RSOCKBUFS];
  static int was_here = FALSE;
  
  if(raa_current_db == NULL || was_here) return NULL;
  sock_flush(raa_current_db); /* tres important */
  isfull = FALSE;
#if defined(WIN32)
  p = sock_fgets(raa_current_db, buffer, RSOCKBUFS);
#else
  p = fgets(buffer, RSOCKBUFS, raa_current_db->raa_sockfdr);
#endif
  if(p == NULL || strcmp(p, SERVER_UPDATE_MESSAGE) == 0) {
	if(!was_here) {
		was_here = TRUE;
		*buffer = 0;
		if(raa_current_db != NULL && raa_current_db->dbname != NULL) {
			sprintf(buffer, "%s: ", raa_current_db->dbname);
			}
		strcat(buffer, ( p == NULL ?
  			"Error: connection to acnuc server is down. Please try again."
  			:
			"Error: acnuc server is down for database update. Please try again later." )
			);
		if(raa_error_mess_proc == NULL) {
			fprintf(stderr, "%s: %s\n", raa_current_db->dbname, buffer);
			raa_acnucclose(raa_current_db);
			exit(ERREUR);
			}
		else (*raa_error_mess_proc)(raa_current_db, buffer);/*this function sd call raa_acnucclose*/
		was_here = FALSE;
		}
	return NULL;
	}
	
  was_here = FALSE;
  lnbuf = strlen(buffer);
  p = buffer + lnbuf - 1;
  if(*p ==  '\n') isfull = TRUE;
  while(p >= buffer && (*p ==  '\n' || *p == '\r') ) *(p--) = 0;
  if(wascompleteline != NULL) *wascompleteline = isfull;
  return buffer; 
}


char *read_sock(raa_db_access *raa_current_db) /* lit une ligne entiere, rend ligne dans memoire privee */
{
int wasfull, l2, l = 0;
char *p;
static char *reponse = NULL;
static int lr = 0;

do	{
	p = read_sock_tell(raa_current_db, &wasfull);
	if(p == NULL) return NULL;
	l2 = strlen(p);
	if(l + l2 +  1 > lr) {
		lr = l + l2 +  1;
		reponse = (char *)realloc(reponse, lr);
		}
	memcpy(reponse + l, p, l2);
	l += l2;
	}
while(! wasfull);
reponse[l] = 0;
return reponse;
}


char *read_sock_timeout(raa_db_access *raa_current_db, int timeout_ms)
{
fd_set readfds;
struct timeval tout;
int err;

#if defined(WIN32)
SOCKET fd;
if(raa_current_db == NULL) return NULL;
fd = (SOCKET)(raa_current_db->raa_sockfdr);
#else
int fd;
if(raa_current_db == NULL) return NULL;
fd = fileno(raa_current_db->raa_sockfdr);
#endif
FD_ZERO(&readfds);
FD_SET(fd, &readfds);
tout.tv_sec = timeout_ms / 1000; tout.tv_usec = 1000*(timeout_ms % 1000);
err = select(fd + 1, &readfds, NULL, NULL, &tout);
if(err > 0 && FD_ISSET(fd, &readfds) ) {
	return read_sock(raa_current_db);
	}
return NULL;
}

enum {errservname=1, /* bad server name */
cantopensocket,  /* 2 error opening socket */
unknowndb,  /* 3 not in list of known dbs */
unavailabledb,  /* 4 db is currently unavailable */
dbisopen,  /* 5 a db is already open and was not closed */
badpsswd,  /* 6 bad password for protected db */
nomemory,  /* 7 not enough memory */
badracnuc,  /* 8 enviroment variables racnuc or acnuc undefined or inadequate */
nosocket /* 9 no socket was opened yet */
};

int raa_acnucopen (char *clientid, raa_db_access **psock) 
/* opens the acnuc db using the environment variable racnuc, or, if undefined, acnuc,
that should be defined to an url of the form
raa://pbil.univ-lyon1.fr:5558/embl
clientid: NULL or a string identifying the client
*/
{
char *serveurName, *db_name, *p;
int port, err;

if( (p = getenv("racnuc")) == NULL) p = getenv("acnuc");
if(p == NULL) return badracnuc;
err = raa_decode_address(p, &serveurName, &port, &db_name);
if(err) return badracnuc;
err = raa_acnucopen_alt (serveurName,  port, db_name, clientid, psock);
return err;
}


int raa_acnucopen_alt (char *serveurName, int port, char *db_name, char *clientid, raa_db_access **p) 
/*
clientid: NULL or a string identifying the client
*/
{
int err;
 
 err = raa_open_socket(serveurName, port, clientid, p);
 if(err != 0) return err;
 err = raa_opendb(*p, db_name);
 if(err != 0) {
  	list_open_dbs_remove(*p);
 	}
 return err;
 }


int raa_open_socket(char *serveurName, int port, char *clientid, raa_db_access **psock)
/*
clientid: NULL or a string identifying the client
*/
{
  raa_db_access *raa_current_db;
  struct hostent *he;
  struct sockaddr_in their_addr;
  char *reponse;
  int err;
#ifdef WIN32
WSADATA mywsadata;
SOCKET raa_snum;
#else
int raa_snum;
#endif

raa_current_db = init_raa_db_access();
if(raa_current_db == NULL) return nomemory; /* not enough memory */
  /* création de la socket */
#ifdef WIN32
  err = WSAStartup(MAKEWORD(2,2), &mywsadata); /* indispensable avant utilisation socket */
  if (err == 0) raa_snum = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (GROUP)0, 0);
  if (err != 0 || raa_snum == INVALID_SOCKET) {
  	list_open_dbs_remove(raa_current_db);
  	return cantopensocket;
  	}
#else
  raa_snum = socket(AF_INET, SOCK_STREAM, 0);
  if (raa_snum == -1) {
  	list_open_dbs_remove(raa_current_db);
  	return cantopensocket;
  	}
#endif
/* création de deux flux type FILE * */
#if defined(WIN32)
raa_current_db->raa_sockfdr = (FILE *)raa_snum;
raa_current_db->raa_sockfdw = (FILE *)raa_snum;
#else
raa_current_db->raa_sockfdr = fdopen(raa_snum,"r");
raa_current_db->raa_sockfdw = fdopen(raa_snum,"a");
#endif

  if ((he = gethostbyname(serveurName)) == NULL) {
  	list_open_dbs_remove(raa_current_db);
  	return errservname;
  	}
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(port);
  their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(their_addr.sin_zero, '\0', 8);
  
 err = connect(raa_snum, (struct sockaddr *)&their_addr, sizeof(struct sockaddr) );
 if (err != 0) {
  	list_open_dbs_remove(raa_current_db);
 	return cantopensocket;
 	}
 reponse=read_sock(raa_current_db);
 if(reponse == NULL) {
  	list_open_dbs_remove(raa_current_db);
 	return cantopensocket;
 	}
 if(clientid != NULL) {
 	sock_printf(raa_current_db, "clientid&id=\"%s\"\n", clientid);
	reponse=read_sock(raa_current_db);
	if(reponse == NULL) {
  		list_open_dbs_remove(raa_current_db);
		return cantopensocket;
		}
	}
*psock = raa_current_db;
 return 0;
}


extern void raa_MD5String (char *in_string, char out_digest[33]);

int raa_opendb_pw(raa_db_access *raa_current_db, char *db_name, void *ptr, char *(*getpasswordf)(void *) )
/*
getpasswordf: pointer to function that gets called if a password is needed
ptr: pointer to data passed to the getpasswordf function
return values :
    3    not in list of known dbs
    4    db is unavailable
    5    a db is already open
    6    failed password-based authorization

*/
{
Reponse *rep;
char *reponse, *code, *p, *challenge;
int codret, totspecs, totkeys;

if(raa_current_db == NULL) return nosocket;
 sock_printf(raa_current_db, "acnucopen&db=%s\n", db_name);
 reponse=read_sock(raa_current_db);
 if(reponse == NULL) return nosocket;
 rep=initreponse();
 parse(reponse,rep);
 code=val(rep,"code");
 codret=atoi(code);
 free(code);
 if(codret == 6) {
	static char reply[33], tmp[300];
	char *password = NULL;
	clear_reponse(rep);
	if(getpasswordf != NULL) password = getpasswordf(ptr);
	if(password == NULL) return badpsswd;
	challenge = reponse + 17;
	/* reply = MD5 of challenge:db_name:MD5(password) */
	raa_MD5String(password, reply);
	memset(password, 0, strlen(password));
	sprintf(tmp, "%s:%s:%s", challenge, db_name, reply);
	raa_MD5String(tmp, reply);
	memset(tmp, 0, strlen(tmp));
	sock_printf(raa_current_db, "reply=%s\n", reply);
	reponse = read_sock(raa_current_db);
	rep=initreponse();
	parse(reponse,rep);
	code=val(rep,"code");
	codret=atoi(code);
	free(code);
	}
if (codret != 0) { 
	clear_reponse(rep);
 	return codret;
 	} 
 
p = val(rep,"type");
raa_current_db->dbname = strdup(db_name);
raa_current_db->genbank = raa_current_db->embl = raa_current_db->swissprot = 
	raa_current_db->nbrf = FALSE;
if(p != NULL) {
 	if(strcmp(p, "GENBANK") == 0) raa_current_db->genbank = TRUE;
 	else if(strcmp(p, "EMBL") == 0) raa_current_db->embl = TRUE;
 	else if(strcmp(p, "SWISSPROT") == 0) raa_current_db->swissprot = TRUE;
 	else if(strcmp(p, "NBRF") == 0) raa_current_db->nbrf = TRUE;
 	free(p);
 	}
p = val(rep,"totseqs");
raa_current_db->nseq = atoi(p);
free(p);
p = val(rep,"totspecs");
totspecs = atoi(p);
free(p);
p = val(rep,"totkeys");
totkeys = atoi(p);
free(p);
raa_current_db->maxa=(totspecs > totkeys ? totspecs : totkeys);
raa_current_db->longa=(raa_current_db->maxa-1)/(8 * sizeof(int))+1;

/* default values useful if talking to old server */
	raa_current_db->WIDTH_SMJ = 20; 
	raa_current_db->L_MNEMO = 16; 
	raa_current_db->WIDTH_BIB = 40;
	raa_current_db->WIDTH_AUT = 20;
	raa_current_db->WIDTH_SP = 40;
	raa_current_db->WIDTH_KW = 40;
	raa_current_db->lrtxt = 60;
	raa_current_db->SUBINLNG=63;
	raa_current_db->ACC_LENGTH=13; /* conservative value */
p = val(rep,"L_MNEMO");
if(p != NULL) { raa_current_db->L_MNEMO = atoi(p); free(p); }
p = val(rep,"WIDTH_SP");
if(p != NULL) { raa_current_db->WIDTH_SP = atoi(p); free(p); }
p = val(rep,"WIDTH_KW");
if(p != NULL) { raa_current_db->WIDTH_KW = atoi(p); free(p); }
p = val(rep,"WIDTH_AUT");
if(p != NULL) { raa_current_db->WIDTH_AUT = atoi(p); free(p); }
p = val(rep,"WIDTH_BIB");
if(p != NULL) { raa_current_db->WIDTH_BIB = atoi(p); free(p); }
p = val(rep,"WIDTH_SMJ");
if(p != NULL) { raa_current_db->WIDTH_SMJ = atoi(p); free(p); }
p = val(rep,"ACC_LENGTH");
if(p != NULL) { raa_current_db->ACC_LENGTH = atoi(p); free(p); }
p = val(rep,"lrtxt");
if(p != NULL) { raa_current_db->lrtxt = atoi(p); free(p); }
p = val(rep,"SUBINLNG");
if(p != NULL) { 
	raa_current_db->SUBINLNG = atoi(p); 
	free(p); 
	raa_current_db->rlng_buffer = (struct rlng *)realloc(raa_current_db->rlng_buffer, 
		(raa_current_db->SUBINLNG + 1) * sizeof(int));
	}

clear_reponse(rep);
return 0;
}


int raa_opendb(raa_db_access *raa_current_db, char *db_name)
{
return raa_opendb_pw(raa_current_db, db_name, NULL, NULL);
}


int raa_decode_address(char *url, char **p_ip_name, int *socket, char **p_remote_db)
/* decode syntax such as raa://pbil.univ-lyon1.fr:5557/embl
return !=0 if error
*/
{
char *p, *q;
static char ip_name[200];
static char remote_db[100];

p = url;
if(p == NULL) return 1;
if( (q = strstr(url, "://") ) != NULL ) p = q + 3;
q = strchr(p, ':');
if(q == NULL) return 1;
memcpy(ip_name, p, q - p); ip_name[q - p] = 0;
if(p_ip_name != NULL) *p_ip_name = ip_name;
if(socket != NULL) *socket = atoi(q+1);
if(p_remote_db == NULL) return 0;
q = strchr(p, '/');
if(q != NULL) {
	q++;
	while(*q == ' ') q++;
	if(*q == 0) *p_remote_db = NULL;
	else 	{
		strcpy(remote_db, q);
		*p_remote_db = remote_db;
		}
	}
else *p_remote_db = NULL;
return 0;
}


#define maxSUBINLNG 512
static raa_db_access *init_raa_db_access(void)
{
raa_db_access *data;
struct chain_void *elt;

data = (raa_db_access *)calloc(1, sizeof(raa_db_access));
elt = (struct chain_void *)malloc(sizeof(struct chain_void));
if(data == NULL || elt == NULL) return NULL;
data->rlng_buffer = (struct rlng *)malloc((maxSUBINLNG+1)*sizeof(int));
if(data->rlng_buffer == NULL) return NULL;
/* ajouter un elt a liste ds db ouvertes */
elt->data = data;
elt->next = raa_list_open_dbs;
raa_list_open_dbs = elt;

/* initialiser les champs non nuls */
data->gfrag_data.l_nseq_buf = INT_MAX;
data->nextelt_data.current_rank = -1;
data->nextelt_data.previous = -2;
data->readshrt_data.shrt_begin = S_BUF_SHRT - 1;
#ifdef WIN32
data->sock_input_pos = data->sock_input; data->sock_input_end = data->sock_input;
#endif
return data;
}


static int fill_gfrag_buf(raa_db_access *raa_current_db, int nsub, int first)
{
char *p, *line;
int lu, l, length, wasfull;

sock_printf(raa_current_db,"gfrag&number=%d&start=%d&length=%d\n", nsub, first, RAA_GFRAG_BSIZE);
/* retour:  length=xx&...the seq...\n */
line = read_sock_tell(raa_current_db, &wasfull);
if(line == NULL) return 0;
if(strncmp(line, "length=", 7) != 0 || (p = strchr(line, '&')) == NULL ) {
	return 0;
	}
length = atoi(line + 7);
lu = strlen(++p);
memcpy(raa_current_db->gfrag_data.buffer, p, lu);
while(! wasfull) {
	line = read_sock_tell(raa_current_db, &wasfull);
	if(line == NULL) break;
	l = strlen(line);
	if(lu+l <= RAA_GFRAG_BSIZE) memcpy(raa_current_db->gfrag_data.buffer + lu, line, l);
	lu += l;
	}
raa_current_db->gfrag_data.buffer[lu] = 0;
return lu;
}



int raa_gfrag(raa_db_access *raa_current_db, int nsub, int first, int lfrag, char *dseq) 
{
int lu, piece;
char *debut;

if(raa_current_db == NULL) return 0;
if(raa_current_db->gfrag_data.lbuf == 0 || nsub != raa_current_db->gfrag_data.nseq_buf || 
		first >= raa_current_db->gfrag_data.first_buf + raa_current_db->gfrag_data.lbuf || 
		first < raa_current_db->gfrag_data.first_buf) {
	if( nsub == raa_current_db->gfrag_data.nseq_buf && first > raa_current_db->gfrag_data.l_nseq_buf) 
		lu = 0;
	else
		lu = fill_gfrag_buf(raa_current_db, nsub, first);	
	
	if(lu == 0) return 0;
	raa_current_db->gfrag_data.lbuf = lu;
	if(raa_current_db->gfrag_data.lbuf < RAA_GFRAG_BSIZE) 
		raa_current_db->gfrag_data.l_nseq_buf = first + raa_current_db->gfrag_data.lbuf - 1;
	else raa_current_db->gfrag_data.l_nseq_buf = INT_MAX;
	raa_current_db->gfrag_data.first_buf = first;
	raa_current_db->gfrag_data.nseq_buf = nsub;
	}
debut = raa_current_db->gfrag_data.buffer + (first - raa_current_db->gfrag_data.first_buf);
lu = raa_current_db->gfrag_data.lbuf + raa_current_db->gfrag_data.first_buf-1 - first+1;
if( lu > lfrag) lu = lfrag;
memcpy(dseq, debut, lu);
while(lfrag > lu) {
	piece = raa_gfrag(raa_current_db, nsub, first + lu, lfrag - lu, dseq + lu);
	if(piece == 0) break;
	lu += piece;
	}
dseq[lu] = 0;
return lu;
}


void list_open_dbs_remove(raa_db_access *raa_current_db)
/* enlever de liste des db ouvertes */
{
struct chain_void *elt, *elt2;
int i;

if(raa_current_db == NULL) return; /* should not happen */
if(raa_current_db->dbname != NULL) free(raa_current_db->dbname);
if(raa_current_db->rlng_buffer != NULL) free(raa_current_db->rlng_buffer);
if(raa_current_db->readsub_data.name != NULL) free(raa_current_db->readsub_data.name);
for(i = 0; i < raa_current_db->annot_data.annotcount; i++) 
	free(raa_current_db->annot_data.annotline[i]);
for(i = 0; i < BLOCK_ELTS_IN_LIST; i++) if(raa_current_db->nextelt_data.tabname[i] != NULL) 
			free(raa_current_db->nextelt_data.tabname[i]);
if(raa_current_db->readsmj_data.lastrec > 0) {
		free(raa_current_db->readsmj_data.plongs);
		for(i=2; i <= raa_current_db->readsmj_data.lastrec; i++) {
			if(raa_current_db->readsmj_data.names[i] != NULL) free(raa_current_db->readsmj_data.names[i]);
			if(raa_current_db->readsmj_data.libels[i] != NULL) free(raa_current_db->readsmj_data.libels[i]);
			}
		free(raa_current_db->readsmj_data.names); free(raa_current_db->readsmj_data.libels);
		raa_current_db->readsmj_data.lastrec = 0;
		}

if(raa_list_open_dbs == NULL) return; /* should not happen */
if(raa_list_open_dbs->data == raa_current_db) {
	elt = raa_list_open_dbs;
	raa_list_open_dbs = raa_list_open_dbs->next;
	free(elt);
	}
else {
	elt = raa_list_open_dbs;
	while(elt->next != NULL) {
		if(elt->next->data == raa_current_db) {
			elt2 = elt->next;
			elt->next = elt2->next;
			free(elt2);
			break;
			}
		elt = elt->next;
		}
	}
free(raa_current_db);
}


static void raa_free_sp_tree(raa_node *pere)
{
raa_node *next, *tmp_n;
struct raa_pair *liste, *tmp_p;

liste = pere->list_desc;
while(liste != NULL) {
	tmp_p = liste->next;
	raa_free_sp_tree(liste->value);
	free(liste);
	liste = tmp_p;
	}
next = pere->syno;
while(next != NULL && next != pere) {
	tmp_n = next->syno;
	free(next->name);
	free(next);
	next = tmp_n;
	}
free(pere->name);
if(pere->libel != NULL) free(pere->libel);
if(pere->libel_upcase != NULL) free(pere->libel_upcase);
free(pere);
}


void raa_acnucclose(raa_db_access *raa_current_db) {
char *reponse;
int i;

if(raa_current_db == NULL) return;

sock_fputs(raa_current_db, "acnucclose\n"); 
  
reponse=read_sock(raa_current_db);
if(reponse != NULL) {
	sock_fputs(raa_current_db, "quit\n"); 
	sock_flush(raa_current_db);
	}
#ifdef WIN32
  closesocket( (SOCKET) (raa_current_db->raa_sockfdw) );
#else
  fclose(raa_current_db->raa_sockfdr);
  fclose(raa_current_db->raa_sockfdw);
#endif

if(raa_current_db->tot_key_annots > 0) {
	for(i = 0; i < raa_current_db->tot_key_annots; i++) {
		free(raa_current_db->key_annots[i]);
		free(raa_current_db->key_annots_min[i]);
		}
	free(raa_current_db->key_annots);
	free(raa_current_db->key_annots_min);
	free(raa_current_db->want_key_annots);
	raa_current_db->tot_key_annots = 0;
	}
if(raa_current_db->tid_to_rank != NULL) free(raa_current_db->tid_to_rank);
if(raa_current_db->sp_tree != NULL) {
	raa_free_sp_tree(raa_current_db->sp_tree[2]);
	free(raa_current_db->sp_tree);
	}

list_open_dbs_remove(raa_current_db);
}


int raa_prep_acnuc_query(raa_db_access *raa_current_db) {
/* returns -1 if error or number of free bit lists
*/
  char *reponse, *p, *q, *annotlines;
  int codret, i;
  Reponse *rep;

if(raa_current_db == NULL) return -1;
  rep = initreponse();

  sock_fputs(raa_current_db, "countfreelists\n");
  reponse=read_sock(raa_current_db);
if(reponse == NULL) return -1;
  parse(reponse,rep);
  reponse=val(rep,"code");
  if(reponse == NULL) return -1;
  codret=atoi(reponse);
  free(reponse);
  if(codret != 0) return -1;
  reponse=val(rep,"free");
  if(reponse != NULL) {
	codret = atoi(reponse);
  	free(reponse);
  	}
annotlines = val(rep, "annotlines");
raa_current_db->tot_key_annots = 0;
if(annotlines != NULL) {
	p = annotlines - 1;
	do 	{
		p++;
		raa_current_db->tot_key_annots++;
		}
	while((p = strchr(p, '|')) != NULL);
	raa_current_db->want_key_annots = (unsigned char *)malloc(raa_current_db->tot_key_annots * 
		sizeof(unsigned char));
	raa_current_db->key_annots = (char **)malloc(raa_current_db->tot_key_annots * sizeof(char *));
	raa_current_db->key_annots_min = (char **)malloc(raa_current_db->tot_key_annots * sizeof(char *));
	p = annotlines;
	for(i = 0; i < raa_current_db->tot_key_annots; i++) {
		q = strchr(p, '|');
		if(q == NULL) q = p + strlen(p);
		raa_current_db->key_annots[i] = malloc(q - p + 1);
		raa_current_db->key_annots_min[i] = malloc(q - p + 1);
		memcpy(raa_current_db->key_annots_min[i], p, q - p);
		raa_current_db->key_annots_min[i][q - p] = 0;
		strcpy(raa_current_db->key_annots[i], raa_current_db->key_annots_min[i]);
		majuscules(raa_current_db->key_annots[i]);
		compact(raa_current_db->key_annots[i]);
		p = q + 1;
		}
	free(annotlines);
	}
else codret = -1;
  clear_reponse(rep);
  return codret;
}


static char *raa_requete_remote_file(raa_db_access *raa_current_db, char *oldrequete, int **plist, char **); 

int raa_proc_query(raa_db_access *raa_current_db, char *requete, char **message, 
	char *nomliste, int *numlist, int *count, int *locus, int *type) {
  char *reponse, *code, *numlistchr, *countchr, *locuschr, *typechr, *badfname;
  int codret, *tmp_blists;
  Reponse *rep;
  
if(raa_current_db == NULL) return -1;
  requete = raa_requete_remote_file(raa_current_db, requete, &tmp_blists, &badfname);
  if(requete == NULL) {
  	if(message != NULL) {
  		static char fmt[] = "problem accessing file: %s";
  		*message = (char *)malloc(strlen(fmt) + strlen(badfname) + 1);
  		sprintf(*message, fmt, badfname);
  		}
  	return 1;
  	}
  sock_printf(raa_current_db,"proc_query&query=\"%s\"&name=\"%s\"\n", 
  	protect_quotes(requete), nomliste);
  free(requete);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) {
  	 if(message != NULL) *message = strdup("connection with server is down");
	 return -1;
	 }
  rep=initreponse();
  parse(reponse,rep);
  code=val(rep,"code");
  codret=atoi(code);
  if(codret == 0) {
    	numlistchr=val(rep,"lrank");
 	 *numlist=atoi(numlistchr);
  	countchr=val(rep,"count");
 	if(count != NULL) *count=atoi(countchr);
  	typechr=val(rep,"type");
  	locuschr=val(rep,"locus");
  	if(type != NULL) {
  		if (strcmp(typechr,"SQ")==0) *type='S';
  		else if (strcmp(typechr,"KW")==0) *type='K';
  		else if (strcmp(typechr,"SP")==0) *type='E';
  		}
  	if(locus != NULL) *locus = strcmp(locuschr,"T") == 0;
	  free(countchr);
	  free(locuschr);
	  free(typechr);
	  free(numlistchr);
  	}
 else if(message != NULL) {
 	*message = val(rep, "message");
 	}
  free(code);
  clear_reponse(rep);
  if(tmp_blists != NULL) {
 	 while(*tmp_blists != 0) raa_releaselist(raa_current_db, *(tmp_blists++) );
 	 }
  return codret;
}


int raa_nexteltinlist(raa_db_access *raa_current_db, int first, int lrank, char **pname, int *plength) 
{
return raa_nexteltinlist_annots(raa_current_db, first, lrank, pname, plength, NULL, NULL);
}


int raa_nexteltinlist_annots(raa_db_access *raa_current_db, int first, int lrank, char **pname, int *plength, 
	raa_long *paddr, int *pdiv)
{
int num, next, count;
char *p;
Reponse *rep;

if(raa_current_db == NULL) return 0;
if(lrank == raa_current_db->nextelt_data.current_rank && 
		raa_current_db->nextelt_data.previous < raa_current_db->nextelt_data.total - 1 && 
	(raa_current_db->nextelt_data.previous == -1 || 
		first == raa_current_db->nextelt_data.tabnum[raa_current_db->nextelt_data.previous] ) ) {
	raa_current_db->nextelt_data.previous++;
	next = raa_current_db->nextelt_data.tabnum[raa_current_db->nextelt_data.previous];
	if(next != 0) {
		if(pname != NULL) *pname = raa_current_db->nextelt_data.tabname[raa_current_db->nextelt_data.previous];
		if(plength != NULL) *plength = raa_current_db->nextelt_data.tablength[raa_current_db->nextelt_data.previous];
		if(paddr != NULL) *paddr = raa_current_db->nextelt_data.taboffset[raa_current_db->nextelt_data.previous];
		if(pdiv != NULL) *pdiv = raa_current_db->nextelt_data.tabdiv[raa_current_db->nextelt_data.previous];
		}
	return next;
	}

count = BLOCK_ELTS_IN_LIST; 
for(num = 0; num < count; num++) {
	if(raa_current_db->nextelt_data.tabname[num] != NULL) free(raa_current_db->nextelt_data.tabname[num]);
	}
memset(raa_current_db->nextelt_data.tabname, 0, count * sizeof(char *));
sock_printf(raa_current_db,"nexteltinlist&lrank=%d&first=%d&count=%d\n",lrank,first, count);
num = 0; raa_current_db->nextelt_data.current_rank = lrank;
raa_current_db->nextelt_data.total = 0;
do {
  p = read_sock(raa_current_db);
  if(p == NULL) return 0;
  rep = initreponse();
  parse(p, rep);
  p = val(rep,"next");
  if(p == NULL) {
	clear_reponse(rep);
	return 0;
	}
  next = atoi(p);
  free(p);
  raa_current_db->nextelt_data.total++;
  raa_current_db->nextelt_data.tabnum[num] = next;
  if(next != 0) {
	  raa_current_db->nextelt_data.tabname[num] = val(rep, "name");
	  if( (p= val(rep, "length")) != NULL) {
		raa_current_db->nextelt_data.tablength[num] = atoi(p);
		free(p);
		}
	  if((p= val(rep, "offset")) != NULL) {
		raa_current_db->nextelt_data.taboffset[num] = scan_raa_long(p);
		free(p);
		}
	  if((p= val(rep, "div")) != NULL) {
		raa_current_db->nextelt_data.tabdiv[num] = atoi(p);
		free(p);
		}
	}
  clear_reponse(rep);
  num++;
  }
while(next != 0 && --count > 0);
raa_current_db->nextelt_data.previous = -1;
return raa_nexteltinlist_annots(raa_current_db, first, lrank, pname, plength, paddr, pdiv);
}


raa_long scan_raa_long(char *txt)
{
raa_long val;

sscanf(txt, RAA_LONG_FORMAT, &val);
return val;
}


char *print_raa_long(raa_long val)
{
static char buffer[50];
sprintf(buffer, RAA_LONG_FORMAT, val);
return buffer;
}




static char *load_annots_buf(raa_db_access *raa_current_db, raa_long faddr, int div, int was_nextannots)
/* appeler juste après avoir envoye sur socket
read_annots&... ou  next_annots&... : 
retour attendu sur socket : nl=xx[&offset=xx]&...nl lines each with \n     */
{
int i, nl;
char *firstline, *p, *q;

  if(raa_current_db == NULL) return NULL;
  for(i=0; i < raa_current_db->annot_data.annotcount; i++) 
  	free(raa_current_db->annot_data.annotline[i]);
  raa_current_db->annot_data.annotcount = 0;
  raa_current_db->annot_data.annotaddrlast = faddr;
  firstline = read_sock(raa_current_db);
  if(firstline == NULL) return NULL;
  if(strncmp(firstline, "nl=", 3) != 0 || (p = strchr(firstline, '&')) == NULL ) return NULL;
  nl = atoi(firstline + 3);
  if(nl == 0) return NULL;
  p++;
  if(was_nextannots && strncmp(p, "offset=", 7) == 0) {
  	p = strchr(p, '&');
  	if(p == NULL) return NULL;
  	p++;
  	}
  i = strlen(p);
  raa_current_db->annot_data.annotline[0] = (char *)malloc(i+1);
  strcpy(raa_current_db->annot_data.annotline[0], p);
  raa_current_db->annot_data.annotaddrlast += strlen(raa_current_db->annot_data.annotline[0]) + 1;
  for(i = 1; i < nl; i++) {
  	q = read_sock(raa_current_db);
  	if(q == NULL) return NULL;
  	raa_current_db->annot_data.annotline[i] = strdup( q );
  	raa_current_db->annot_data.annotaddrlast += strlen(raa_current_db->annot_data.annotline[i]) + 1;
  	} 
  raa_current_db->annot_data.annotcurrent = 1; raa_current_db->annot_data.annotcount = nl; 
  raa_current_db->annot_data.annotaddr = faddr; raa_current_db->annot_data.annotdiv = div; 
  raa_current_db->annot_data.annotaddrfirst = faddr;
  strcpy(raa_current_db->annot_data.annotsbuffer, raa_current_db->annot_data.annotline[0]);
  return raa_current_db->annot_data.annotsbuffer;
}


char *raa_read_annots(raa_db_access *raa_current_db, raa_long faddr, int div)
{
  int i;
  raa_long debut;
  char *p;

if(raa_current_db == NULL) return NULL;
  if(raa_current_db->annot_data.annotcount > 0 && 
  		div == raa_current_db->annot_data.annotdiv && 
  		faddr >= raa_current_db->annot_data.annotaddrfirst && 
  		faddr < raa_current_db->annot_data.annotaddrlast ) {
	debut = raa_current_db->annot_data.annotaddrfirst;
  	for(i = 1; i <= raa_current_db->annot_data.annotcount; i++) {
  		if(debut == faddr) {
  			raa_current_db->annot_data.annotcurrent = i;
  			raa_current_db->annot_data.annotaddr = debut;
  			strcpy(raa_current_db->annot_data.annotsbuffer, raa_current_db->annot_data.annotline[raa_current_db->annot_data.annotcurrent - 1]);
  			return raa_current_db->annot_data.annotsbuffer;
  			}
  		debut += strlen(raa_current_db->annot_data.annotline[i - 1]) + 1;
  		} 
	}
  sock_printf(raa_current_db,"read_annots&offset=%s&div=%d&nl=%d\n", 
  		print_raa_long(faddr), div, ANNOTCOUNT);
  p = load_annots_buf(raa_current_db, faddr, div, FALSE);
  return p;
}


char *raa_next_annots(raa_db_access *raa_current_db, raa_long *paddr)
{
  raa_long faddr;
  char *p;
  
if(raa_current_db == NULL) return NULL;
  if(raa_current_db->annot_data.annotcurrent < raa_current_db->annot_data.annotcount) {
  	raa_current_db->annot_data.annotaddr += 
  		strlen(raa_current_db->annot_data.annotline[raa_current_db->annot_data.annotcurrent - 1]) + 1;
  	if(paddr != NULL) *paddr = raa_current_db->annot_data.annotaddr;
  	raa_current_db->annot_data.annotcurrent++;
  	strcpy(raa_current_db->annot_data.annotsbuffer, 
  		raa_current_db->annot_data.annotline[raa_current_db->annot_data.annotcurrent - 1]);
  	return raa_current_db->annot_data.annotsbuffer;
  	}
  faddr = raa_current_db->annot_data.annotaddrlast;
  if(paddr != NULL) *paddr = faddr;
  sock_printf(raa_current_db, "next_annots&nl=%d\n", ANNOTCOUNT);
  p = load_annots_buf(raa_current_db, faddr, raa_current_db->annot_data.annotdiv, TRUE);
  return p;
}


int raa_iknum(raa_db_access *raa_current_db, char *name, raa_file cas)
{
char *reponse, *p;
int val;

if(raa_current_db == NULL) return 0;
  sock_printf(raa_current_db,"iknum&name=\"%s\"&type=%s\n", protect_quotes(name), 
  	(cas == raa_key ? "KW" : "SP") );
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  p = strchr(reponse, '=');
  if(p == NULL) return 0;
  p++;
  sscanf(p, "%u", &val);
  return val;
}


int raa_isenum(raa_db_access *raa_current_db, char *name)
{
char *reponse, *p;
int val;

if(raa_current_db == NULL) return 0;
  sock_printf(raa_current_db,"isenum&name=\"%s\"\n", protect_quotes(name) );
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  p = strchr(reponse, '=');
  if(p == NULL) return 0;
  p++;
  sscanf(p, "%u", &val);
  return val;
}


int raa_bcount(raa_db_access *raa_current_db, int lrank)
{
  Reponse *rep;
  char *reponse, *code, *countstr;
  int count = 0;
  
if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_printf(raa_current_db,"bcount&lrank=%d\n",lrank);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  code = val(rep,"code");
  if(*code == '0') {
  	countstr = val(rep,"count");
	count = atoi(countstr);
	free(countstr); 
  	}
  free(code);
  clear_reponse(rep);
  return count;
}

void raa_bit1(raa_db_access *raa_current_db, int lrank, int num)
{
char *reponse;

if(raa_current_db == NULL) return;
  sock_printf(raa_current_db,"bit1&lrank=%d&num=%d\n", lrank, num);
  reponse=read_sock(raa_current_db);
}


void raa_bit0(raa_db_access *raa_current_db, int lrank, int num)
{
char *reponse;

if(raa_current_db == NULL) return;
  sock_printf(raa_current_db,"bit0&lrank=%d&num=%d\n", lrank, num);
  reponse=read_sock(raa_current_db);
}


int raa_btest(raa_db_access *raa_current_db, int lrank, int num)
{
char *reponse;

if(raa_current_db == NULL) return 0;
  sock_printf(raa_current_db,"btest&lrank=%d&num=%d\n", lrank, num);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  return strcmp(reponse, "code=0&on") == 0;
}

void raa_copylist(raa_db_access *raa_current_db, int from, int to)
{
char *reponse;

if(raa_current_db == NULL) return;
  sock_printf(raa_current_db,"copylist&lfrom=%d&lto=%d\n", from, to);
  reponse=read_sock(raa_current_db);
}


void raa_zerolist(raa_db_access *raa_current_db, int rank)
{
char *reponse;

if(raa_current_db == NULL) return;
  sock_printf(raa_current_db,"zerolist&lrank=%d\n", rank);
  reponse=read_sock(raa_current_db);
}


void raa_setliststate(raa_db_access *raa_current_db, int lrank, int locus, int type)
{
char *reponse, str_type[3];

if(raa_current_db == NULL) return;
if(type == 'S') strcpy(str_type, "SQ");
else if(type == 'K') strcpy(str_type, "KW");
else  strcpy(str_type, "SP");

  sock_printf(raa_current_db,"setliststate&lrank=%d&locus=%c&type=%s\n", lrank, (locus ? 'T' : 'F'),
  	str_type );
  reponse=read_sock(raa_current_db);
}


char *raa_getliststate(raa_db_access *raa_current_db, int lrank, int *locus, int *type, int *count)
/* list name is returned in static memory */
{
  Reponse *rep;
  char *reponse, *code, *countstr, *locusstr, *typestr, *retp = NULL;
  static char *namestr = NULL;
  
if(raa_current_db == NULL) return NULL;
  rep=initreponse();
  sock_printf(raa_current_db,"getliststate&lrank=%d\n",lrank);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return NULL;
  parse(reponse, rep);
  code = val(rep,"code");
  if(code != NULL && *code == '0') {
  	typestr = val(rep,"type");
  	if(type != NULL) {
  		if(strcmp(typestr, "SQ") == 0) *type = 'S';
  		else if(strcmp(typestr, "KW") == 0) *type = 'K';
  		else *type = 'E';
  		}
  	if(namestr != NULL) free(namestr); /* allocation precedante */
  	namestr = val(rep, "name");
  	countstr = val(rep, "count");
	if(count != NULL) *count = atoi(countstr);
  	locusstr = val(rep, "locus");
  	if(locus != NULL) *locus = (*locusstr == 'T');
	free(countstr); 
	free(locusstr); 
	free(typestr); 
	retp = namestr;
  	}
  if(code != NULL) free(code);
  clear_reponse(rep);
  return retp;
}


char *raa_residuecount(raa_db_access *raa_current_db, int lrank)
{
Reponse *rep;
char *reponse, *code;
static char total[30];

if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_printf(raa_current_db,"residuecount&lrank=%d\n",lrank);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  code = val(rep,"code");
  strcpy(total, "0");
  if(code != NULL && *code == '0') {
	free(code);
  	code = val(rep,"count");
 	if(code != NULL) {
		strcpy(total, code);
		free(code);
		}
	}
clear_reponse(rep);
return total;
}


int raa_getemptylist(raa_db_access *raa_current_db, char *name)
{
  Reponse *rep;
  char *reponse, *code, *rankstr;
  int rank = 0;
  
if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_printf(raa_current_db, "getemptylist&name=\"%s\"\n", protect_quotes(name));
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  code = val(rep,"code");
  if(code != NULL && (*code == '0' || *code == '3') ) {
  	rankstr = val(rep, "lrank");
	rank = atoi(rankstr);
	free(rankstr); 
  	}
  if(code != NULL) free(code);
  clear_reponse(rep);
  return rank;
}


int raa_setlistname(raa_db_access *raa_current_db, int lrank, char *name)
{
  Reponse *rep;
char *reponse, *code;
int retval;

if(raa_current_db == NULL) return -1;
  rep=initreponse();
  sock_printf(raa_current_db, "setlistname&lrank=%d&name=\"%s\"\n", lrank, name);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return -1;
  parse(reponse, rep);
  code = val(rep,"code");
  if(*code == '0') retval = 0;
  else if(*code == '3') retval = 1;
  else retval = -1;
  free(code);
  clear_reponse(rep);
return retval;  
}


int raa_getlistrank(raa_db_access *raa_current_db, char *name)
{
  Reponse *rep;
char *reponse, *rankstr;
int rank;

if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_printf(raa_current_db, "getlistrank&name=\"%s\"\n", name);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  rankstr = val(rep,"lrank");
  if(rankstr == NULL) return 0;
  rank = atoi(rankstr);
  free(rankstr);
  clear_reponse(rep);
  return rank;  
}



int raa_releaselist(raa_db_access *raa_current_db, int lrank)
{
  Reponse *rep;
char *reponse, *rankstr;
int rank;

if(raa_current_db == NULL) return 1;
  rep=initreponse();
  sock_printf(raa_current_db, "releaselist&lrank=%d\n", lrank);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 1;
  parse(reponse, rep);
  rankstr = val(rep,"code");
  if(rankstr == NULL) return 1;
  rank = atoi(rankstr);
  free(rankstr);
  clear_reponse(rep);
  return rank;  
}



int raa_countfilles(raa_db_access *raa_current_db, int lrank)
{
  Reponse *rep;
char *reponse, *rankstr;
int rank;

if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_printf(raa_current_db, "countsubseqs&lrank=%d\n", lrank);
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  rankstr = val(rep,"count");
  if(rankstr == NULL) return 0;
  rank = atoi(rankstr);
  free(rankstr);
  clear_reponse(rep);
  return rank;  
}


int raa_alllistranks(raa_db_access *raa_current_db, int **pranks)
{
  Reponse *rep;
char *reponse, *rankstr, *p;
int count, *ranks, i;

if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_fputs(raa_current_db, "alllistranks\n");
  reponse=read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  rankstr = val(rep,"count");
  p = strchr(reponse, '&');
  if(rankstr == NULL || p == NULL) return 0;
  count = atoi(rankstr);
  ranks = (int *)malloc(count * sizeof(int));
  if(ranks == NULL ) return 0;
  p++;
  for(i=0; i< count; i++) {
  	sscanf(p, "%d", &ranks[i]);
  	p = strchr(p, ',');
  	if(p == NULL) break;
  	p++;
  	}
  *pranks = ranks;
  free(rankstr);
  clear_reponse(rep);
  return count;  
}



int raa_fcode(raa_db_access *raa_current_db, raa_file cas, char *name)
{
char *reponse, *p, type[5];
int value;

if(raa_current_db == NULL) return 0;
if(cas == raa_aut) strcpy(type, "AUT");
else if(cas == raa_bib) strcpy(type, "BIB");
else if(cas == raa_acc) strcpy(type, "ACC");
else if(cas == raa_smj) strcpy(type, "SMJ");
else if(cas == raa_sub) strcpy(type, "SUB");
else return 0;
  sock_printf(raa_current_db,"fcode&name=\"%s\"&type=%s\n", protect_quotes(name), type );
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  p = strchr(reponse, '=');
  if(p == NULL) return 0;
  p++;
  sscanf(p, "%u", &value);
  return value;
}



int raa_read_first_rec(raa_db_access *raa_current_db, raa_file cas)
{
int value;
char type[5], *p, *reponse;

if(raa_current_db == NULL) return 1;
if(raa_current_db->first_recs[cas] != 0) return raa_current_db->first_recs[cas];

if(cas == raa_aut) strcpy(type, "AUT");
else if(cas == raa_bib) strcpy(type, "BIB");
else if(cas == raa_acc) strcpy(type, "ACC");
else if(cas == raa_smj) strcpy(type, "SMJ");
else if(cas == raa_sub) strcpy(type, "SUB");


else if(cas == raa_loc) strcpy(type, "LOC");
else if(cas == raa_key) strcpy(type, "KEY");
else if(cas == raa_spec) strcpy(type, "SPEC");
else if(cas == raa_shrt) strcpy(type, "SHRT");
else if(cas == raa_lng) strcpy(type, "LNG");
else if(cas == raa_ext) strcpy(type, "EXT");
else if(cas == raa_txt) strcpy(type, "TXT");
else return 0;
sock_printf(raa_current_db,"readfirstrec&type=%s\n", type );
reponse = read_sock(raa_current_db);
if(reponse == NULL) return 1;
p = strstr(reponse, "count=");
if(p == NULL) return 0;
sscanf(p + 6, "%u", &value);
raa_current_db->first_recs[cas] = value;
return value;
}


char *raa_readsub_pannots(raa_db_access *raa_current_db, int num, int *plength, int *ptype, int *pext, int *plkey, int *plocus, 
	int *pframe, int *pgencode, raa_long *paddr, int *pdiv)
/* do both seq_to_annots and readsub and buffer result for one seq */
{
  Reponse *rep;
  char *p, *reponse;
  int code, l;
  
if(raa_current_db == NULL) return NULL;
  if(num < 2 || num > raa_current_db->nseq) return NULL;
  if(num != raa_current_db->readsub_data.previous) {
  	sock_printf(raa_current_db,"seq_to_annots&number=%d\nreadsub&num=%u\n", num, num);
  	reponse = read_sock(raa_current_db);
    if(reponse == NULL) return NULL;
  	/* process reply to seq_to_annots */
  	rep=initreponse();
  	parse(reponse, rep);
 	p = val(rep,"code");
	code = atoi(p);
  	free(p);
  	if(code == 0) {
  		p = val(rep,"offset");
  		raa_current_db->readsub_data.addr = scan_raa_long(p);
  		free(p);
  		p = val(rep,"div");
		raa_current_db->readsub_data.div = atoi(p);
  		free(p);
  		}
  	clear_reponse(rep);
  	/* process reply to readsub */
  	reponse = read_sock(raa_current_db);
    if(reponse == NULL) return NULL;
  	rep=initreponse();
  	parse(reponse, rep);
  	p = val(rep,"code");
  	if(p == NULL) return NULL;
  	code = atoi(p);
  	free(p);
  	if(code != 0) {
  		clear_reponse(rep);
  		return NULL;
  		}
  	p = val(rep, "name");
  	l = strlen(p) + 1;
  	if(l > raa_current_db->readsub_data.lname) {
  		raa_current_db->readsub_data.lname = l;
  		raa_current_db->readsub_data.name = (char *)realloc(raa_current_db->readsub_data.name, 
  			raa_current_db->readsub_data.lname);
  		}
  	strcpy(raa_current_db->readsub_data.name, p);
  	free(p);
  	p = val(rep, "length");
  	raa_current_db->readsub_data.length = atoi(p);
  	free(p);
  	p = val(rep, "type");
  	raa_current_db->readsub_data.type = atoi_u(p);
  	free(p);
  	p = val(rep, "is_sub");
  	raa_current_db->readsub_data.locus = atoi_u(p);
  	free(p);
  	p = val(rep, "toext");
  	raa_current_db->readsub_data.toext = atoi_u(p);
  	if(raa_current_db->readsub_data.locus > 0) 
  			raa_current_db->readsub_data.toext = - raa_current_db->readsub_data.toext;
  	free(p);
  	p = val(rep, "plkey");
  	raa_current_db->readsub_data.lkey = atoi_u(p);
  	free(p);
  	p = val(rep, "frame");
  	raa_current_db->readsub_data.frame = atoi_u(p);
  	free(p);
  	p = val(rep, "genet");
  	raa_current_db->readsub_data.gencode = atoi_u(p);
  	free(p);
  	raa_current_db->readsub_data.previous = num;
  	clear_reponse(rep);
  	}
  if(plength != NULL) *plength = raa_current_db->readsub_data.length;
  if(ptype != NULL) *ptype = raa_current_db->readsub_data.type;
  if(plocus != NULL) *plocus = raa_current_db->readsub_data.locus;
  if(pext != NULL) *pext = raa_current_db->readsub_data.toext;
  if(plkey != NULL) *plkey = raa_current_db->readsub_data.lkey;
  if(pframe != NULL) *pframe = raa_current_db->readsub_data.frame;
  if(pgencode != NULL) *pgencode = raa_current_db->readsub_data.gencode;
  if(paddr != NULL) *paddr = raa_current_db->readsub_data.addr;
  if(pdiv != NULL) *pdiv = raa_current_db->readsub_data.div;
  return raa_current_db->readsub_data.name;  
}


int raa_seq_to_annots(raa_db_access *raa_current_db, int numseq, raa_long *faddr, int *div)
{
char *p;

p = raa_readsub_pannots(raa_current_db, numseq,NULL,NULL,NULL,NULL,NULL,NULL,NULL,faddr,div);
return p == NULL;
}


char *raa_readsub(raa_db_access *raa_current_db, int num, int *plength, int *ptype, int *pext, int *plkey, int *plocus, 
	int *pframe, int *pgencode)
{
return raa_readsub_pannots(raa_current_db, num,plength,ptype,pext,plkey,plocus,pframe,pgencode,NULL,NULL);
}


char *raa_readloc(raa_db_access *raa_current_db, int num, int *sub, int *pnuc, int *spec, int *host, int *plref, 
	int *molec, int *placc, int *org)
{
  Reponse *rep;
  char *p, *reponse;
  int code;
  static char date[50];

if(raa_current_db == NULL) return NULL;
  rep=initreponse();
  sock_printf(raa_current_db,"readloc&num=%u\n", num);
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return NULL;
  parse(reponse, rep);
  p = val(rep,"code");
  if(p == NULL) return NULL;
  code = atoi(p);
  free(p);
  if(code == 0) {
  	if(sub != NULL) {
  		p = val(rep, "sub");
  		*sub = atoi_u(p);
  		free(p);
  		}
  	if(pnuc != NULL) {
  		p = val(rep, "pnuc");
  		*pnuc = atoi_u(p);
  		free(p);
  		}
  	if(spec != NULL) {
  		p = val(rep, "spec");
  		*spec = atoi_u(p);
  		free(p);
  		}
  	if(host != NULL) {
  		p = val(rep, "host");
  		*host = atoi_u(p);
  		free(p);
  		}
  	if(plref != NULL) {
  		p = val(rep, "plref");
  		*plref = atoi_u(p);
  		free(p);
  		}
  	if(molec != NULL) {
  		p = val(rep, "molec");
  		*molec = atoi_u(p);
  		free(p);
  		}
  	if(placc != NULL) {
  		p = val(rep, "placc");
  		*placc = atoi_u(p);
  		free(p);
  		}
  	if(org != NULL) {
  		p = val(rep, "org");
  		*org = atoi_u(p);
  		free(p);
  		}
  	p = val(rep, "date");
  	strcpy(date, p);
  	free(p);
  	p = date;
  	}
  else p = NULL;
  clear_reponse(rep);
  return p;  
}


char *raa_readspec(raa_db_access *raa_current_db, int num, char **plibel, int *plsub, int *pdesc, int *psyno, int *plhost)
{
  Reponse *rep;
  char *p, *reponse;
  int code;
  
if(raa_current_db == NULL) return NULL;
  if(num == raa_current_db->readspec_data.previous && raa_current_db->readspec_data.previous != 0) {
  	if(plibel != NULL) {
  		if(*(raa_current_db->readspec_data.libel) != 0) *plibel = raa_current_db->readspec_data.libel;
  		else *plibel = NULL;
  		}
  	if(plsub != NULL) *plsub = raa_current_db->readspec_data.lsub;
  	if(pdesc != NULL) *pdesc = raa_current_db->readspec_data.desc;
  	if(psyno != NULL) *psyno = raa_current_db->readspec_data.syno;
  	if(plhost != NULL) *plhost = raa_current_db->readspec_data.host;
  	return raa_current_db->readspec_data.name;
  	}

  rep=initreponse();
  sock_printf(raa_current_db, "readspec&num=%u\n", num);
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return NULL;
  parse(reponse, rep);
  p = val(rep,"code");
  if(p == NULL) return NULL;
  code = atoi(p);
  free(p);
  if(code != 0) {
  	  clear_reponse(rep);
  	  return NULL;
  	  }

  raa_current_db->readspec_data.previous = num;
  	p = val(rep, "plsub");
  	raa_current_db->readspec_data.lsub = atoi_u(p);
  	free(p);
  	p = val(rep, "desc");
  	raa_current_db->readspec_data.desc = atoi_u(p);
  	free(p);
  	p = val(rep, "syno");
  	raa_current_db->readspec_data.syno = atoi_u(p);
  	free(p);
  	p = val(rep, "host");
  	raa_current_db->readspec_data.host = atoi_u(p);
  	free(p);
  	p = val(rep, "libel");
  	if(p != NULL) {
  		strcpy(raa_current_db->readspec_data.libel, p);
  		free(p);
  		}
  	else *(raa_current_db->readspec_data.libel) = 0;
  	p = val(rep, "name");
  	strcpy(raa_current_db->readspec_data.name, p );
  	free(p);
  clear_reponse(rep);
  return raa_readspec(raa_current_db, num, plibel, plsub, pdesc, psyno, plhost);  
}


char *raa_readkey(raa_db_access *raa_current_db, int num, char **plibel, int *plsub, int *pdesc, int *psyno)
{
  Reponse *rep;
  char *p, *reponse;
  int code;
  
if(raa_current_db == NULL) return NULL;
  if(num == raa_current_db->readkey_data.previous && raa_current_db->readkey_data.previous != 0) {
  	if(plibel != NULL) {
  		if(*(raa_current_db->readkey_data.libel) != 0) *plibel = raa_current_db->readkey_data.libel;
  		else *plibel = NULL;
  		}
  	if(plsub != NULL) *plsub = raa_current_db->readkey_data.lsub;
  	if(pdesc != NULL) *pdesc = raa_current_db->readkey_data.desc;
  	if(psyno != NULL) *psyno = raa_current_db->readkey_data.syno;
  	return raa_current_db->readkey_data.name;
  	}

  rep=initreponse();
  sock_printf(raa_current_db, "readkey&num=%u\n", num);
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return NULL;
  parse(reponse, rep);
  p = val(rep,"code");
  if(p == NULL) return NULL;
  code = atoi(p);
  free(p);
  if(code != 0) {
  	  clear_reponse(rep);
  	  return NULL;
  	  }

    raa_current_db->readkey_data.previous = num;
  	p = val(rep, "plsub");
  	raa_current_db->readkey_data.lsub = atoi_u(p);
  	free(p);
  	p = val(rep, "desc");
  	raa_current_db->readkey_data.desc = atoi_u(p);
  	free(p);
  	p = val(rep, "syno");
  	raa_current_db->readkey_data.syno = atoi_u(p);
  	free(p);
  	p = val(rep, "libel");
  	if(p != NULL) {
  		strcpy(raa_current_db->readkey_data.libel, p);
  		free(p);
  		}
  	else *(raa_current_db->readkey_data.libel) = 0;
  	p = val(rep, "name");
  	strcpy(raa_current_db->readkey_data.name, p);
  	free(p);
  clear_reponse(rep);
  return raa_readkey(raa_current_db, num, plibel, plsub, pdesc, psyno);  
}


static int load_smj(raa_db_access *raa_current_db, char ***names, unsigned **plongs, char ***libels)
{
  Reponse *rep;
  char *reponse, *p;
  int nl, i, code, totsmj, recnum;

if(raa_current_db == NULL) return 0;
totsmj = raa_read_first_rec(raa_current_db, raa_smj);
  rep=initreponse();
  sock_printf(raa_current_db,"readsmj&num=2&nl=%d\n", totsmj - 1);
/* ==>readsmj&num=..&nl=..
code=0&nl=..
recnum=..&name=".."&plong=..{&libel=".."}   nl times
*/
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  p = val(rep,"code");
  if(p == NULL) return 0;
  code = atoi(p);
  free(p);
  if(code != 0) return 0;
  p = val(rep,"nl");
  if(p == NULL) return 0;
  nl = atoi(p);
  free(p);
  clear_reponse(rep);
  if(nl == 0) return 0;
  *names = (char **)calloc(totsmj + 1 , sizeof(char *));
  *plongs = (unsigned *)calloc(totsmj + 1 , sizeof(unsigned));
  *libels = (char **)calloc(totsmj + 1 , sizeof(char *));
  for(i = 0; i < nl; i++) {
   	reponse = read_sock(raa_current_db);
    if(reponse == NULL) return 0;
 	rep=initreponse();
    	parse(reponse, rep);
  	p = val(rep, "recnum");
  	recnum = atoi(p);
  	free(p);
  	p = val(rep, "plong");
  	(*plongs)[recnum] = atoi_u(p);
  	free(p);
  	p = val(rep, "name");
  	(*names)[recnum] = p;
  	p = val(rep, "libel");
  	(*libels)[recnum] = p;
  	clear_reponse(rep);
  	}
return totsmj;
}


char *raa_readsmj(raa_db_access *raa_current_db, int num, char **plibel, int *plong)
{
if(raa_current_db == NULL) return NULL;
if(raa_current_db->readsmj_data.lastrec == 0) {
	raa_current_db->readsmj_data.lastrec = load_smj(raa_current_db,  
		&raa_current_db->readsmj_data.names, &raa_current_db->readsmj_data.plongs, 
		&raa_current_db->readsmj_data.libels);
	}
if(num <= 1 || num > raa_current_db->readsmj_data.lastrec) return NULL;
if(plong != NULL) *plong = raa_current_db->readsmj_data.plongs[num];
if(plibel != NULL) *plibel = raa_current_db->readsmj_data.libels[num];
return raa_current_db->readsmj_data.names[num];
}


char *raa_readacc(raa_db_access *raa_current_db, int num, int *plsub)
{
  Reponse *rep;
  char *p, *reponse;
  int code;
  static char name[100];

if(raa_current_db == NULL) return NULL;
  rep=initreponse();
  sock_printf(raa_current_db,"readacc&num=%u\n", num);
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return NULL;
  parse(reponse, rep);
  p = val(rep,"code");
  if(p == NULL) return NULL;
  code = atoi(p);
  free(p);
  if(code == 0) {
  	if(plsub != NULL) {
  		p = val(rep, "plsub");
  		*plsub = atoi_u(p);
  		free(p);
  		}
  	p = val(rep, "name");
  	strcpy(name, p);
  	free(p);
  	p = name;
  	}
  else p = NULL;
  clear_reponse(rep);
  return p;  
}


int raa_readext(raa_db_access *raa_current_db, int num, int *mere, int *deb, int *fin)
{
  Reponse *rep;
  char *p, *reponse;
  int code, next;

if(raa_current_db == NULL) return 0;
  rep=initreponse();
  sock_printf(raa_current_db,"readext&num=%u\n", num);
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  parse(reponse, rep);
  p = val(rep,"code");
  if(p == NULL) return 0;
  code = atoi(p);
  free(p);
  if(code == 0) {
  	if(mere != NULL) {
  		p = val(rep, "mere");
  		*mere = atoi_u(p);
  		free(p);
  		}
  	if(deb != NULL) {
  		p = val(rep, "debut");
  		*deb = atoi_u(p);
  		free(p);
  		}
  	if(fin != NULL) {
  		p = val(rep, "fin");
  		*fin = atoi_u(p);
  		free(p);
  		}
  	p = val(rep, "next");
  	next = atoi_u(p);
  	free(p);
  	}
  else next = 0;
  clear_reponse(rep);
  return next;  
}


int raa_readlng(raa_db_access *raa_current_db, int num)
/* fills the rlng_buffer structure */
{
  char *p, *reponse;
  int count, i;

if(raa_current_db == NULL) return 0;
  memset(raa_current_db->rlng_buffer, 0, (raa_current_db->SUBINLNG+1)*sizeof(int));
  sock_printf(raa_current_db,"readlng&num=%u\n", num);
/* retour code=0&n=xx&x1,x2,...{&next=xx}  */
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return 0;
  if(strncmp(reponse, "code=0&n=", 9) != 0 || (count = atoi(reponse+9)) == 0) {
  	return 0;
  	}
  p = strchr(reponse+9, '&');
  for(i=0; i < count && i < raa_current_db->SUBINLNG; i++) {
  	raa_current_db->rlng_buffer->sub[i] = atoi_u(p+1);
  	p = strchr(p+1, ',');
  	if(p == NULL) break;
  	}
  p = strstr(reponse, "next=");
  if(p!= NULL) raa_current_db->rlng_buffer->next = atoi_u(p+5);
  else raa_current_db->rlng_buffer->next=0;
  return raa_current_db->rlng_buffer->next;  
}



static void load_shrt_buffer(raa_db_access *raa_current_db, unsigned point)
{
char *reponse, *p, *q;
int n, i;
unsigned val, next, previous;

if(raa_current_db == NULL) return;
sock_printf(raa_current_db, "readshrt&num=%u&max=%d\n", point, MAX_RDSHRT);
/* reponse is:  code=0&n=xx&val,next,.... n times ...\n  */
reponse = read_sock(raa_current_db);

if(reponse == NULL || strncmp(reponse, "code=0&n=", 9) != 0) {
	return;
	}
n = atoi(reponse + 9);
if (n == 0) return;
p = strchr(reponse+9, '&');
if(p == NULL) return;
q = p + 1;
previous = point;
for(i = 0; i < n; i++) {
	p = strtok(q, ",");
	q = NULL;
	sscanf(p, "%u", &val);
	p = strtok(NULL, ",");
	sscanf(p, "%u", &next);
	raa_current_db->readshrt_data.shrt_begin = (raa_current_db->readshrt_data.shrt_begin + 1) % S_BUF_SHRT;
	raa_current_db->readshrt_data.shrt_buffer[raa_current_db->readshrt_data.shrt_begin][0] = previous;
	raa_current_db->readshrt_data.shrt_buffer[raa_current_db->readshrt_data.shrt_begin][1] = val;
	raa_current_db->readshrt_data.shrt_buffer[raa_current_db->readshrt_data.shrt_begin][2] = next;
	previous = next;
	}
if(raa_current_db->readshrt_data.shrt_begin > raa_current_db->readshrt_data.shrt_max - 1) {
	raa_current_db->readshrt_data.shrt_max = raa_current_db->readshrt_data.shrt_begin + 1;
	}
return;	
}


unsigned raa_readshrt(raa_db_access *raa_current_db, unsigned point, int *pval)
{
int i;

if(raa_current_db == NULL) return 0;
if(raa_current_db->readshrt_data.total == 0) 
	raa_current_db->readshrt_data.total = (unsigned)raa_read_first_rec(raa_current_db, raa_shrt);
if(point < 2 || point > raa_current_db->readshrt_data.total) return 0;

for(i = 0; i < raa_current_db->readshrt_data.shrt_max; i++) {
	if(raa_current_db->readshrt_data.shrt_buffer[i][0] == point) {
		if(pval != NULL) *pval = raa_current_db->readshrt_data.shrt_buffer[i][1];
		return raa_current_db->readshrt_data.shrt_buffer[i][2];
		}
	}
load_shrt_buffer(raa_current_db, point);
return raa_readshrt(raa_current_db, point, pval);
}



char *raa_ghelp(raa_db_access *raa_current_db, char *fname, char *topic)
/* returns all help topic in one string in private memory
*/
{
char *reponse, *p, *fintext, *tmp;
int nl, l, i;
static char *text = NULL;
static int ltext = 0;

if(raa_current_db == NULL) return NULL;
  sock_printf(raa_current_db,"ghelp&file=%s&item=%s\n", fname, topic);
  reponse = read_sock(raa_current_db);
  if(reponse == NULL) return NULL;
  nl = 0;
  if(strncmp(reponse, "nl=", 3) == 0) nl = atoi(reponse+3);
  p = strchr(reponse, '&');
  if(nl <= 0 || p == NULL) return NULL;
  fintext = text; p++;
  for(i = 0; i < nl; i++) {
  	l = strlen(p) + 1; /* +1 pour ajouter \n */
  	if( (fintext - text) + l > ltext) {
  		ltext += 1000;
  		tmp = (char *)realloc(text, ltext + 1);
  		if(tmp == NULL) {
  			if(text != NULL) free(text);
  			text = NULL; ltext = 0;
  			return NULL;
  			}
  		fintext = tmp + (fintext - text);
  		text = tmp;
  		}
  	memcpy(fintext, p, l -1);
  	fintext += l;
  	*(fintext - 1) = '\n';
  	if(i + 1 < nl) reponse = read_sock(raa_current_db);
    if(reponse == NULL) return NULL;
  	p = reponse;
  	}
  *fintext = 0;
  return text;	
}


struct raa_matchkey {
	int *ranks;
	char **names;
	int count;
	int current;
	int no_more;
	};

static void raa_free_matchkeys(raa_db_access *raa_current_db)
{
int i;
struct raa_matchkey *data = (struct raa_matchkey *)(raa_current_db->matchkey_data);

if(data == NULL) return;
if(data->count > 0) {
	for(i = 0; i < data->count; i++) free(data->names[i]);
	free(data->names);
	free(data->ranks);
	}
free(data);
raa_current_db->matchkey_data = NULL;
}


static void next_block_matchkeys(raa_db_access *raa_current_db, int num, char *pattern)
{
const int blocksize = 2001;
char *reponse, *p;
int count, i;
struct raa_matchkey *data;

if(raa_current_db == NULL) return;
raa_free_matchkeys(raa_current_db);
sock_printf(raa_current_db, "nextmatchkey&num=%d", num );
if(num == 2) sock_printf(raa_current_db, "&pattern=\"%s\"", protect_quotes(pattern) );
sock_printf(raa_current_db, "&count=%d\n", blocksize);
reponse = read_sock(raa_current_db);
if(reponse == NULL || strncmp(reponse, "code=0&count=", 13) != 0) return;
sscanf(reponse + 13, "%d", &count);
data = (struct raa_matchkey *)calloc(1, sizeof(struct raa_matchkey));
raa_current_db->matchkey_data = data;
if(data == NULL || count == 0) return;
data->ranks = (int *)malloc(count * sizeof(int));
data->names = (char **)malloc(count * sizeof(char *));
data->current = 0;
data->no_more = (count < blocksize);
data->count = count;
if(data->ranks == NULL || data->names == NULL) data->count = 0;
for(i = 0; i < data->count; i++) {
	reponse = read_sock(raa_current_db);
	sscanf(reponse + 4, "%d", &(data->ranks[i]) );
	p = strchr(reponse, '&');
	p = strchr(p, '=') + 1;
	data->names[i] = strdup(unprotect_quotes(p));
	if(data->names[i] == NULL) {
		data->count = i;
		break;
		}
	}
for(i = data->count; i < count; i++) {
	read_sock(raa_current_db);
	}
return;
}


int raa_nextmatchkey(raa_db_access *raa_current_db, int num, char *pattern, char **matching)
/*  *matching returned in static memory */
{
struct raa_matchkey *data;
int no_more, count;

if(raa_current_db == NULL) return 0;
if(num == 2) {
	next_block_matchkeys(raa_current_db, 2, pattern);
	}
data = (struct raa_matchkey *)(raa_current_db->matchkey_data);
if(data == NULL) return 0;
if(data->current < data->count) {
	if(matching != NULL) *matching = data->names[data->current];
	return data->ranks[(data->current)++];
	}
count = data->count;
if(count > 0) {
	no_more = data->no_more;
	}
raa_free_matchkeys(raa_current_db);
if(count == 0 || no_more) return 0;
next_block_matchkeys(raa_current_db, num, NULL);
return raa_nextmatchkey(raa_current_db, num, NULL, matching);
}


int atoi_u(const char *p)
{
unsigned value;

sscanf(p, "%u", &value);
return (int)value;
}


char *protect_quotes(char *name)
/* remplacer tous les " par \"
name : une chaine inchangee
retourne un pointeur vers la chaine rendue privee pour cette fonction
*/
{
char *p, *q;
int l, count;
static char *bis = NULL;
static int lbis = 0;

count = 0; p = name - 1;
while( (p=strchr(p+1, '"')) != NULL) count++;
if(count == 0) return name;

l = strlen(name);
if(l + count > lbis) {
	lbis = l + count;
	bis = (char *)realloc(bis, lbis + 1);
	}
p = name; q = bis;
while(TRUE) {
	if(*p == '"') *(q++) = '\\';
	*q = *p;
	if(*p == 0) break;
	q++; p++;
	}
return bis;
}


static char *prepare_remote_file(raa_db_access *raa_current_db, char *oldrequete, char *debut, char *type, int *plrank, 
	char **badfname)
{
char *p, *q, *reponse, *fin;
static char line[200];
int nl, l, code;
FILE *in;
Reponse *rep;

*plrank = 0; *badfname = line;
p = strchr(debut, '=') + 1;
while(isspace(*p)) p++;
if(*p == '"') { /* if filename is bracketed by " */
	*(p++) = ' ';
	fin = strchr(p, '"');
	if(fin == NULL) return NULL;
	*fin = ' ';
	}
else	{
	fin = p;
	do ++fin; while( *fin != 0 && *fin != ')' && !isspace(*fin) );
	}
l = fin - p;
if(l >= sizeof(line)) l = sizeof(line) - 1;
memcpy(line, p, l); line[l] = 0;
if(fin - p >= sizeof(line)) return NULL;
in = fopen(line, "r");
if(in == NULL) return NULL;
nl = 0;
while( fgets(line, sizeof(line), in) != NULL) nl++;
rewind(in);
sock_printf(raa_current_db, "crelistfromclientdata&type=%s&nl=%d\n", type, nl);
if(nl > 0) {
	while( fgets(line, sizeof(line), in) != NULL) {
		l = strlen(line);
		if(line[l - 1] != '\n')  strcpy(line + l, "\n");
		sock_fputs(raa_current_db, line);
		}
	}
fclose(in);

reponse = read_sock(raa_current_db);
if(reponse == NULL) {
	strcpy(line, "connection with server is down");
	return NULL;
	}
rep = initreponse();
parse(reponse, rep);
q = val(rep,"code");
code = atoi(q);
free(q);
if(code != 0) {
	if(code == 3) strcpy(line, "too many lists, delete a few");
	else sprintf(line, "code=%d", code);
	return NULL;
	}
q = val(rep,"name");
l = strlen(q);
reponse = (char *)malloc( (debut - oldrequete) + 1 + l + 1 + strlen(fin) + 1);
p = reponse;
memcpy(p, oldrequete, debut - oldrequete);
p += debut - oldrequete;
*(p++) = ' ';
memcpy(p, q, l);
p += l;
*(p++) = ' ';
free(q);
strcpy(p, fin);
free(oldrequete);
q = val(rep,"lrank");
if(q != NULL) {
	*plrank = atoi(q);
	free(q);
	}

clear_reponse(rep);
return reponse;
}


static int *add_tmp_blist(int lrank, int *list)
{
static int total = 0, current, *prelist = NULL; /* need not be specific to each opened db */

if(list == NULL) { /* initialisation */
	if(prelist != NULL) free(prelist);
	total = 10; current = 0;
	list = (int *)malloc(total*sizeof(int));
	prelist = list;
	return list;
	}
if(current >= total) {
	int *tmp;
	tmp = (int *)realloc(list, (total + 10)*sizeof(int));
	if(tmp == NULL) return list;
	total += 10;
	list = tmp;
	}
list[current++] = lrank;
prelist = list;
return list;
}


char *maj_strstr(char *in, char *target)
{
static char *buffer = NULL;
static int lbuf = 0;
int l;
char *p;

l = strlen(in);
if(l > lbuf) {
	lbuf = l;
	buffer = (char *)realloc(buffer, lbuf + 1);
	}
strcpy(buffer, in);
majuscules(buffer);
p = strstr(buffer, target);
if(p != NULL) p = in + (p - buffer);
return p;
}


static char *raa_requete_remote_file(raa_db_access *raa_current_db, char *oldrequete, int **plist, char **pbadfname)
/*
rend NULL ou une requete dont les F= FA= FS= FK= ont ete changes en list-name
et qui a ete creee par malloc
*/
{
char *oldori, *p;
int lrank, *list;

oldori = strdup(oldrequete);
if(maj_strstr(oldori, "F=") == NULL && maj_strstr(oldori, "FA=") == NULL && 
   maj_strstr(oldori, "FK=") == NULL && maj_strstr(oldori, "FS=") == NULL) {
	*plist = NULL;
	return oldori;
	}
list = add_tmp_blist(0, NULL); /* initialisation a vide */
while(oldori != NULL && (p = maj_strstr(oldori, "F=")) != NULL) {
	oldori = prepare_remote_file(raa_current_db, oldori, p, "SQ", &lrank, pbadfname);
	if(lrank != 0) list = add_tmp_blist(lrank, list);
	}
while(oldori != NULL && (p = maj_strstr(oldori, "FA=")) != NULL) {
	oldori = prepare_remote_file(raa_current_db, oldori, p, "AC", &lrank, pbadfname);
	if(lrank != 0) list = add_tmp_blist(lrank, list);
	}
while(oldori != NULL && (p = maj_strstr(oldori, "FS=")) != NULL) {
	oldori = prepare_remote_file(raa_current_db, oldori, p, "SP", &lrank, pbadfname);
	if(lrank != 0) list = add_tmp_blist(lrank, list);
	}
while(oldori != NULL && (p = maj_strstr(oldori, "FK=")) != NULL) {
	oldori = prepare_remote_file(raa_current_db, oldori, p, "KW", &lrank, pbadfname);
	if(lrank != 0) list = add_tmp_blist(lrank, list);
	}
list = add_tmp_blist(0, list); /* marquage fin de liste par zero */
if(oldori == NULL && list != NULL) {
 	 while(*list != 0) raa_releaselist(raa_current_db,  *(list++) );
 	 list = NULL;
 	 }
*plist = list;
return oldori;
}


int raa_savelist(raa_db_access *raa_current_db, int lrank, FILE *out, int use_acc, char *prefix)
{
char *reponse;
int err;

if(raa_current_db == NULL) return 1;
sock_printf(raa_current_db, "savelist&lrank=%d&type=%c\n", lrank, (use_acc ? 'A' : 'N') );
reponse = read_sock(raa_current_db);
if(reponse == NULL) return 1;
err = strcmp(reponse, "code=0");
if(err != 0) {
	return 1;
	}
while(TRUE) {
	reponse = read_sock(raa_current_db);
	if(reponse == NULL) return 1;
	if(strcmp(reponse, "savelist END.") == 0) break;
	if(prefix != NULL) fputs(prefix, out);
	fprintf(out, "%s\n", reponse);
	}
return 0;
}


int raa_modifylist(raa_db_access *raa_current_db, int lrank, char *type, char *operation, int *pnewlist, int (*check_interrupt)(void), 
	int *p_processed )
{
Reponse *rep;
char *p, *reponse;
int code;

if(raa_current_db == NULL) return 3;
sock_printf(raa_current_db, "modifylist&lrank=%d&type=%s&operation=\"%s\"\n", lrank, 
	type, operation );
sock_flush(raa_current_db); /* tres important */
if(check_interrupt == NULL) {
	reponse = read_sock(raa_current_db);
	if(reponse == NULL) return 3;
	}
else	{
	while(TRUE) {
		reponse = read_sock_timeout(raa_current_db, 500 /* msec */);
		if(raa_current_db == NULL) return 3;
		if(reponse != NULL) break;
		if( check_interrupt() ) {
			sock_fputs(raa_current_db, "\033" /* esc */ );
			sock_flush(raa_current_db);
			}
		}
	}
	
rep = initreponse();
parse(reponse, rep);
p = val(rep,"code");
code = atoi(p);
free(p);
if(code != 0) return code;
p = val(rep,"lrank");
*pnewlist = atoi(p);
free(p);
p = val(rep,"processed");
if(p != NULL && p_processed != NULL) *p_processed = atoi(p);
if(p != NULL) free(p);
clear_reponse(rep);
return 0;
}


int raa_knowndbs(raa_db_access *raa_current_db, char ***pnames, char ***pdescriptions)
{
int nl;
char **names = NULL, **descriptions = NULL;
char *reponse, *p, *q, *r;
int l, i;

if(raa_current_db == NULL) return 0;
sock_printf(raa_current_db, "knowndbs\n" );
reponse = read_sock(raa_current_db);
if(reponse == NULL || strncmp(reponse, "nl=", 3) != 0) {
	return 0;
	}
nl = atoi(reponse + 3);
if(nl == 0) return 0;
names = (char **) malloc(nl * sizeof(char *));
descriptions = (char **) malloc(nl * sizeof(char *));
for(i = 0; i < nl; i++) {
	reponse = read_sock(raa_current_db);
	if(reponse == NULL) return 0;
	p = strchr(reponse, '|');
	if(p != NULL)  {
		*(p++) = 0;
		q = strchr(p, '|');
		if(q != NULL)  *(q++) = 0;
		}
	l = strlen(reponse);
	names[i] = (char *)malloc(l+1);
	strcpy(names[i], reponse);
	compact(names[i]);
	if(p != NULL && q != NULL) {
		while( (r=strchr(p, '\t')) != NULL) *r = ' ';
		majuscules(p); compact(p);
		l = strlen(q);
		descriptions[i] = (char *)malloc(l+20);
		descriptions[i][0] = 0;
		if(strcmp(p, "OFF") == 0) strcpy(descriptions[i], "(offline) ");
		strcat(descriptions[i], q);
		}
	else descriptions[i] = NULL;
	}
*pnames = names; *pdescriptions = descriptions;
return nl;
}


char *raa_short_descr(raa_db_access *raa_current_db, int seqnum, char *text, int maxlen, raa_long pinf, int div, char *name)
/*
to get a description of a sequence or of a subsequence
seqnum	the sequence number
text	the string to be loaded with description
maxlen	the max # of chars allowed in text (\0 is put but not counted in maxlen)
return value	a pointer to text 
*/
{
int l, deb;
char *p;

text[maxlen]=0;
strcpy(text, name);
l=strlen(text);
if(strchr(name, '.') != NULL) { /* subsequence */
	if( (p = raa_read_annots(raa_current_db, pinf, div)) == NULL) return text;
	p[20]=0;
	strcat(text,p+4);
	l=strlen(text);
	while(text[l-1]==' ') l--;
	text[l]=0;
	if( ( p = strchr(p + 21, '/') ) != NULL) {
		strncat(text, p, maxlen - l);
		l = strlen(text);
		if(l > 75) return text;
		}
	do	{
		p = raa_next_annots(raa_current_db, NULL);
		if( strcmptrail(p,20,NULL,0) && 
			strncmp(p,"FT        ",10) ) return text;
		}
	while(p[21]!='/');
	do	{
		strncat(text,p+20,maxlen-l);
		l=strlen(text);
		if(l>75) return text;
		p = raa_next_annots(raa_current_db, NULL);
		}
	while ( !strcmptrail(p,20,NULL,0) || 
				!strncmp(p,"FT        ",10) );
	}
else	{ /* parent sequence */
	if( (p = raa_read_annots(raa_current_db, pinf, div)) == NULL) return text;
	p = raa_next_annots(raa_current_db, NULL);
	if(raa_current_db->nbrf) {
		deb=17;
		}
	else	{
		deb=13;
		if(raa_current_db->embl || raa_current_db->swissprot) {
			while (strncmp(p,"DE",2)) {
				p = raa_next_annots(raa_current_db, NULL);
				}
			deb=6;
			}
		}
	do	{
		strncat(text,p+deb-2,maxlen-l);
		l=strlen(text);
		if(l>=77) return text;
		p = raa_next_annots(raa_current_db, NULL);
		}
	while( !strncmp(p,"  ",2) || !strncmp(p,"DE",2) );
	}
return text;
}


struct chain_void *raa_get_list_open_dbs(void)
{
return raa_list_open_dbs;
}


#define END_COORDINATE_TEST(line) strncmp(line, "extractseqs END.", 16)

static int *next_1_coordinate_set(raa_db_access *raa_current_db)
{
char  *p, *line, *q;
int  start, last, seqnum, elt, i, count;
int *table = NULL;

line = read_sock(raa_current_db);
if(END_COORDINATE_TEST(line) == 0) return NULL;
count = 0; p = line;
while(TRUE) {
	p = strchr(p+1, '|');
	if( p == NULL) break;
	count++;
	}
table = (int *)malloc( (3 * count + 1) * sizeof(int));
if(table == NULL) {
	do line = read_sock(raa_current_db);
	while(END_COORDINATE_TEST(line) != 0) ;
	return NULL;
	}
table[0] = count;
elt = 1;
for(i = 0; i < count; i++) {
	p = strchr(line, '|');
	if(p == NULL) break;
	q = strchr(line, '=');
	if(q == NULL || q > p) break;
	sscanf(q+1, "%d", &seqnum);
	q = strchr(q+1, '=');
	if(q == NULL || q > p) break;
	sscanf(q+1, "%d", &start);
	q = strchr(q+1, '=');
	if(q == NULL || q > p) break;
	sscanf(q+1, "%d", &last);
	table[elt++] = seqnum;
	table[elt++] = start;
	table[elt++] = last;
	line = p + 1;
	}
return table;
}


struct coord_series_struct {
	int count;
	int **table;
	int next;
	} ;

void *raa_prep_coordinates(raa_db_access *raa_current_db, int lrank, int seqnum, 
	char *operation, /* "simple","fragment","feature","region" */
	char *feature_name, char *bounds, char *min_bounds)
/*
only one of lrank and seqnum is != 0 to work on a sequence list or on an individual sequence
feature_name: used for operations feature and region, NULL otherwise
bounds: used for operations fragment and region, NULL otherwise
        syntax by examples "45,155"  "-100,100"  "-10,e+100"  "E-10,e+100"
min_bounds: NULL for operations other than fragment and region
            can be NULL for fragment and region and means min_bounds same as bounds
            if not NULL, same syntax as bounds
pcount: upon return, set to number of coordinate series

return value: NULL if error, or pointer to opaque data
*/
{
char message[200];
char *line;
int *v, maxi, **table, rank;
struct coord_series_struct *retval;
	
sprintf(message, "extractseqs&%s=%d&format=coordinates&operation=%s", 
	seqnum == 0 ? "lrank" : "seqnum", 
	seqnum == 0 ? lrank : seqnum, 
	operation);
sock_fputs(raa_current_db, message);
if(strcmp(operation, "feature") == 0 || strcmp(operation, "region") == 0) {
	sprintf(message, "&feature=%s", feature_name);
	sock_fputs(raa_current_db, message);
	}
if(strcmp(operation, "fragment") == 0 || strcmp(operation, "region") == 0) {
	sprintf(message, "&bounds=%s", bounds);
	sock_fputs(raa_current_db, message);
	}
if(min_bounds != NULL) {
	sprintf(message, "&minbounds=%s", min_bounds);
	sock_fputs(raa_current_db, message);
	}
sock_fputs(raa_current_db, "\n"); sock_flush(raa_current_db);
line = read_sock(raa_current_db);
if(strcmp(line, "code=0") != 0) {
	return NULL;
	}

maxi = 100; rank = 0;
table = (int **)malloc(maxi*sizeof(int *));
if(table == NULL) {
	do line = read_sock(raa_current_db);
	while(END_COORDINATE_TEST(line) != 0) ;
	return NULL;
	}
while( (v = next_1_coordinate_set(raa_current_db)) != NULL) {
	if(rank >= maxi) {
		int **vv;
		vv = (int **)realloc(table, 2*maxi*sizeof(int *));
		if(vv == NULL) {
			do line = read_sock(raa_current_db);
			while(END_COORDINATE_TEST(line) != 0) ;
			break;
			}
		maxi = 2*maxi;
		table = vv;
		}
	table[rank++] = v;
	}
table = realloc(table, rank*sizeof(int *));
retval = (struct coord_series_struct *)malloc(sizeof(struct coord_series_struct));
if(retval == NULL) return NULL;
retval->count = rank;
retval->table = table;
retval->next = 0;
return retval;
}


int *raa_1_coordinate_set(void *v)
/*
to be called repetitively as
table = raa_1_coordinate_set(v);
until returns NULL
with the opaque pointer returned by the raa_prep_coordinates call

returns int array table in private memory containing 1 + 3*table[0] elements
count = table[0] ; 
j = 0;
for(i=0; i < count; i++) {
	table[j+1] is the acnuc number of the sequence
	table[j+2] is the start position in this sequence
	table[j+3] is the end position in this sequence
	j += 3;
	}
start position > end position <==> fragment is on the complementary strand of the acnuc seq
returns NULL when all coordinate series have been processed
*/
{
int *retval, i;
struct coord_series_struct *s = (struct coord_series_struct *)v;

if(s->next >= s->count) {
	for(i=0; i < s->count; i++) free(s->table[i]);
	free(s->table);
	free(s);
	return NULL;
	}	
retval = s->table[(s->next)++];
return retval;
}


char *raa_translate_cds(raa_db_access *raa_current_db, int seqnum)
/* traduction d'un cds avec codon initiateur traite et * internes ==> X
rendue dans memoire allouee ici qu'il ne faut pas modifier
retour NULL si pb lecture de la seq
*/
{
static char *buffer = NULL;
static int lbuffer = 0;
int debut_codon, longueur, pos, code, phase;
char codon[4], *p;

raa_readsub(raa_current_db, seqnum,&longueur,NULL,NULL,NULL,NULL,&phase,&code);
debut_codon = phase + 1;
longueur = (longueur - debut_codon + 1)/3;
if(longueur > lbuffer) {
	if(buffer != NULL) free(buffer);
	buffer = (char *)malloc(longueur + 1);
	lbuffer = longueur;
	}
if(buffer == NULL) {lbuffer = 0; return NULL; }
buffer[0] = raa_translate_init_codon(raa_current_db, seqnum);
debut_codon += 3;
for(pos = 1; pos < longueur; pos++) {
	if( raa_gfrag(raa_current_db, seqnum, debut_codon, 3, codon) == 0) return NULL;
	buffer[pos] = codaa(codon,code);
	debut_codon += 3;
	}
buffer[longueur] = 0;
while( (p = strchr(buffer, '*') ) != NULL && p - buffer < longueur - 1 )
	*p = 'X';
return buffer;
}


char raa_translate_init_codon(raa_db_access *raa_current_db, int numseq)
{
char codon[4];
int point, special_init = TRUE, val, gc, phase;
static int num_5_partial = 0;

if(num_5_partial == 0) num_5_partial = raa_iknum(raa_current_db, "5'-PARTIAL", raa_key);
raa_readsub(raa_current_db, numseq, NULL, NULL,NULL, &point, NULL, &phase, &gc);
if(phase != 0) special_init = FALSE;
else	{ /* la seq est-elle 5'-PARTIAL ? */
	while(point != 0) {
		point = raa_readshrt(raa_current_db, point, &val);
		if(val == num_5_partial) {
			special_init = FALSE;
			break;
			}
		}
	}
raa_gfrag(raa_current_db, numseq, phase + 1, 3, codon);
if(special_init)  /* traduction speciale du codon initiateur */
	return init_codon_to_aa(codon, gc);
else	return codaa(codon, gc);
}


static void ajout_synonyme(raa_node *secondaire, raa_node *principal)
{
raa_node *next;

if(principal->syno == NULL) {
	principal->syno = secondaire;
	secondaire->syno = principal;
	}
else	{
	next = principal->syno;
	principal->syno = secondaire;
	secondaire->syno = next;
	}	
}


static void ajout_branche(raa_node *pere, raa_node *fils)
{
struct raa_pair *point, *nouveau;

nouveau = (struct raa_pair *)mycalloc(1,sizeof(struct raa_pair));
nouveau->value = fils;
if( (point = pere->list_desc) == NULL) {
	pere->list_desc = nouveau;
	}
else	{
	while(point->next != NULL) point = point->next;
	point->next = nouveau;
	}
}


static void raa_decode_desc_arbre(char *reponse, raa_node **tab_noeud)
/* reponse contient
rank&pere&count&"...name..."&"...libel..."
synonyme est identifie par pere < 0 et -pere = son principal
*/
{ 
int num, pere, count, l;
char *p, *q, *name, *libel;


num = atoi(reponse);
p = strchr(reponse, '&');
pere = atoi(p + 1);
p = strchr(p+1, '&');
count = atoi(p + 1);
/* ne pas brancher un noeud deja branche ailleurs auparavant */
if(tab_noeud[num] != NULL ) return;
tab_noeud[num] = (raa_node *)calloc(1, sizeof(raa_node));
tab_noeud[num]->rank = num;
if(pere < 0) { /* un synonyme */
	ajout_synonyme(tab_noeud[num], tab_noeud[-pere]);
	}
else 	{
	tab_noeud[num]->count = count;
	tab_noeud[num]->parent = tab_noeud[pere];
	if(num != 2) ajout_branche(tab_noeud[pere], tab_noeud[num]);
	}

p = strchr(p+1, '&') + 1;
q = p;
do 	{
	q++;
    	if(*q == 0) break;
	}
while(*q != '"' || *(q-1) == '\\');
l = q-p+1;
name = (char *)malloc(l+1);
strncpy(name, p, l); name[l] = 0;
unprotect_quotes(name);
tab_noeud[num]->name = name;
q++;
if(*q == '&') {
	p = q+1;
	q = p;
	do 	{
		q++;
    		if(*q == 0) break;
		}
	while(*q != '"' || *(q-1) == '\\');
	l = q-p+1;
	libel = (char *)malloc(l+1);
	strncpy(libel, p, l); libel[l] = 0;
	unprotect_quotes(libel);
	tab_noeud[num]->libel = libel;
	p = (char *)malloc(l+1);
	strcpy(p, libel);
	majuscules(p);
	tab_noeud[num]->libel_upcase = p;
	if(strncmp(p, "ID:", 3) == 0 || (p = strstr(p, "|ID:")) != NULL) {
		p += 2; if(*p != ':') p++;
		sscanf(p + 1, "%d", &(tab_noeud[num]->tid) );
		}
	}
}


static void raa_calc_taxo_count(raa_node *racine)
{
struct raa_pair *paire;
int count = 0;

paire = racine->list_desc;
while(paire != NULL) {
	raa_calc_taxo_count(paire->value);
	count += paire->value->count;
	paire = paire->next;
	}
racine->count += count;
}


int raa_loadtaxonomy(raa_db_access  *raa_current_db, char *rootname, 
	int (*progress_function)(int, void *), void *progress_arg, 
	int (*need_interrupt_f)(void *), void *interrupt_arg)
/* charge la taxo complete dans raa_current_db->sp_tree et rend 0 ssi OK */
{
int totspec, i, maxtid;
raa_node **tab_noeud;
struct raa_pair *pair, *pair2;
char *reponse;
int count, pourcent, prev_pourcent = 0;
void *opaque;
int interrupted;

if(raa_current_db == NULL) return 1;
if(raa_current_db->sp_tree != NULL) return 0;
interrupted = FALSE;
sock_fputs(raa_current_db, "zlibloadtaxonomy\n");
sock_flush(raa_current_db);
/* reply: 
<start of compressed data using zlib >
code=0&total=xx
rank&pere&count&"...name..."&"...libel..."
loadtaxonomy END.
<end of compressed data, back to normal data >
*/
opaque = prepare_sock_gz_r( raa_current_db->raa_sockfdr );
reponse = z_read_sock(opaque);
if(reponse == NULL || strncmp(reponse, "code=0&total=", 13) != 0) {
	return 1;
	}
totspec = atoi(reponse + 13);
tab_noeud = (raa_node **)calloc(totspec + 1, sizeof(raa_node *));
count = 0;
while(TRUE) {
	reponse = z_read_sock(opaque);
	if(strcmp(reponse, "loadtaxonomy END.") == 0) {
		if(interrupted && (tab_noeud != NULL) ) {
			for(i = 2; i <= totspec; i++) {
				if(tab_noeud[i] == NULL) continue;
				if(tab_noeud[i]->name != NULL) free(tab_noeud[i]->name);
				if(tab_noeud[i]->libel != NULL) free(tab_noeud[i]->libel);
				if(tab_noeud[i]->libel_upcase != NULL) free(tab_noeud[i]->libel_upcase);
				pair = tab_noeud[i]->list_desc; 
				while(pair != NULL) {
					pair2 = pair->next;
					free(pair);
					pair = pair2;
					}
				free(tab_noeud[i]);
				}
			free(tab_noeud);
			tab_noeud = NULL;
			/* just to consume ESC that may have arrived after loadtaxonomy END. */
			sock_fputs(raa_current_db, "null_command\n");
			read_sock(raa_current_db);
			}
		break;
		}
	if(tab_noeud != NULL) raa_decode_desc_arbre(reponse, tab_noeud);
	pourcent = ((++count) * 100) / totspec; 
	if(pourcent > prev_pourcent) {
		prev_pourcent = pourcent;
		if( progress_function != NULL && progress_function(pourcent, progress_arg) ) {
			if( need_interrupt_f != NULL && (! interrupted) && need_interrupt_f(interrupt_arg) ) {
				sock_fputs(raa_current_db, "\033" /* esc */);
				sock_flush(raa_current_db);
				interrupted = TRUE;
				}
			}
		}
	}
close_sock_gz_r(opaque);
if(tab_noeud != NULL) {
	raa_calc_taxo_count(tab_noeud[2]);
	free(tab_noeud[2]->name);
	tab_noeud[2]->name = strdup(rootname);
	maxtid = 0;
	for(i = 2; i <= totspec; i++) if(tab_noeud[i] != NULL && tab_noeud[i]->tid > maxtid) maxtid = tab_noeud[i]->tid;
	raa_current_db->tid_to_rank = (int *)calloc(maxtid + 1, sizeof(int));
	if(raa_current_db->tid_to_rank != NULL) {
		raa_current_db->max_tid = maxtid;
		for(i = 2; i <= totspec; i++) {
			if(tab_noeud[i] != NULL && tab_noeud[i]->tid != 0) raa_current_db->tid_to_rank[tab_noeud[i]->tid] = 
				tab_noeud[i]->rank;
			}
		}
	raa_current_db->sp_tree = tab_noeud;
	}
return (tab_noeud == NULL ? 1 : 0);
}


char *raa_get_taxon_info(raa_db_access *raa_current_db, char *name, int rank, int tid, int *p_rank, 
	int *p_tid, int *p_parent, struct raa_pair **p_desc_list)
/*
from a taxon identified by its name or, if name is NULL, by its rank or, if rank is 0, by its taxon ID (tid)
computes :
- if p_rank != NULL, the taxon rank in *p_rank 
- if p_tid != NULL, the taxon ID in *p_tid 
- if p_parent != NULL, the taxon's parent rank in *p_parent (2 indicates that taxon is at top level)
- if p_desc_list != NULL, the start of the linked chain of taxon's descending taxa in *p_desc_list
returns the taxon name, or NULL if any error
*/
{
int totspec;

if(raa_current_db == NULL) return NULL;
if(raa_current_db->sp_tree == NULL) raa_loadtaxonomy(raa_current_db, "root", NULL, NULL, NULL, NULL);
if(raa_current_db->sp_tree == NULL) return NULL;
totspec = raa_read_first_rec(raa_current_db, raa_spec);
if(name != NULL) {
	name = strdup(name);
	if(name == NULL) return NULL;
	trim_key(name); majuscules(name);
	for(rank = 3; rank <= totspec; rank++) {
		if(raa_current_db->sp_tree[rank] != NULL && strcmp(name, raa_current_db->sp_tree[rank]->name) == 0) break;
		}
	free(name);
	}
if(name == NULL && rank == 0 && tid >= 1 && tid <= raa_current_db->max_tid) rank = raa_current_db->tid_to_rank[tid];
if(rank > totspec || rank < 2 || raa_current_db->sp_tree[rank] == NULL) return NULL;
if(rank != 2) {
	while(raa_current_db->sp_tree[rank]->parent == NULL) {
		rank = raa_current_db->sp_tree[rank]->syno->rank;
		}
	}
if(p_rank != NULL) *p_rank = rank;
if(p_tid != NULL) *p_tid = raa_current_db->sp_tree[rank]->tid;
if(p_parent != NULL) *p_parent = ( raa_current_db->sp_tree[rank]->parent != NULL ? 
										raa_current_db->sp_tree[rank]->parent->rank : 0);
if(p_desc_list != NULL) *p_desc_list = raa_current_db->sp_tree[rank]->list_desc;
return raa_current_db->sp_tree[rank]->name;
}


char *raa_getattributes_both(raa_db_access *raa_current_db, const char *id, int rank,
	int *prank, int *plength, int *pframe, int *pgc, char **pacc, char **pdesc, char **pspecies, char **pseq)
/*
for a sequence identified by name or acc. no. (id != NULL) or by rank
returns rank, name, accession, length, frame, acnuc genetic code ID,
one-line description, species, and full sequence.
return value: NULL if not found or name (in private memory)
pacc, pdesc, pspecies and pseq point to private memory upon return
prank, plength, pframe, pgc, pacc, pdesc, pspecies, pseq can be NULL is no such information is needed
*/
{
Reponse *rep;
char *p, *reponse;
int err;
static char mnemo[WIDTH_MAX], species[WIDTH_MAX], access[WIDTH_MAX], descript[WIDTH_MAX];

if(raa_current_db == NULL) return NULL;
if(id != NULL) sock_printf(raa_current_db, "getattributes&id=%s&seq=%c\n", id, pseq == NULL ? 'F' : 'T');
else sock_printf(raa_current_db, "getattributes&rank=%d&seq=%c\n", rank, pseq == NULL ? 'F' : 'T');
reponse = read_sock(raa_current_db);
if(reponse == NULL) {
	return NULL;
	}
rep=initreponse();
parse(reponse,rep);
p=val(rep,"code");
err = atoi(p);
free(p);
if(err == 0) {
	if(prank != NULL) {
		p=val(rep,"rank");
		*prank = atoi(p);
		free(p);
		}
	if(plength != NULL) {
		p=val(rep,"length");
		*plength = atoi(p);
		free(p);
		}
	if(pframe != NULL) {
		p=val(rep,"fr");
		if(p != NULL) {
			*pframe = atoi(p);
			free(p);
			}
		else *pframe = 0;
		}
	if(pgc != NULL) {
		p=val(rep,"gc");
		if(p != NULL) {
			*pgc = atoi(p);
			free(p);
			}
		else *pgc = 0;
		}
	p = val(rep, "name");
	strcpy(mnemo, p);
	free(p);
	if(pacc != NULL) {
		p = val(rep, "acc");
		strcpy(access, p);
		free(p);
		*pacc = access;
		}
	if(pspecies != NULL) {
		p = val(rep, "spec");
		strcpy(species, p);
		free(p);
		*pspecies = species;
		p = species;
		while(*(++p) != 0) *p = tolower(*p);
		}
	if(pdesc != NULL) {
		p = val(rep, "descr");
		strcpy(descript, p);
		free(p);
		*pdesc = descript;
		}
	if(pseq != NULL) {
		*pseq = read_sock(raa_current_db); 
		if(*pseq != NULL) {
			*pseq += 4; /* seq=xxxx */
			}
		}
	err = 0;
	}
clear_reponse(rep);
return (err ? NULL : mnemo);
}


char *raa_getattributes(raa_db_access *raa_current_db, const char *id,
						int *prank, int *plength, int *pframe, int *pgc, char **pacc, char **pdesc, char **pspecies, char **pseq)
{
	return raa_getattributes_both(raa_current_db, id, 0, prank, plength, pframe, pgc, pacc, pdesc, pspecies, pseq);
}


char *raa_seqrank_attributes(raa_db_access *raa_current_db, int rank,
							 int *plength, int *pframe, int *pgc, char **pacc, char **pdesc, char **pspecies, char **pseq)
{
	return raa_getattributes_both(raa_current_db, NULL, rank, NULL, plength, pframe, pgc, pacc, pdesc, pspecies, pseq);
}



