/* $Id: icq_response.c,v 1.207 2005/04/09 11:21:54 kuhlmann Exp $ */
/* Copyright: This file may be distributed under version 2 of the GPL licence. */

#include "micq.h"
#include "icq_response.h"
#include "util_rl.h"
#include "tabs.h"
#include "contact.h"
#include "server.h"
#include "util.h"
#include "conv.h"
#include "preferences.h"
#include "connection.h"

#ifndef ENABLE_TCL
#define TCLMessage(from, text)
#define TCLEvent(from, type, data)
#endif

/*
 * Inform that a user went online
 */
void IMOnline (Contact *cont, Connection *conn, UDWORD status)
{
    Event *egevent;
    UDWORD old;

    if (!cont)
        return;

    if (status == cont->status)
        return;
    
    if (status == STATUS_OFFLINE)
    {
        IMOffline (cont, conn);
        return;
    }
    
    OptSetVal (&cont->copts, CO_TIMESEEN, time (NULL));

    old = cont->status;
    cont->status = status;
    cont->oldflags &= ~CONT_SEENAUTO;
    
    putlog (conn, NOW, cont, status, old != STATUS_OFFLINE ? LOG_CHANGE : LOG_ONLINE, 0xFFFF, "");
 
    if (!cont->group || ContactPrefVal (cont, CO_IGNORE)
        || (!ContactPrefVal (cont, CO_SHOWONOFF)  && old == STATUS_OFFLINE)
        || (!ContactPrefVal (cont, CO_SHOWCHANGE) && old != STATUS_OFFLINE)
        || (~conn->connect & CONNECT_OK))
        return;
    
    if ((egevent = QueueDequeue2 (conn, QUEUE_DEP_OSCARLOGIN, 0, 0)))
    {
        egevent->due = time (NULL) + 3;
        QueueEnqueue (egevent);
        return;
    }

    if (prG->event_cmd && *prG->event_cmd)
        EventExec (cont, prG->event_cmd, !~old ? 2 : 5, status, NULL);

    rl_printf ("%s %s%*s%s ", s_now, COLCONTACT, uiG.nick_len + s_delta (cont->nick), cont->nick, COLNONE);
    rl_printf (~old ? i18n (2212, "changed status to %s") : i18n (2213, "logged on (%s)"), s_status (status));
    if (cont->version && !~old)
        rl_printf (" [%s]", cont->version);
    if ((status & STATUSF_BIRTH) && (!(old & STATUSF_BIRTH) || !~old))
        rl_printf (" (%s)", i18n (2033, "born today"));
    rl_print (".\n");

    if (prG->verbose && !~old && cont->dc)
    {
        rl_printf ("    %s %s / ", i18n (1642, "IP:"), s_ip (cont->dc->ip_rem));
        rl_printf ("%s:%ld    %s %d    %s (%d)\n", s_ip (cont->dc->ip_loc),
            cont->dc->port, i18n (1453, "TCP version:"), cont->dc->version,
            cont->dc->type == 4 ? i18n (1493, "Peer-to-Peer") : i18n (1494, "Server Only"),
            cont->dc->type);
    }
    TCLEvent (cont, "status", s_status (status));
}

/*
 * Inform that a user went offline
 */
void IMOffline (Contact *cont, Connection *conn)
{
    UDWORD old;

    if (!cont)
        return;
    
    if (cont->status == STATUS_OFFLINE)
        return;

    putlog (conn, NOW, cont, STATUS_OFFLINE, LOG_OFFLINE, 0xFFFF, "");

    OptSetVal (&cont->copts, CO_TIMESEEN, time (NULL));
    old = cont->status;
    cont->status = STATUS_OFFLINE;

    if (!cont->group || ContactPrefVal (cont, CO_IGNORE) || !ContactPrefVal (cont, CO_SHOWONOFF))
        return;

    if (prG->event_cmd && *prG->event_cmd)
        EventExec (cont, prG->event_cmd, 3, old, NULL);
 
    rl_printf ("%s %s%*s%s %s\n",
             s_now, COLCONTACT, uiG.nick_len + s_delta (cont->nick), cont->nick,
             COLNONE, i18n (1030, "logged off."));
    TCLEvent (cont, "status", "logged_off");
}

#define MSGICQACKSTR   ">>>"
#define MSGICQ5ACKSTR  "> >"
#define MSGICQRECSTR   "<<<"
#define MSGTCPACKSTR   ConvTranslit ("\xc2\xbb\xc2\xbb\xc2\xbb", "}}}")
#define MSGTCPRECSTR   ConvTranslit ("\xc2\xab\xc2\xab\xc2\xab", "{{{")
#define MSGSSLACKSTR   ConvTranslit ("\xc2\xbb%\xc2\xbb", "}%}")
#define MSGSSLRECSTR   ConvTranslit ("\xc2\xab%\xc2\xab", "{%{")
#define MSGTYPE2ACKSTR ConvTranslit (">>\xc2\xbb", ">>}")
#define MSGTYPE2RECSTR ConvTranslit ("\xc2\xab<<", "{<<")

/*
 * Central entry point for protocol triggered output.
 */
void IMIntMsg (Contact *cont, Connection *conn, time_t stamp, UDWORD tstatus, int_msg_t type, const char *text, Opt *opt)
{
    const char *line, *opt_text;
    const char *col = COLCONTACT;
    UDWORD opt_port, opt_bytes;
    char *p, *q;

    if (!cont || ContactPrefVal (cont, CO_IGNORE))
    {
        OptD (opt);
        return;
    }
    if (!OptGetStr (opt, CO_MSGTEXT, &opt_text))
        opt_text = "";
    if (!OptGetVal (opt, CO_PORT, &opt_port))
        opt_port = 0;
    if (!OptGetVal (opt, CO_BYTES, &opt_bytes))
        opt_bytes = 0;

    switch (type)
    {
        case INT_FILE_ACKED:
            line = s_sprintf (i18n (2462, "File transfer %s to port %s%ld%s."),
                              s_qquote (opt_text), COLQUOTE, opt_port, COLNONE);
            break;
        case INT_FILE_REJED:
            line = s_sprintf (i18n (2463, "File transfer %s rejected by peer: %s."),
                              s_qquote (opt_text), s_wordquote (text));
            break;
        case INT_FILE_ACKING:
            line = s_sprintf (i18n (2464, "Accepting file %s (%s%ld%s bytes)."),
                              s_qquote (opt_text), COLQUOTE, opt_bytes, COLNONE);
            break;
        case INT_FILE_REJING:
            line = s_sprintf (i18n (2465, "Refusing file request %s (%s%ld%s bytes): %s."),
                              s_qquote (opt_text), COLQUOTE, opt_bytes, COLNONE, s_wordquote (text));
            break;
        case INT_CHAR_REJING:
            line = s_sprintf (i18n (2466, "Refusing chat request (%s/%s) from %s%s%s."),
                              p = strdup (s_qquote (opt_text)), q = strdup (s_qquote (text)),
                              COLCONTACT, cont->nick, COLNONE);
            free (p);
            free (q);
            break;
        case INT_MSGTRY_TYPE2:
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", i18n (2293, "--="), COLSINGLE, text);
            break;
        case INT_MSGTRY_DC:
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", i18n (2294, "==="), COLSINGLE, text);
            break;
        case INT_MSGACK_TYPE2:
            col = COLACK;
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", MSGTYPE2ACKSTR, COLSINGLE, text);
            break;
        case INT_MSGACK_DC:
            col = COLACK;
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", MSGTCPACKSTR, COLSINGLE, text);
            break;
#ifdef ENABLE_SSL
        case INT_MSGACK_SSL:
            col = COLACK;
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", MSGSSLACKSTR, COLSINGLE, text);
            break;
#endif
        case INT_MSGACK_V8:
            col = COLACK;
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", MSGICQACKSTR, COLSINGLE, text);
            break;
        case INT_MSGACK_V5:
            col = COLACK;
            line = ContactPrefVal (cont, CO_HIDEACK) ? NULL : s_sprintf ("%s%s %s", MSGICQ5ACKSTR, COLSINGLE, text);
            break;
        default:
            line = "";
    }

    if (line)
    {
        if (tstatus != STATUS_OFFLINE && (!cont || cont->status == STATUS_OFFLINE || !cont->group))
            rl_printf ("(%s) ", s_status (tstatus));
        
        rl_printf ("%s ", s_time (&stamp));
        if (cont)
            rl_printf ("%s%*s%s ", col, uiG.nick_len + s_delta (cont->nick), cont->nick, COLNONE);
        
        if (prG->verbose > 1)
            rl_printf ("<%d> ", type);

        for (p = q = strdup (line); *q; q++)
            if (*q == (char)0xfe)
                *q = '*';
        rl_print (p);
        rl_print ("\n");
        free (p);
    }

    OptD (opt);
    HistMsg (conn, cont, stamp == NOW ? time (NULL) : stamp, text, HIST_OUT);
}

#define HISTSIZE 100

struct History_s
{
    Connection *conn;
    Contact *cont;
    time_t stamp;
    char *msg;
    UWORD inout;
};
typedef struct History_s History;

static History hist[HISTSIZE];

/*
 * History
 */
void HistMsg (Connection *conn, Contact *cont, time_t stamp, const char *msg, UWORD inout)
{
    int i, j, k;

    if (hist[HISTSIZE - 1].conn && hist[0].conn)
    {
        free (hist[0].msg);
        for (i = 0; i < HISTSIZE - 1; i++)
            hist[i] = hist[i + 1];
        hist[HISTSIZE - 1].conn = NULL;
    }

    for (i = k = j = 0; j < HISTSIZE - 1 && hist[j].conn; j++)
        if (cont == hist[j].cont)
        {
            if (!k)
                i = j;
            if (++k == 10)
            {
                free (hist[i].msg);
                for ( ; i < HISTSIZE - 1; i++)
                    hist[i] = hist[i + 1];
                hist[HISTSIZE - 1].conn = NULL;
                j--;
            }
        }
    
    hist[j].conn = conn;
    hist[j].cont = cont;
    hist[j].stamp = stamp;
    hist[j].msg = strdup (msg);
    hist[j].inout = inout;
}

void HistShow (Contact *cont)
{
    int i;
    
    for (i = 0; i < HISTSIZE; i++)
        if (hist[i].conn && (!cont || hist[i].cont == cont))
            rl_printf ("%s%s %s%*s %s%s %s" COLMSGINDENT "%s\n",
                       COLDEBUG, s_time (&hist[i].stamp),
                       hist[i].inout == HIST_IN ? COLINCOMING : COLACK,
                       uiG.nick_len + s_delta (hist[i].cont->nick),
                       hist[i].cont->nick, hist[i].inout == HIST_IN ? "<-" : "->",
                       COLNONE, COLMESSAGE, hist[i].msg);
}

/*
 * Central entry point for incoming messages.
 */
void IMSrvMsg (Contact *cont, Connection *conn, time_t stamp, Opt *opt)
{
    const char *tmp, *tmp2, *tmp3, *tmp4, *tmp5, *tmp6;
    char *cdata;
    const char *opt_text, *carr;
    UDWORD opt_type, opt_origin, opt_status, opt_bytes, opt_ref, j;
    int i;
    
    if (!cont)
    {
        OptD (opt);
        return;
    }

    if (!OptGetStr (opt, CO_MSGTEXT, &opt_text))
        opt_text = "";
    cdata = strdup (opt_text);
    while (*cdata && strchr ("\n\r", cdata[strlen (cdata) - 1]))
        cdata[strlen (cdata) - 1] = '\0';
    if (!OptGetVal (opt, CO_MSGTYPE, &opt_type))
        opt_type = MSG_NORM;
    if (!OptGetVal (opt, CO_ORIGIN, &opt_origin))
        opt_origin = CV_ORIGIN_v5;

    carr = (opt_origin == CV_ORIGIN_dc) ? MSGTCPRECSTR :
#ifdef ENABLE_SSL
           (opt_origin == CV_ORIGIN_ssl) ? MSGSSLRECSTR :
#endif
           (opt_origin == CV_ORIGIN_v8) ? MSGTYPE2RECSTR : MSGICQRECSTR;

    putlog (conn, stamp, cont,
        OptGetVal (opt, CO_STATUS, &opt_status) ? opt_status : STATUS_OFFLINE, 
        (opt_type == MSG_AUTH_ADDED) ? LOG_ADDED : LOG_RECVD, opt_type,
        cdata);
    
    if (ContactPrefVal (cont, CO_IGNORE))
    {
        free (cdata);
        OptD (opt);
        return;
    }

    TabAddIn (cont);

    if (uiG.idle_flag)
    {
        if ((cont != uiG.last_rcvd) || !uiG.idle_uins || !uiG.idle_msgs)
            s_repl (&uiG.idle_uins, s_sprintf ("%s %s", uiG.idle_uins && uiG.idle_msgs ? uiG.idle_uins : "", cont->nick));

        uiG.idle_msgs++;
        ReadLinePromptSet (s_sprintf ("[%s%ld%s%s]%s%s",
                           COLINCOMING, uiG.idle_msgs, uiG.idle_uins,
                           COLNONE, COLSERVER, i18n (2467, "mICQ>")));
    }

    if (prG->flags & FLAG_AUTOFINGER && ~cont->updated & UPF_AUTOFINGER &&
        ~cont->updated & UPF_SERVER && ~cont->updated & UPF_DISC)
    {
        cont->updated |= UPF_AUTOFINGER;
        IMCliInfo (conn, cont, 0);
    }

#ifdef MSGEXEC
    if (prG->event_cmd && *prG->event_cmd)
        EventExec (cont, prG->event_cmd, 1, opt_type, opt_text);
#endif
    if (uiG.nick_len < 4)
        uiG.nick_len = 4;
    rl_printf ("\a%s %s%*s%s ", s_time (&stamp), COLINCOMING, uiG.nick_len + s_delta (cont->nick), cont->nick, COLNONE);
    
    if (OptGetVal (opt, CO_STATUS, &opt_status) && (!cont || cont->status != opt_status || !cont->group))
        rl_printf ("(%s) ", s_status (opt_status));

    if (prG->verbose > 1)
        rl_printf ("<%ld> ", opt_type);

    uiG.last_rcvd = cont;
    if (cont)
    {
        s_repl (&cont->last_message, cdata);
        cont->last_time = time (NULL);
    }

    switch (opt_type & ~MSGF_MASS)
    {
        case MSGF_MASS: /* not reached here, but quiets compiler warning */
        while (1)
        {
            rl_printf ("(?%lx?) %s" COLMSGINDENT "%s\n", opt_type, COLMESSAGE, opt_text);
            rl_printf ("    '");
            for (j = 0; j < strlen (opt_text); j++)
                rl_printf ("%c", ((cdata[j] & 0xe0) && (cdata[j] != 127)) ? cdata[j] : '.');
            rl_print ("'\n");
            break;

        case MSG_NORM:
        default:
            rl_printf ("%s %s" COLMSGINDENT "%s\n", carr, COLMESSAGE, cdata);
            HistMsg (conn, cont, stamp == NOW ? time (NULL) : stamp, cdata, HIST_IN);
            TCLEvent (cont, "message", s_sprintf ("{%s}", cdata));
            TCLMessage (cont, cdata);
            break;

        case MSG_FILE:
            if (!OptGetVal (opt, CO_BYTES, &opt_bytes))
                opt_bytes = 0;
            if (!OptGetVal (opt, CO_REF, &opt_ref))
                opt_ref = 0;
            rl_printf (i18n (2468, "requests file transfer %s of %s%ld%s bytes (sequence %s%ld%s).\n"),
                      s_qquote (cdata), COLQUOTE, opt_bytes, COLNONE, COLQUOTE, opt_ref, COLNONE);
            TCLEvent (cont, "file_request", s_sprintf ("{%s} %ld %ld", cdata, opt_bytes, opt_ref));
            break;

        case MSG_AUTO:
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (2108, "auto"), COLMESSAGE, cdata);
            break;

        case MSGF_GETAUTO | MSG_GET_AWAY: 
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (1972, "away"), COLMESSAGE, cdata);
            break;

        case MSGF_GETAUTO | MSG_GET_OCC:
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (1973, "occupied"), COLMESSAGE, cdata);
            break;

        case MSGF_GETAUTO | MSG_GET_NA:
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (1974, "not available"), COLMESSAGE, cdata);
            break;

        case MSGF_GETAUTO | MSG_GET_DND:
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (1971, "do not disturb"), COLMESSAGE, cdata);
            break;

        case MSGF_GETAUTO | MSG_GET_FFC:
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (1976, "free for chat"), COLMESSAGE, cdata);
            break;

        case MSGF_GETAUTO | MSG_GET_VER:
            rl_printf ("<%s> %s" COLMSGINDENT "%s\n", i18n (2109, "version"), COLMESSAGE, cdata);
            break;

        case MSG_URL:
            tmp  = s_msgtok (cdata); if (!tmp)  continue;
            tmp2 = s_msgtok (NULL);  if (!tmp2) continue;
            
            rl_printf ("%s %s\n%s %*s", carr, s_msgquote (tmp), s_now, uiG.nick_len - 4, "");
            rl_printf ("%s %s %s\n", i18n (2469, "URL:"), carr, s_wordquote (tmp2));
            HistMsg (conn, cont, stamp == NOW ? time (NULL) : stamp, cdata, HIST_IN);
            break;

        case MSG_AUTH_REQ:
            tmp = s_msgtok (cdata); if (!tmp) continue;
            tmp2 = s_msgtok (NULL);
            tmp3 = tmp4 = tmp5 = tmp6 = NULL;
            
            if (tmp2)
            {
                tmp3 = s_msgtok (NULL); if (!tmp3) continue;
                tmp4 = s_msgtok (NULL); if (!tmp4) continue;
                tmp5 = s_msgtok (NULL); if (!tmp5) continue;
                tmp6 = s_msgtok (NULL); if (!tmp6) continue;
            }
            else
            {
                tmp6 = tmp;
                tmp = NULL;
            }

            rl_printf (i18n (2470, "requests authorization: %s\n"), s_msgquote (tmp6));
            
            if (tmp && strlen (tmp))
                rl_printf ("%-15s %s\n", "???1:", s_wordquote (tmp));
            if (tmp2 && strlen (tmp2))
                rl_printf ("%-15s %s\n", i18n (1564, "First name:"), s_wordquote (tmp2));
            if (tmp3 && strlen (tmp3))
                rl_printf ("%-15s %s\n", i18n (1565, "Last name:"), s_wordquote (tmp3));
            if (tmp4 && strlen (tmp4))
                rl_printf ("%-15s %s\n", i18n (1566, "Email address:"), s_wordquote (tmp4));
            if (tmp5 && strlen (tmp5))
                rl_printf ("%-15s %s\n", "???5:", s_wordquote (tmp5));
            TCLEvent (cont, "authorization", "request");
            break;

        case MSG_AUTH_DENY:
            rl_printf (i18n (2233, "refused authorization: %s%s%s\n"), COLMESSAGE, COLMSGINDENT, cdata);
            TCLEvent (cont, "authorization", "refused");
            break;

        case MSG_AUTH_GRANT:
            rl_print (i18n (1901, "has authorized you to add them to your contact list.\n"));
            TCLEvent (cont, "authorization", "granted");
            break;

        case MSG_AUTH_ADDED:
            tmp = s_msgtok (cdata);
            if (!tmp)
            {
                rl_print (i18n (1755, "has added you to their contact list.\n"));
                TCLEvent (cont, "contactlistadded", "");
                break;
            }
            tmp2 = s_msgtok (NULL); if (!tmp2) continue;
            tmp3 = s_msgtok (NULL); if (!tmp3) continue;
            tmp4 = s_msgtok (NULL); if (!tmp4) continue;

            rl_printf ("%s ", s_cquote (tmp, COLCONTACT));
            rl_print  (i18n (1755, "has added you to their contact list.\n"));
            rl_printf ("%-15s %s\n", i18n (1564, "First name:"), s_wordquote (tmp2));
            rl_printf ("%-15s %s\n", i18n (1565, "Last name:"), s_wordquote (tmp3));
            rl_printf ("%-15s %s\n", i18n (1566, "Email address:"), s_wordquote (tmp4));
            TCLEvent (cont, "contactlistadded", "");
            break;

        case MSG_EMAIL:
        case MSG_WEB:
            tmp  = s_msgtok (cdata); if (!tmp)  continue;
            tmp2 = s_msgtok (NULL);  if (!tmp2) continue;
            tmp3 = s_msgtok (NULL);  if (!tmp3) continue;
            tmp4 = s_msgtok (NULL);  if (!tmp4) continue;
            tmp5 = s_msgtok (NULL);  if (!tmp5) continue;
            tmp6 = s_msgtok (NULL);  if (!tmp6) continue;

            if (opt_type == MSG_EMAIL)
            {
                rl_printf (i18n (2571, "\"%s\" <%s> emailed you a message [%s]: %s\n"),
                    s_cquote (tmp, COLCONTACT), s_cquote (tmp4, COLCONTACT),
                    s_cquote (tmp5, COLCONTACT), s_msgquote (tmp6));
                TCLEvent (cont, "mail", s_sprintf ("{%s} {%s} {%s} {%s}", tmp, tmp4, tmp5, tmp6));
            }
            else
            {
                rl_printf (i18n (2572, "\"%s\" <%s> sent you a web message [%s]: %s\n"),
                    s_cquote (tmp, COLCONTACT), s_cquote (tmp4, COLCONTACT),
                    s_cquote (tmp5, COLCONTACT), s_msgquote (tmp6));
                TCLEvent (cont, "web", s_sprintf ("{%s} {%s} {%s} {%s}", tmp, tmp4, tmp5, tmp6));
            }
            break;

        case MSG_CONTACT:
            tmp = s_msgtok (cdata); if (!tmp) continue;

            rl_printf (i18n (1595, "\nContact List.\n============================================\n%d Contacts\n"),
                     i = atoi (tmp));

            while (i--)
            {
                tmp2 = s_msgtok (NULL); if (!tmp2) continue;
                tmp3 = s_msgtok (NULL); if (!tmp3) continue;
                
                rl_print  (s_cquote (tmp2, COLCONTACT));
                rl_printf ("\t\t\t%s\n", s_msgquote (tmp3));
            }
            break;
        }
    }
    free (cdata);
    OptD (opt);
}
