#include "http.h"
#include "diagnostic.h"
#include "lwip/tcp.h"
#include <string.h>

typedef struct {
    struct tcp_pcb *pcb;
    char request[HTTP_REQUEST_CAP + 1];
    char response[HTTP_RESPONSE_CAP];
    size_t used, response_len, queued, acknowledged;
    unsigned polls;
    bool replying;
} client_t;
static client_t clients[2];
static struct tcp_pcb *listener;

static void detach(client_t *c) {
    tcp_arg(c->pcb, NULL); tcp_recv(c->pcb, NULL); tcp_sent(c->pcb, NULL);
    tcp_poll(c->pcb, NULL, 0); tcp_err(c->pcb, NULL);
}
static err_t finish(client_t *c, bool abort_now) {
    struct tcp_pcb *pcb = c->pcb;
    detach(c); memset(c, 0, sizeof(*c));
    if (abort_now || tcp_close(pcb) != ERR_OK) { tcp_abort(pcb); return ERR_ABRT; }
    return ERR_OK;
}
static void error_cb(void *arg, err_t err) {
    (void)err;
    if (arg) memset(arg, 0, sizeof(client_t));
}
static err_t pump(client_t *c) {
    if (!c->replying) return ERR_OK;
    if (c->acknowledged == c->response_len) return finish(c, false);
    size_t remaining = c->response_len - c->queued;
    if (remaining) {
        size_t n = tcp_sndbuf(c->pcb);
        if (n > remaining) n = remaining;
        if (n) {
            err_t err = tcp_write(c->pcb, c->response+c->queued, (u16_t)n, TCP_WRITE_FLAG_COPY);
            if (err == ERR_OK) c->queued += n;
            else if (err != ERR_MEM) return finish(c, true);
        }
        tcp_output(c->pcb);
    }
    return ERR_OK;
}
static err_t sent_cb(void *arg, struct tcp_pcb *pcb, u16_t len) {
    (void)pcb;
    client_t *c = arg;
    c->acknowledged += len;
    return pump(c);
}
static err_t poll_cb(void *arg, struct tcp_pcb *pcb) {
    (void)pcb;
    client_t *c = arg;
    // Absolute bound, not reset by trickled input: 10 seconds per client.
    if (++c->polls >= 10) return finish(c, true);
    return pump(c);
}
static err_t recv_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    client_t *c = arg;
    if (!p) return c->replying ? pump(c) : finish(c, false);
    if (err != ERR_OK) { pbuf_free(p); return finish(c, true); }
    tcp_recved(pcb, p->tot_len);
    if (c->replying) { pbuf_free(p); return ERR_OK; }
    size_t n = p->tot_len;
    bool overflow = n > HTTP_REQUEST_CAP-c->used;
    if (overflow) n = HTTP_REQUEST_CAP-c->used;
    pbuf_copy_partial(p, c->request+c->used, (u16_t)n, 0);
    c->used += n; c->request[c->used] = 0;
    pbuf_free(p);
    bool complete = strstr(c->request, "\r\n\r\n") != NULL;
    if (complete || overflow || c->used == HTTP_REQUEST_CAP) {
        int route = overflow || (!complete && c->used == HTTP_REQUEST_CAP) ? 431 : http_route(c->request, c->used);
        c->response_len = http_response(c->response, sizeof(c->response), route, &diagnostic);
        if (!c->response_len) return finish(c, true);
        c->replying = true;
        return pump(c);
    }
    return ERR_OK;
}
static err_t accept_cb(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)arg;
    if (!pcb) return ERR_VAL;
    if (err != ERR_OK) { tcp_abort(pcb); return ERR_ABRT; }
    for (size_t i = 0; i < 2; ++i) if (!clients[i].pcb) {
        client_t *c = &clients[i];
        memset(c, 0, sizeof(*c)); c->pcb = pcb;
        tcp_arg(pcb, c); tcp_recv(pcb, recv_cb); tcp_err(pcb, error_cb);
        tcp_sent(pcb, sent_cb); tcp_poll(pcb, poll_cb, 2);
        return ERR_OK;
    }
    tcp_abort(pcb); return ERR_ABRT;
}
bool http_start(void) {
    if (listener) return true;
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return false;
    if (tcp_bind(pcb, IP_ANY_TYPE, 80) != ERR_OK) { tcp_abort(pcb); return false; }
    listener = tcp_listen_with_backlog(pcb, 2);
    if (!listener) { tcp_abort(pcb); return false; }
    tcp_accept(listener, accept_cb);
    return true;
}
