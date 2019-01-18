/* Header file "imap.hpp" */

/* used in imap.cpp */

/* Author: Ulrik Stig Hansen
   Program last changed: 13th December 2018 */

#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>
#include <iostream>

#define MAX_MESSAGES 100

/* HELPER FUNCTIONS */

namespace IMAP {
class Message {
public:

	struct mailimap * imap = 0;
	uint32_t uid = 0;
	int r = 0;
	std::string from = "";
	std::function<void()> updateUI;
	Message** messages;

	Message(struct mailimap* imap, uint32_t uid, std::function<void()> updateUI, Message** messages)
		: imap(imap), uid(uid), updateUI(updateUI), messages(messages) {}

	std::string getBody();
	std::string get_msg_content(clist * fetch_result, size_t * p_msg_size);
	std::string get_msg_att_msg_content(struct mailimap_msg_att * msg_att, size_t * p_msg_size);
	void to_string(clist* list);
	std::string getField(std::string fieldname);
	void deleteFromMailbox();

};

class Session {
public:

	struct mailimap * imap = mailimap_new(0, NULL);
	Message** messages;
	int r = 0;
	std::function<void()> updateUI;

	Session(std::function<void()> updateUI) : updateUI(updateUI) {}

	int numberOfMessages();
	Message** getMessages();
	void connect(std::string const& server, size_t port = 143);
	void login(std::string const& userid, std::string const& password);
	void selectMailbox(std::string const& mailbox);
	uint32_t get_uid(struct mailimap_msg_att * msg_att);

	~Session() {

		mailimap_logout(imap);
		mailimap_free(imap);

		for(int i = 0; messages[i] != nullptr; i++)
			delete messages[i];

		delete [] messages;

	}
};

}

#endif /* IMAP_H */
