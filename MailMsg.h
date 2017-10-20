#ifndef _MAILMSG_H_
#define _MAILMSG_H_

#define MAX_MAIL_MSG_DATA_SIZE  32

/**
 * This struct is for a single piece of Mail to be sent to a thread. In our app
 * the first byte is used to indicate which thread to forward MQTT messages to.
 * However, this is already looked at by the messageArrived() callback. 
 * Nonetheless, we will store the entire MQTT payload via mail for illustration.
 * The second byte of `content` tells the thread how to "dispatch" the message.
 * 
 * The alternative way to design this struct is to extract any application layer
 * headers you have in your MQTT message payload and store them into different
 * struct members. Your .content can then hold just the payload of your 
 * application layer packet.
 */
typedef struct {
    char content[MAX_MAIL_MSG_DATA_SIZE];
    size_t length;
} MailMsg;

#endif /* _MAILMSG_H_ */