/*
 * Copyright (C) 2026 by the M-Time Simulation project
 *
 * This file is part of Open5GS (AckNCT/open5gs `mtime-lcs` fork).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ogs-sctp.h"
#include "lcs-ap-path.h"

#include "mme-context.h"
#include "mme-event.h"
#include "s1ap-path.h"

#if HAVE_USRSCTP
static void usrsctp_recv_handler(struct socket *socket, void *data, int flags);
#else
static void lksctp_accept_handler(short when, ogs_socket_t fd, void *data);
static void lksctp_recv_handler(short when, ogs_socket_t fd, void *data);
#endif

static void accept_handler(ogs_sock_t *sock);
static void recv_handler(ogs_sock_t *sock);

/*
 * SLs (LCS-AP, TS 29.171): the MME is the SCTP server. It listens and the
 * E-SMLC (SCTP client) dials in; on accept we attach the association to the
 * single esmlc context and post MME_EVENT_LCS_AP_LO_ACCEPT to drive its FSM.
 */
ogs_sock_t *lcs_ap_server(ogs_socknode_t *node)
{
    char buf[OGS_ADDRSTRLEN];
    ogs_sock_t *sock = NULL;
#if !HAVE_USRSCTP
    ogs_poll_t *poll = NULL;
#endif

    ogs_assert(node);

#if HAVE_USRSCTP
    sock = ogs_sctp_server(SOCK_SEQPACKET, node->addr, node->option);
    if (!sock) return NULL;
    usrsctp_set_non_blocking((struct socket *)sock, 1);
    usrsctp_set_upcall((struct socket *)sock, usrsctp_recv_handler, NULL);
#else
    sock = ogs_sctp_server(SOCK_STREAM, node->addr, node->option);
    if (!sock) return NULL;
    poll = ogs_pollset_add(ogs_app()->pollset,
            OGS_POLLIN, sock->fd, lksctp_accept_handler, sock);
    ogs_assert(poll);

    node->poll = poll;
#endif

    node->sock = sock;
    node->cleanup = ogs_sctp_destroy;

    ogs_info("lcs_ap_server() [%s]:%d",
            OGS_ADDR(node->addr, buf), OGS_PORT(node->addr));

    return sock;
}

static void accept_handler(ogs_sock_t *sock)
{
    char buf[OGS_ADDRSTRLEN];
    ogs_sock_t *new = NULL;
    mme_esmlc_t *esmlc = NULL;

    ogs_assert(sock);

    new = ogs_sock_accept(sock);
    if (!new) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno, "accept() failed");
        return;
    }

    esmlc = ogs_list_first(&mme_self()->esmlc_list);
    if (!esmlc) {
        ogs_error("[LCS-AP] No E-SMLC configured; dropping association [%s]:%d",
                OGS_ADDR(&new->remote_addr, buf),
                OGS_PORT(&new->remote_addr));
        ogs_sctp_destroy(new);
        return;
    }

    /* One association at a time: reject a second dial-in until the current one
     * drops (CONNREFUSED clears esmlc->sock). */
    if (esmlc->sock) {
        ogs_warn("[LCS-AP] E-SMLC already connected; "
                "rejecting new association [%s]:%d",
                OGS_ADDR(&new->remote_addr, buf),
                OGS_PORT(&new->remote_addr));
        ogs_sctp_destroy(new);
        return;
    }

    esmlc->sock = new;
    esmlc->poll = ogs_pollset_add(ogs_app()->pollset,
            OGS_POLLIN, new->fd, lksctp_recv_handler, new);
    ogs_assert(esmlc->poll);

    ogs_info("[LCS-AP] E-SMLC accepted [%s]:%d",
            OGS_ADDR(&new->remote_addr, buf),
            OGS_PORT(&new->remote_addr));

    lcs_ap_event_push(MME_EVENT_LCS_AP_LO_ACCEPT, new, NULL, NULL, 0, 0);
}

#if HAVE_USRSCTP
static void usrsctp_recv_handler(struct socket *socket, void *data, int flags)
{
    int events;

    while ((events = usrsctp_get_events(socket)) &&
           (events & SCTP_EVENT_READ)) {
        recv_handler((ogs_sock_t *)socket);
    }
}
#else
static void lksctp_accept_handler(short when, ogs_socket_t fd, void *data)
{
    ogs_assert(data);
    ogs_assert(fd != INVALID_SOCKET);

    accept_handler(data);
}

static void lksctp_recv_handler(short when, ogs_socket_t fd, void *data)
{
    ogs_sock_t *sock = NULL;

    sock = data;
    ogs_assert(fd != INVALID_SOCKET);
    ogs_assert(sock);

    recv_handler(sock);
}
#endif

static void recv_handler(ogs_sock_t *sock)
{
    ogs_pkbuf_t *pkbuf;
    int size;
    ogs_sctp_info_t sinfo;
    int flags = 0;

    ogs_assert(sock);

    pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_SDU_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put(pkbuf, OGS_MAX_SDU_LEN);
    size = ogs_sctp_recvmsg(
            sock, pkbuf->data, pkbuf->len, NULL, &sinfo, &flags);
    if (size < 0 || size >= OGS_MAX_SDU_LEN) {
        ogs_error("ogs_sctp_recvmsg(%d) failed(%d:%s)",
                size, errno, strerror(errno));
        ogs_pkbuf_free(pkbuf);
        return;
    }

    if (flags & MSG_NOTIFICATION) {
        union sctp_notification *not =
            (union sctp_notification *)pkbuf->data;

        switch(not->sn_header.sn_type) {
        case SCTP_ASSOC_CHANGE :
            ogs_debug("SCTP_ASSOC_CHANGE:"
                    "[T:%d, F:0x%x, S:%d, I/O:%d/%d]",
                    not->sn_assoc_change.sac_type,
                    not->sn_assoc_change.sac_flags,
                    not->sn_assoc_change.sac_state,
                    not->sn_assoc_change.sac_inbound_streams,
                    not->sn_assoc_change.sac_outbound_streams);

            if (not->sn_assoc_change.sac_state == SCTP_COMM_UP) {
                ogs_debug("SCTP_COMM_UP");

                if ((not->sn_assoc_change.sac_outbound_streams-1) >= 1) {
                    /* NEXT_ID(MAX >= MIN) */
                    lcs_ap_event_push(MME_EVENT_LCS_AP_LO_SCTP_COMM_UP,
                            sock, NULL, NULL,
                            not->sn_assoc_change.sac_inbound_streams,
                            not->sn_assoc_change.sac_outbound_streams);
                }
            } else if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP ||
                    not->sn_assoc_change.sac_state == SCTP_COMM_LOST) {

                if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP)
                    ogs_debug("SCTP_SHUTDOWN_COMP");
                if (not->sn_assoc_change.sac_state == SCTP_COMM_LOST)
                    ogs_debug("SCTP_COMM_LOST");

                lcs_ap_event_push(MME_EVENT_LCS_AP_LO_CONNREFUSED,
                        sock, NULL, NULL, 0, 0);
            }
            break;
        case SCTP_SHUTDOWN_EVENT :
        case SCTP_SEND_FAILED :
            if (not->sn_header.sn_type == SCTP_SHUTDOWN_EVENT)
                ogs_debug("SCTP_SHUTDOWN_EVENT:[T:%d, F:0x%x, L:%d]",
                        not->sn_shutdown_event.sse_type,
                        not->sn_shutdown_event.sse_flags,
                        not->sn_shutdown_event.sse_length);
            if (not->sn_header.sn_type == SCTP_SEND_FAILED)
#if HAVE_USRSCTP
                ogs_error("SCTP_SEND_FAILED:[T:%d, F:0x%x, S:%d]",
                        not->sn_send_failed_event.ssfe_type,
                        not->sn_send_failed_event.ssfe_flags,
                        not->sn_send_failed_event.ssfe_error);
#else
                ogs_error("SCTP_SEND_FAILED:[T:%d, F:0x%x, S:%d]",
                        not->sn_send_failed.ssf_type,
                        not->sn_send_failed.ssf_flags,
                        not->sn_send_failed.ssf_error);
#endif

            lcs_ap_event_push(MME_EVENT_LCS_AP_LO_CONNREFUSED,
                    sock, NULL, NULL, 0, 0);
            break;
        case SCTP_PEER_ADDR_CHANGE:
            ogs_warn("SCTP_PEER_ADDR_CHANGE:[T:%d, F:0x%x, S:%d]",
                    not->sn_paddr_change.spc_type,
                    not->sn_paddr_change.spc_flags,
                    not->sn_paddr_change.spc_error);
            break;
        case SCTP_REMOTE_ERROR:
            ogs_warn("SCTP_REMOTE_ERROR:[T:%d, F:0x%x, S:%d]",
                    not->sn_remote_error.sre_type,
                    not->sn_remote_error.sre_flags,
                    not->sn_remote_error.sre_error);
            break;
        default :
            ogs_error("Discarding event with unknown flags:0x%x type:0x%x",
                    flags, not->sn_header.sn_type);
            break;
        }
    } else if (flags & MSG_EOR) {
        ogs_pkbuf_trim(pkbuf, size);

        lcs_ap_event_push(MME_EVENT_LCS_AP_MESSAGE, sock, NULL, pkbuf, 0, 0);
        return;
    } else {
        ogs_error("ogs_sctp_recvmsg(%d) failed(%d:%s-0x%x)",
                size, errno, strerror(errno), flags);
    }
    ogs_pkbuf_free(pkbuf);
}
