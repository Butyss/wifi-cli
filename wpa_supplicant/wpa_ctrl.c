#include "wpa_ctrl.h"
#include "includes.h"
#include <sys/uio.h>

#define WPA_CTRL_MAX_REPLY_SIZE 4096
#define WPA_CTRL_MSGS_MAX_SIZE 1024

static int wpa_ctrl_open_unix(struct wpa_ctrl *ctrl, const char *ctrl_path)
{
    struct sockaddr_un addr;
    int s;

    s = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s < 0)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ctrl_path, sizeof(addr.sun_path) - 1);

    if (connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    ctrl->s = s;
    return 0;
}

struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path)
{
    struct wpa_ctrl *ctrl;
    char *buf;

    ctrl = malloc(sizeof(*ctrl));
    if (!ctrl)
        return NULL;
    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->msg_queue = malloc(WPA_CTRL_MSGS_MAX_SIZE);
    if (!ctrl->msg_queue) {
        free(ctrl);
        return NULL;
    }
    ctrl->msg_queue_size = WPA_CTRL_MSGS_MAX_SIZE;

    if (wpa_ctrl_open_unix(ctrl, ctrl_path) < 0) {
        free(ctrl->msg_queue);
        free(ctrl);
        return NULL;
    }

    ctrl->ctrl_path = strdup(ctrl_path);
    return ctrl;
}

void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
    if (ctrl->s >= 0)
        close(ctrl->s);
    if (ctrl->ctrl_path)
        free(ctrl->ctrl_path);
    if (ctrl->msg_queue)
        free(ctrl->msg_queue);
    free(ctrl);
}

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
                     char *reply, size_t *reply_len,
                     void (*msg_cb)(char *msg, size_t len))
{
    struct sockaddr_un addr;
    char *buf;
    size_t len;
    ssize_t res;
    int attempts = 0;

    buf = malloc(WPA_CTRL_MAX_REPLY_SIZE);
    if (!buf)
        return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ctrl->ctrl_path, sizeof(addr.sun_path) - 1);

retry:
    res = sendto(ctrl->s, cmd, cmd_len, 0, (struct sockaddr *) &addr, sizeof(addr));
    if (res < 0) {
        free(buf);
        return -1;
    }

    len = recv(ctrl->s, buf, WPA_CTRL_MAX_REPLY_SIZE - 1, 0);
    if (len < 0) {
        free(buf);
        return -1;
    }

    buf[len] = '\0';

    if ((size_t) len >= 4 && strncmp(buf, "FAIL", 4) == 0) {
        free(buf);
        return -1;
    }

    if ((size_t) len >= 3 && strncmp(buf, "OK\n", 3) == 0) {
        if (len > 3)
            memcpy(reply, buf + 3, len - 3);
        else
            len = 0;
        *reply_len = len;
        free(buf);
        return 0;
    }

    if (len > 6 && strncmp(buf, "<3>", 3) == 0) {
        if (msg_cb)
            msg_cb(buf + 3, len - 3);
        attempts++;
        if (attempts < 100)
            goto retry;
    }

    memcpy(reply, buf, len);
    *reply_len = len;
    free(buf);
    return 0;
}

int wpa_ctrl_attach(struct wpa_ctrl *ctrl)
{
    char reply[10];
    size_t reply_len = sizeof(reply);

    return wpa_ctrl_request(ctrl, "ATTACH", 6, reply, &reply_len, NULL);
}

int wpa_ctrl_detach(struct wpa_ctrl *ctrl)
{
    char reply[10];
    size_t reply_len = sizeof(reply);

    return wpa_ctrl_request(ctrl, "DETACH", 6, reply, &reply_len, NULL);
}

int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
    ssize_t len;

    len = recv(ctrl->s, reply, WPA_CTRL_MAX_REPLY_SIZE - 1, 0);
    if (len < 0)
        return -1;

    reply[len] = '\0';
    *reply_len = len;
    return 0;
}

int wpa_ctrl_pending(struct wpa_ctrl *ctrl)
{
    struct timeval tv;
    fd_set rfd;
    int res;

    FD_ZERO(&rfd);
    FD_SET(ctrl->s, &rfd);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    res = select(ctrl->s + 1, &rfd, NULL, NULL, &tv);
    if (res < 0)
        return -1;

    return FD_ISSET(ctrl->s, &rfd) ? 1 : 0;
}

int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl)
{
    return ctrl->s;
}
