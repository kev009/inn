/*  $Id$
**
**  Common code for authenticators and resolvers.
**
**  Collects common code to read information from nnrpd that should be done
**  the same for all authenticators, and common code to get information about
**  the incoming connection.
*/

#include "config.h"
#include "clibrary.h"
#include "libinn.h"

#include "libauth.h"
#include "inn/messages.h"

#define NAMESTR "ClientAuthname: "
#define PASSSTR "ClientPassword: "

#define CLIHOST "ClientHost: "
#define CLIIP "ClientIP: "
#define CLIPORT "ClientPort: "
#define LOCIP "LocalIP: "
#define LOCPORT "LocalPort: "

#ifdef HAVE_INET6
# include <netdb.h>
#endif

/* Main loop.  If res != NULL, expects to get resolver info from nnrpd, and
   writes it into the struct.  If auth != NULL, expects to get authentication
   info from nnrpd, and writes it into the struct. */

static bool
get_connection_info(FILE *stream, struct res_info *res, struct auth_info *auth)
{
    char buff[SMBUF];
    size_t length;
    char *cip = NULL, *sip = NULL, *cport = NULL, *sport = NULL;
#ifdef HAVE_INET6
    struct addrinfo *r, hints;
#else
    struct sockaddr_in *loc_sin, *cli_sin;
#endif

    /* Zero fields first (anything remaining NULL after is missing data) */
    if (res != NULL) {
        res->clienthostname = NULL;
        res->client = NULL;
        res->local = NULL;
    }
    if (auth != NULL) {
        auth->username = NULL;
        auth->password = NULL;
    }

    /* Read input from nnrpd a line at a time, stripping \r\n. */
    while (fgets(buff, sizeof(buff), stream) != NULL) {
        length = strlen(buff);
        if (length == 0 || buff[length - 1] != '\n')
            goto error;
        buff[length - 1] = '\0';
        if (length > 1 && buff[length - 2] == '\r')
            buff[length - 2] = '\0';

        /* Parse */
        if (strncmp(buff, ".", 2) == 0)
            break;
        else if (auth != NULL && strncmp(buff, NAMESTR, strlen(NAMESTR)) == 0)
            auth->username = xstrdup(buff + strlen(NAMESTR));
        else if (auth != NULL && strncmp(buff, PASSSTR, strlen(PASSSTR)) == 0)
            auth->password = xstrdup(buff + strlen(PASSSTR));
        else if (res != NULL && strncmp(buff, CLIHOST, strlen(CLIHOST)) == 0)
            res->clienthostname = xstrdup(buff + strlen(CLIHOST));
        else if (res != NULL && strncmp(buff, CLIIP, strlen(CLIIP)) == 0)
            cip = xstrdup(buff + strlen(CLIIP));
        else if (res != NULL && strncmp(buff, CLIPORT, strlen(CLIPORT)) == 0)
            cport = xstrdup(buff + strlen(CLIPORT));
        else if (res != NULL && strncmp(buff, LOCIP, strlen(LOCIP)) == 0)
            sip = xstrdup(buff + strlen(LOCIP));
        else if (res != NULL && strncmp(buff, LOCPORT, strlen(LOCPORT)) == 0)
            sport = xstrdup(buff + strlen(LOCPORT));
        else {
            /**** We just ignore excess fields for now ****/

            /* warn("libauth: unexpected data from nnrpd: \"%s\"", buff); */
            /* goto error; */
        }
    }

    /* If some field is missing, free the rest and error out. */
    if (auth != NULL && (auth->username == NULL || auth->password == NULL)) {
        warn("libauth: requested authenticator data not sent by nnrpd");
        goto error;
    }
    if (res != NULL && (res->clienthostname == NULL || cip == NULL ||
                cport == NULL || sip == NULL || sport == NULL)) {
        warn("libauth: requested resolver data not sent by nnrpd");
        goto error;
    }

    /* Generate sockaddrs from IP and port strings */
    if (res != NULL) {
#ifdef HAVE_INET6
        /* sockaddr_in6 may be overkill for PF_INET case, but oh well */
        res->client = xcalloc(1, sizeof(struct sockaddr_in6));
        res->local = xcalloc(1, sizeof(struct sockaddr_in6));

        memset( &hints, 0, sizeof( hints ) );
        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_socktype = SOCK_STREAM;

        hints.ai_family = strchr( cip, ':' ) != NULL ? PF_INET6 : PF_INET;
        if( getaddrinfo( cip, cport, &hints, &r ) != 0)
            goto error;
        if( r->ai_addrlen > sizeof(struct sockaddr_in6) )
            goto error;
        memcpy( res->client, r->ai_addr, r->ai_addrlen );
        freeaddrinfo( r );

        hints.ai_family = strchr( sip, ':' ) != NULL ? PF_INET6 : PF_INET;
        if( getaddrinfo( sip, sport, &hints, &r ) != 0)
            goto error;
        if( r->ai_addrlen > sizeof(struct sockaddr_in6) )
            goto error;
        memcpy( res->local, r->ai_addr, r->ai_addrlen );
        freeaddrinfo( r );
#else
        res->client = xcalloc(1, sizeof(struct sockaddr_in));
        res->local = xcalloc(1, sizeof(struct sockaddr_in));

        cli_sin = (struct sockaddr_in *)(res->client);
        loc_sin = (struct sockaddr_in *)(res->local);
        cli_sin->sin_family = AF_INET;
        if (!inet_aton(cip, &cli_sin->sin_addr))
            goto error;
        cli_sin->sin_port = htons( atoi(cport) );

        loc_sin->sin_family = AF_INET;
        if (!inet_aton(sip, &loc_sin->sin_addr))
            goto error;
        loc_sin->sin_port = htons( atoi(sport) );
#endif

        free(sip);
        free(sport);
        free(cip);
        free(cport);
    }

    return true;

error:
    if (auth != NULL && auth->username != NULL)     free(auth->username);
    if (auth != NULL && auth->password != NULL)     free(auth->password);
    if (res != NULL && res->clienthostname != NULL) free(res->clienthostname);
    if (res != NULL && res->client != NULL)         free(res->client);
    if (res != NULL && res->local != NULL)          free(res->local);
    if (sip != NULL)                                free(sip);
    if (sport != NULL)                              free(sport);
    if (cip != NULL)                                free(cip);
    if (cport != NULL)                              free(cport);
    return false;
}


/* Wrappers to read information from nnrpd, returning an allocated struct on
   success. */

struct res_info *
get_res_info(FILE *stream) {
    struct res_info *res = xmalloc(sizeof(struct res_info));

    if(get_connection_info(stream, res, NULL))
        return res;

    free(res);
    return NULL;
}


struct auth_info *
get_auth_info(FILE *stream) {
    struct auth_info *auth = xmalloc(sizeof(struct auth_info));

    if(get_connection_info(stream, NULL, auth))
        return auth;

    free(auth);
    return NULL;
}

void
free_res_info(struct res_info *res) {
    if(res == NULL)
        return;
    if(res->client != NULL)             free(res->client);
    if(res->local != NULL)              free(res->local);
    if(res->clienthostname != NULL)     free(res->clienthostname);
    free(res);
}

void
free_auth_info(struct auth_info *auth) {
    if(auth == NULL)
        return;
    if(auth->username != NULL)          free(auth->username);
    if(auth->password != NULL)          free(auth->password);
    free(auth);
}
