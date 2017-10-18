#ifndef _MAILMSG_H_
#define _MAILMSG_H_

#define MAX_MAIL_MSG_DATA_SIZE  32

/**
 * This struct is for a single piece of Mail to be sent to a thread. In our app
 * the second byte of `content` tells the thread how to "dispatch" the message.
 */
typedef struct {
    char content[MAX_MAIL_MSG_DATA_SIZE];
    size_t length;
} MailMsg;

#endif /* _MAILMSG_H_ */